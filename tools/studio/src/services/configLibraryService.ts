import {
  type ConfigBackupFile,
  type ConfigLibraryItem,
} from '@/types/configBackup';
import { backupToDownloadName, finalizeImportedBackup, parseConfigBackupFile } from '@/services/configSnapshotService';

const DB_NAME = 'binarykeyboard-config-library';
const DB_VERSION = 2;
const LEGACY_STORE_NAME = 'slots';
const STORE_NAME = 'configs';

function openDb(): Promise<IDBDatabase> {
  return new Promise((resolve, reject) => {
    const request = indexedDB.open(DB_NAME, DB_VERSION);
    request.onerror = () => reject(request.error ?? new Error('无法打开配置库'));
    request.onupgradeneeded = (event) => {
      const db = request.result;
      const tx = request.transaction;
      const oldVersion = event.oldVersion;
      let store: IDBObjectStore;
      if (!db.objectStoreNames.contains(STORE_NAME)) {
        store = db.createObjectStore(STORE_NAME, { keyPath: 'index' });
      } else if (tx) {
        store = tx.objectStore(STORE_NAME);
      } else {
        return;
      }

      if (oldVersion < 2 && tx && db.objectStoreNames.contains(LEGACY_STORE_NAME)) {
        const legacyStore = tx.objectStore(LEGACY_STORE_NAME);
        legacyStore.openCursor().onsuccess = (cursorEvent) => {
          const cursor = (cursorEvent.target as IDBRequest<IDBCursorWithValue | null>).result;
          if (!cursor) return;
          store.put(cursor.value);
          cursor.continue();
        };
      }
    };
    request.onsuccess = () => resolve(request.result);
  });
}

async function withStore<T>(
  mode: IDBTransactionMode,
  task: (store: IDBObjectStore) => IDBRequest<T> | void,
): Promise<T | undefined> {
  const db = await openDb();
  return new Promise((resolve, reject) => {
    const tx = db.transaction(STORE_NAME, mode);
    const store = tx.objectStore(STORE_NAME);
    let request: IDBRequest<T> | void;
    let settled = false;
    const fail = (error: unknown) => {
      if (settled) return;
      settled = true;
      db.close();
      reject(error);
    };
    tx.oncomplete = () => {
      if (settled) return;
      settled = true;
      db.close();
      resolve(request ? request.result : undefined);
    };
    tx.onerror = () => fail(tx.error ?? new Error('配置库操作失败'));
    tx.onabort = () => fail(tx.error ?? new Error('配置库事务取消'));
    try {
      const nextRequest = task(store);
      request = nextRequest;
      if (nextRequest) {
        nextRequest.onerror = () => fail(nextRequest.error ?? new Error('配置库请求失败'));
      }
    } catch (error) {
      tx.abort();
      fail(error);
    }
  });
}

function configId(index: number): string {
  return `config-${index + 1}`;
}

function assertConfigIndex(index: number): void {
  if (!Number.isInteger(index) || index < 0) {
    throw new Error(`无效配置编号：${index + 1}`);
  }
}

export async function listConfigItems(): Promise<ConfigLibraryItem[]> {
  const result = await withStore<ConfigLibraryItem[]>('readonly', (store) => store.getAll());
  return (result ?? [])
    .filter((item) => Number.isInteger(item.index) && item.index >= 0)
    .sort((a, b) => b.updatedAt.localeCompare(a.updatedAt) || b.index - a.index);
}

export async function getConfigItem(index: number): Promise<ConfigLibraryItem | null> {
  assertConfigIndex(index);
  return (await withStore<ConfigLibraryItem>('readonly', (store) => store.get(index))) ?? null;
}

async function nextConfigIndex(): Promise<number> {
  const items = await listConfigItems();
  return items.reduce((max, item) => Math.max(max, item.index), -1) + 1;
}

export async function createConfigItem(
  backup: ConfigBackupFile,
  name = backup.name,
): Promise<ConfigLibraryItem> {
  return saveConfigItem(await nextConfigIndex(), backup, name);
}

export async function saveConfigItem(
  index: number,
  backup: ConfigBackupFile,
  name = backup.name,
): Promise<ConfigLibraryItem> {
  assertConfigIndex(index);
  const existing = await getConfigItem(index);
  const now = new Date().toISOString();
  const finalized = await finalizeImportedBackup({ ...backup, name }, backup.source);
  const item: ConfigLibraryItem = {
    id: existing?.id ?? configId(index),
    index,
    name: name.trim() || `配置 ${index + 1}`,
    createdAt: existing?.createdAt ?? now,
    updatedAt: now,
    source: finalized.source,
    backup: finalized,
  };
  await withStore('readwrite', (store) => {
    store.put(item);
  });
  return item;
}

export async function renameConfigItem(index: number, name: string): Promise<ConfigLibraryItem> {
  const item = await getConfigItem(index);
  if (!item) throw new Error('配置不存在');
  return saveConfigItem(index, { ...item.backup, name }, name);
}

export async function deleteConfigItem(index: number): Promise<void> {
  assertConfigIndex(index);
  await withStore('readwrite', (store) => {
    store.delete(index);
  });
}

export async function importBackupTextAsConfig(text: string, name?: string): Promise<ConfigLibraryItem> {
  const backup = await parseConfigBackupFile(text);
  return createConfigItem({ ...backup, source: 'file' }, name ?? backup.name);
}

export async function downloadConfigBackup(backup: ConfigBackupFile, name = backup.name): Promise<void> {
  const exportBackup = await finalizeImportedBackup({ ...backup, name }, backup.source);
  const blob = new Blob([JSON.stringify(exportBackup, null, 2)], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const link = document.createElement('a');
  link.href = url;
  link.download = backupToDownloadName(exportBackup);
  document.body.appendChild(link);
  link.click();
  link.remove();
  URL.revokeObjectURL(url);
}

export function readFileAsText(file: File): Promise<string> {
  return new Promise((resolve, reject) => {
    const reader = new FileReader();
    reader.onerror = () => reject(reader.error ?? new Error('读取文件失败'));
    reader.onload = () => resolve(String(reader.result ?? ''));
    reader.readAsText(file);
  });
}
