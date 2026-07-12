/*
 * Retire service workers left behind by older Studio PWA deployments.
 *
 * The current web build does not register a service worker, but browsers that
 * visited a historical PWA build can otherwise keep serving its precache
 * indefinitely. Publishing this file at the original /sw.js URL lets those
 * registrations update once, clear their caches, and unregister themselves.
 */
self.addEventListener("install", () => {
  self.skipWaiting();
});

self.addEventListener("activate", (event) => {
  event.waitUntil((async () => {
    const cacheNames = await caches.keys();
    await Promise.all(cacheNames.map((cacheName) => caches.delete(cacheName)));
    await self.registration.unregister();

    const windows = await self.clients.matchAll({
      type: "window",
      includeUncontrolled: true,
    });
    await Promise.all(windows.map((client) => client.navigate(client.url)));
  })());
});
