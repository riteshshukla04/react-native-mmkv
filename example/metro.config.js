const { getDefaultConfig, mergeConfig } = require('@react-native/metro-config');
const path = require('path');

/**
 * Metro configuration
 * https://reactnative.dev/docs/metro
 *
 * @type {import('@react-native/metro-config').MetroConfig}
 */
const config = {
  watchFolders: [path.resolve(__dirname, '..')],
  resolver: {
    platforms: ['android', 'ios', 'web'],
    resolveRequest: (context, moduleName, platform) => {
      if (platform === 'web' && moduleName === 'react-native') {
        return context.resolveRequest(context, 'react-native-web', platform);
      }
      return context.resolveRequest(context, moduleName, platform);
    },
  },
  server: {
    enhanceMiddleware: (middleware) => {
      return (req, res, next) => {
        if (req.url === '/web') {
          res.setHeader('Content-Type', 'text/html');
          res.end(`<!DOCTYPE html>
<html><head>
<meta charset="utf-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1"/>
<style>html,body,#root{height:100%;margin:0}body{overflow:hidden}#root{display:flex}</style>
</head><body>
<div id="root"></div>
<script src="/index.bundle?platform=web&dev=true&hot=false&lazy=true&unstable_transformProfile=default"></script>
</body></html>`);
          return;
        }
        return middleware(req, res, next);
      };
    },
  },
};

module.exports = mergeConfig(getDefaultConfig(__dirname), config);
