import type { Configuration } from './specs/MMKVFactory.nitro'
import type { MMKV as MMKVType } from './specs/MMKV.nitro'
import { createMMKV } from './createMMKV/createMMKV'

/**
 * MMKV constructor function that creates a native HybridObject instance.
 * 
 * This function can be used with or without the `new` keyword:
 * ```ts
 * const storage1 = new MMKV({ id: 'user-storage' })
 * const storage2 = MMKV({ id: 'app-storage' })
 * ```
 * 
 * @param config - Optional configuration for the MMKV instance
 * @returns A native MMKV HybridObject instance
 */
export const MMKV = function (this: any, config?: Configuration): MMKVType {
  // Always return the native instance, regardless of how it's called
  const instance = createMMKV(config)
  return instance
} as {
  new (config?: Configuration): MMKVType
  (config?: Configuration): MMKVType
}

// Export the type for TypeScript compatibility
export type { MMKV as MMKVType } from './specs/MMKV.nitro'
