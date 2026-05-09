const { getDefaultConfig, mergeConfig } = require('@react-native/metro-config');
const path = require('path');

/** @type {import('@react-native/metro-config').MetroConfig} */
const overrides = {
  watchFolders: [path.resolve(__dirname, '..')],
  resolver: {
    platforms: ['web', 'ios', 'android', 'native'],
  },
};

const config = mergeConfig(getDefaultConfig(__dirname), overrides);

const upstreamResolveRequest = config.resolver.resolveRequest;
config.resolver.resolveRequest = (context, moduleName, platform) => {
  if (
    platform === 'web' &&
    (moduleName === 'react-native' || moduleName === 'react-native/index')
  ) {
    return context.resolveRequest(context, 'react-native-web', platform);
  }
  if (upstreamResolveRequest) {
    return upstreamResolveRequest(context, moduleName, platform);
  }
  return context.resolveRequest(context, moduleName, platform);
};

module.exports = config;
