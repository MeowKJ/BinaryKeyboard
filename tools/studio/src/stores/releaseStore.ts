import { computed, ref } from "vue";
import { defineStore } from "pinia";
import {
  LOCAL_RELEASE_MANIFEST,
  RELEASE_FEED,
  STUDIO_VERSION,
} from "@/generated/versionConfig";

type ReleaseVersions = {
  studio: string;
  ch552: string;
  ch592: string;
};

type ReleaseManifest = {
  generatedAt?: string;
  commit?: string;
  repository?: string;
  versions?: Partial<ReleaseVersions>;
};

const DEFAULT_VERSIONS: ReleaseVersions = {
  studio: LOCAL_RELEASE_MANIFEST.versions.studio,
  ch552: LOCAL_RELEASE_MANIFEST.versions.ch552,
  ch592: LOCAL_RELEASE_MANIFEST.versions.ch592,
};

const RELEASE_RETRY_DELAY_MS = 60_000;

export const useReleaseStore = defineStore("release", () => {
  const latestVersions = ref<ReleaseVersions>({ ...DEFAULT_VERSIONS });
  const isLoading = ref(false);
  const hasLoaded = ref(false);
  const errorMessage = ref<string | null>(null);
  let retryTimer: ReturnType<typeof window.setTimeout> | null = null;

  const studioVersion = computed(() => STUDIO_VERSION);
  const repository = computed(() => RELEASE_FEED.repository);

  function applyManifest(manifest: ReleaseManifest): void {
    latestVersions.value = {
      studio: manifest.versions?.studio || DEFAULT_VERSIONS.studio,
      ch552: manifest.versions?.ch552 || DEFAULT_VERSIONS.ch552,
      ch592: manifest.versions?.ch592 || DEFAULT_VERSIONS.ch592,
    };
  }

  function clearRetry(): void {
    if (retryTimer !== null) {
      window.clearTimeout(retryTimer);
      retryTimer = null;
    }
  }

  function scheduleRetry(): void {
    if (retryTimer !== null) {
      return;
    }
    retryTimer = window.setTimeout(() => {
      retryTimer = null;
      void loadLatestVersions(true);
    }, RELEASE_RETRY_DELAY_MS);
  }

  async function loadLatestVersions(force = false): Promise<void> {
    if ((hasLoaded.value && !force) || isLoading.value) {
      return;
    }

    clearRetry();
    isLoading.value = true;
    errorMessage.value = null;

    try {
      const response = await fetch(RELEASE_FEED.manifestUrl, {
        headers: { Accept: "application/json" },
      });

      if (!response.ok) {
        throw new Error(`manifest request failed: ${response.status}`);
      }

      applyManifest((await response.json()) as ReleaseManifest);
      hasLoaded.value = true;
    } catch (error) {
      applyManifest(LOCAL_RELEASE_MANIFEST);
      errorMessage.value =
        error instanceof Error ? error.message : "release feed unavailable";
      hasLoaded.value = false;
      scheduleRetry();
    } finally {
      isLoading.value = false;
    }
  }

  return {
    latestVersions,
    studioVersion,
    repository,
    isLoading,
    hasLoaded,
    errorMessage,
    loadLatestVersions,
  };
});
