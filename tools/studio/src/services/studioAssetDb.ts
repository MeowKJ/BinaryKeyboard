import type { ConfigProfileDocument } from "@/types/studioAssets";

const DB_NAME = "meowkeyboard-studio-assets";
const DB_VERSION = 1;
const PROFILE_STORE = "configProfiles";

function openDb(): Promise<IDBDatabase> {
  return new Promise((resolve, reject) => {
    const request = indexedDB.open(DB_NAME, DB_VERSION);
    request.onerror = () => reject(request.error ?? new Error("IndexedDB 打开失败"));
    request.onupgradeneeded = () => {
      const db = request.result;
      if (!db.objectStoreNames.contains(PROFILE_STORE)) {
        const store = db.createObjectStore(PROFILE_STORE, { keyPath: "id" });
        store.createIndex("updatedAt", "updatedAt");
      }
    };
    request.onsuccess = () => resolve(request.result);
  });
}

function txDone(tx: IDBTransaction): Promise<void> {
  return new Promise((resolve, reject) => {
    tx.oncomplete = () => resolve();
    tx.onerror = () => reject(tx.error ?? new Error("IndexedDB 事务失败"));
    tx.onabort = () => reject(tx.error ?? new Error("IndexedDB 事务取消"));
  });
}

export async function listConfigProfiles(): Promise<ConfigProfileDocument[]> {
  const db = await openDb();
  try {
    const tx = db.transaction(PROFILE_STORE, "readonly");
    const store = tx.objectStore(PROFILE_STORE);
    const request = store.getAll();
    const profiles = await new Promise<ConfigProfileDocument[]>((resolve, reject) => {
      request.onerror = () => reject(request.error ?? new Error("读取配置档案失败"));
      request.onsuccess = () => resolve(request.result as ConfigProfileDocument[]);
    });
    return profiles.sort((a, b) => b.updatedAt.localeCompare(a.updatedAt));
  } finally {
    db.close();
  }
}

export async function putConfigProfile(profile: ConfigProfileDocument): Promise<void> {
  const db = await openDb();
  try {
    const tx = db.transaction(PROFILE_STORE, "readwrite");
    tx.objectStore(PROFILE_STORE).put(profile);
    await txDone(tx);
  } finally {
    db.close();
  }
}

export async function deleteConfigProfile(id: string): Promise<void> {
  const db = await openDb();
  try {
    const tx = db.transaction(PROFILE_STORE, "readwrite");
    tx.objectStore(PROFILE_STORE).delete(id);
    await txDone(tx);
  } finally {
    db.close();
  }
}
