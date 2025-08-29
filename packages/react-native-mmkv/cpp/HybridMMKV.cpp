//
//  HybridMMKV.cpp
//  react-native-mmkv
//
//  Created by Marc Rousavy on 21.08.2025.
//

#include "HybridMMKV.hpp"
#include "MMKVTypes.hpp"
#include "MMKVValueChangedListenerRegistry.hpp"
#include "ManagedMMBuffer.hpp"
#include "HybridMMKVFactory.hpp"
#include "HybridMMKVPlatformContextSpec.hpp"
#include <NitroModules/NitroLogger.hpp>
#include <NitroModules/HybridObjectRegistry.hpp>

namespace margelo::nitro::mmkv {

// Static helper function for MMKV initialization logic
static void ensureMMKVInitialized() {
  static bool isMMKVInitialized = false;
  if (!isMMKVInitialized) {
    // Create platform context and factory to initialize MMKV
    auto platformContext = std::dynamic_pointer_cast<HybridMMKVPlatformContextSpec>(
      HybridObjectRegistry::createHybridObject("MMKVPlatformContext")
    );
    std::string baseDirectory = platformContext->getBaseDirectory();
    
    Logger::log(LogLevel::Info, "MMKV", "Initializing MMKV with rootPath=%s", baseDirectory.c_str());
    
#ifdef NITRO_DEBUG
    MMKVLogLevel logLevel = ::mmkv::MMKVLogDebug;
#else
    MMKVLogLevel logLevel = ::mmkv::MMKVLogWarning;
#endif
    MMKV::initializeMMKV(baseDirectory, logLevel);
    isMMKVInitialized = true;
  }
}

HybridMMKV::HybridMMKV() : HybridObject(TAG), instance(nullptr), isInitialized(false) {
  // Default constructor for autolinking - instance will be initialized later via initialize() method
}

HybridMMKV::HybridMMKV(const Configuration& config) : HybridObject(TAG), instance(nullptr), isInitialized(false) {
  initialize(config);
}

void HybridMMKV::initialize(const Configuration& config) {
  if (isInitialized) {
    Logger::log(LogLevel::Warning, TAG, "MMKV instance already initialized!");
    return;
  }
  // Ensure MMKV library is initialized
  ensureMMKVInitialized();

  // Process configuration - handle defaults and platform-specific logic
  Configuration processedConfig = config;
  
  // If no ID provided, use default
  if (processedConfig.id.empty()) {
    processedConfig.id = DEFAULT_MMAP_ID;
  }
  
#ifdef __APPLE__
  // Handle iOS App Group path detection
  if (!processedConfig.path.has_value()) {
    auto platformContext = std::dynamic_pointer_cast<HybridMMKVPlatformContextSpec>(
      HybridObjectRegistry::createHybridObject("MMKVPlatformContext")
    );
    std::string appGroupDirectory = platformContext->getAppGroupDirectory();
    if (!appGroupDirectory.empty()) {
      processedConfig.path = appGroupDirectory;
    }
  }
#endif

  // Extract configuration values
  std::string path = processedConfig.path.has_value() ? processedConfig.path.value() : "";
  std::string encryptionKey = processedConfig.encryptionKey.has_value() ? processedConfig.encryptionKey.value() : "";
  bool hasEncryptionKey = encryptionKey.size() > 0;
  
  Logger::log(LogLevel::Info, TAG, "Creating MMKV instance \"%s\"... (Path: %s, Encrypted: %s)", 
              processedConfig.id.c_str(), path.c_str(), hasEncryptionKey ? "true" : "false");

  std::string* pathPtr = path.size() > 0 ? &path : nullptr;
  std::string* encryptionKeyPtr = encryptionKey.size() > 0 ? &encryptionKey : nullptr;
  MMKVMode mode = getMMKVMode(processedConfig);
  
  if (processedConfig.readOnly.has_value() && processedConfig.readOnly.value()) {
    Logger::log(LogLevel::Info, TAG, "Instance is read-only!");
    mode = mode | ::mmkv::MMKV_READ_ONLY;
  }

#ifdef __APPLE__
  instance = MMKV::mmkvWithID(processedConfig.id, mode, encryptionKeyPtr, pathPtr);
#else
  instance = MMKV::mmkvWithID(processedConfig.id, DEFAULT_MMAP_SIZE, mode, encryptionKeyPtr, pathPtr);
#endif

  if (instance == nullptr) [[unlikely]] {
    // Check if instanceId is invalid
    if (processedConfig.id.empty()) [[unlikely]] {
      throw std::runtime_error("Failed to create MMKV instance! `id` cannot be empty!");
    }

    // Check if encryptionKey is invalid
    if (encryptionKey.size() > 16) [[unlikely]] {
      throw std::runtime_error("Failed to create MMKV instance! `encryptionKey` cannot be longer "
                               "than 16 bytes!");
    }

    // Check if path is maybe invalid
    if (path.empty()) [[unlikely]] {
      throw std::runtime_error("Failed to create MMKV instance! `path` cannot be empty!");
    }

    throw std::runtime_error("Failed to create MMKV instance!");
  }
  
  isInitialized = true;
}

double HybridMMKV::getSize() {
  if (!isInitialized || instance == nullptr) {
    throw std::runtime_error("MMKV instance not initialized!");
  }
  return instance->actualSize();
}
bool HybridMMKV::getIsReadOnly() {
  if (!isInitialized || instance == nullptr) {
    throw std::runtime_error("MMKV instance not initialized!");
  }
  return instance->isReadOnly();
}

// helper: overload pattern matching for lambdas
template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

void HybridMMKV::set(const std::string& key, const std::variant<std::string, double, bool, std::shared_ptr<ArrayBuffer>>& value) {
  // Pattern-match each potential value in std::variant
  std::visit(overloaded{[&](const std::string& string) { instance->set(string, key); }, [&](double number) { instance->set(number, key); },
                        [&](bool b) { instance->set(b, key); },
                        [&](const std::shared_ptr<ArrayBuffer>& buf) {
                          MMBuffer buffer(buf->data(), buf->size(), MMBufferCopyFlag::MMBufferNoCopy);
                          instance->set(std::move(buffer), key);
                        }},
             value);

  // Notify on changed
  MMKVValueChangedListenerRegistry::notifyOnValueChanged(instance->mmapID(), key);
}
std::optional<bool> HybridMMKV::getBoolean(const std::string& key) {
  bool hasValue;
  bool result = instance->getBool(key, /* defaultValue */ false, &hasValue);
  if (hasValue) {
    return result;
  } else {
    return std::nullopt;
  }
}
std::optional<std::string> HybridMMKV::getString(const std::string& key) {
  std::string result;
  bool hasValue = instance->getString(key, result, /* inplaceModification */ true);
  if (hasValue) {
    return result;
  } else {
    return std::nullopt;
  }
}
std::optional<double> HybridMMKV::getNumber(const std::string& key) {
  bool hasValue;
  double result = instance->getDouble(key, /* defaultValue */ 0.0, &hasValue);
  if (hasValue) {
    return result;
  } else {
    return std::nullopt;
  }
}
std::optional<std::shared_ptr<ArrayBuffer>> HybridMMKV::getBuffer(const std::string& key) {
  MMBuffer result;
#ifdef __APPLE__
  // iOS: Convert std::string to NSString* for MMKVCore pod compatibility
  bool hasValue = instance->getBytes(@(key.c_str()), result);
#else
  // Android/other platforms: Use std::string directly (converts to
  // std::string_view)
  bool hasValue = instance->getBytes(key, result);
#endif
  if (hasValue) {
    return std::make_shared<ManagedMMBuffer>(std::move(result));
  } else {
    return std::nullopt;
  }
}
bool HybridMMKV::contains(const std::string& key) {
  return instance->containsKey(key);
}
void HybridMMKV::remove(const std::string& key) {
  instance->removeValueForKey(key);
}
std::vector<std::string> HybridMMKV::getAllKeys() {
  return instance->allKeys();
}
void HybridMMKV::clearAll() {
  instance->clearAll();
}
void HybridMMKV::recrypt(const std::optional<std::string>& key) {
  bool successful = false;
  if (key.has_value()) {
    // Encrypt with the given key
    successful = instance->reKey(key.value());
  } else {
    // Remove the encryption key by setting it to a blank string
    successful = instance->reKey(std::string());
  }
  if (!successful) {
    throw std::runtime_error("Failed to recrypt MMKV instance!");
  }
}
void HybridMMKV::trim() {
  instance->trim();
  instance->clearMemoryCache();
}

Listener HybridMMKV::addOnValueChangedListener(const std::function<void(const std::string& /* key */)>& onValueChanged) {
  // Add listener
  auto mmkvID = instance->mmapID();
  auto listenerID = MMKVValueChangedListenerRegistry::addListener(instance->mmapID(), onValueChanged);

  return Listener([=]() {
    // remove()
    MMKVValueChangedListenerRegistry::removeListener(mmkvID, listenerID);
  });
}

MMKVMode HybridMMKV::getMMKVMode(const Configuration& config) {
  if (!config.mode.has_value()) {
    return ::mmkv::MMKV_SINGLE_PROCESS;
  }
  switch (config.mode.value()) {
    case Mode::SINGLE_PROCESS:
      return ::mmkv::MMKV_SINGLE_PROCESS;
    case Mode::MULTI_PROCESS:
      return ::mmkv::MMKV_MULTI_PROCESS;
    default:
      [[unlikely]] throw std::runtime_error("Invalid MMKV Mode value!");
  }
}

} // namespace margelo::nitro::mmkv
