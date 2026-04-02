import {
  androidPlatform,
  androidEmulator,
} from '@react-native-harness/platform-android';
import {
  applePlatform,
  appleSimulator,
} from '@react-native-harness/platform-apple';
import {
  webPlatform,
  chromium,
} from '@react-native-harness/platform-web';

const config = {
  entryPoint: './index.js',
  appRegistryComponentName: 'MmkvExample',

  runners: [
    androidPlatform({
      name: 'android',
      device: androidEmulator('Pixel_8_API_35', {
        apiLevel: 35,
        profile: 'pixel_6',
        diskSize: '1G',
        heapSize: '1G',
      }),
      bundleId: 'com.mrousavy.mmkv.example',
    }),
    applePlatform({
      name: 'ios',
      device: appleSimulator('iPhone 16 Pro', '18.6'),
      bundleId: 'com.mrousavy.mmkv.example',
    }),
    webPlatform({
      name: 'web',
      browser: chromium('http://localhost:8081/web'),
    }),
  ],
  defaultRunner: 'android',
  bridgeTimeout: 120000,

  resetEnvironmentBetweenTestFiles: true,
  unstable__skipAlreadyIncludedModules: false,
};

export default config;
