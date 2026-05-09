// Same-origin host that fronts `react-native start` and serves an HTML
// shell which mounts the app via AppRegistry.runApplication. Bare RN
// doesn't ship a web dev mode, so this is the minimal Expo-equivalent.
//
// Usage:
//   1. bun start                 # Metro on 8081
//   2. bun run web:dev           # this script on 3000
//   3. open http://localhost:3000

import http from 'node:http';
import { readFileSync } from 'node:fs';
import httpProxy from 'http-proxy';

const TARGET = process.env.METRO_URL ?? 'http://localhost:8081';
const PORT = Number(process.env.PORT ?? 3000);

const APP_NAME = (() => {
  try {
    return JSON.parse(readFileSync(new URL('../app.json', import.meta.url)))
      .name;
  } catch {
    return 'MmkvExample';
  }
})();

const HTML = `<!doctype html>
<html lang="en">
  <head>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width,initial-scale=1" />
    <title>${APP_NAME}</title>
    <style>html,body,#root{height:100%;margin:0;padding:0;}</style>
  </head>
  <body>
    <div id="root"></div>
    <script>
      window.__DEV__ = true;
      window.global = window;
      window.process = window.process || { env: { NODE_ENV: 'development' } };
      window.__fbBatchedBridgeConfig = window.__fbBatchedBridgeConfig || { remoteModuleConfig: [] };
      window.nativeModuleProxy = window.nativeModuleProxy || new Proxy({}, { get: () => ({}) });
      window.__turboModuleProxy = window.__turboModuleProxy || (() => null);
    </script>
    <script src="/index.bundle?platform=web&dev=true&minify=false"></script>
  </body>
</html>`;

const proxy = httpProxy.createProxyServer({
  target: TARGET,
  ws: true,
  changeOrigin: true,
});

proxy.on('error', (err, _req, res) => {
  console.error('[web-dev] upstream error:', err.message);
  if (res && !res.headersSent && typeof res.writeHead === 'function') {
    res.writeHead(502, { 'content-type': 'text/plain' });
    res.end('Bad gateway: ' + err.message);
  }
});

const server = http.createServer((req, res) => {
  const accept = req.headers['accept'] || '';
  if (req.url === '/' && accept.includes('text/html')) {
    res.writeHead(200, { 'content-type': 'text/html; charset=utf-8' });
    res.end(HTML);
    return;
  }
  proxy.web(req, res);
});

server.on('upgrade', (req, socket, head) => {
  proxy.ws(req, socket, head);
});

server.listen(PORT, () => {
  console.log(`[web-dev] http://localhost:${PORT} -> ${TARGET}`);
});
