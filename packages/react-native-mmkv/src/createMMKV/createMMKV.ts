import { NitroModules } from 'react-native-nitro-modules'
import type { MMKV } from '../specs/MMKV.nitro'
import type { Configuration } from '../specs/MMKVFactory.nitro'
import { addMemoryWarningListener } from '../addMemoryWarningListener/addMemoryWarningListener'
import { isTest } from '../isTest'
import { createMockMMKV } from './createMMKV.mock'

export function createMMKV(configuration?: Configuration): MMKV {
  if (isTest()) {
    // In a test environment, we mock the MMKV instance.
    return createMockMMKV()
  }

  // Get default ID if needed
  let config = configuration ?? { id: 'mmkv.default' }
  
  // Create the C++ MMKV HybridObject using default constructor
  const mmkv = NitroModules.createHybridObject<MMKV>('MMKV')
  
  // Initialize it with the configuration
  mmkv.initialize(config)
  
  // Add a hook that trims the storage when we get a memory warning
  addMemoryWarningListener(mmkv)
  return mmkv
}
