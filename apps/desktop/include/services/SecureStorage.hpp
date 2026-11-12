#pragma once

#include <optional>
#include <string>

namespace devdialogue::services {

/// Stores small secrets (JWT tokens, cached profile JSON) in the native OS
/// credential vault. Windows uses the Credential Manager; other platforms fall
/// back to an in-process store so the app still builds and runs.
class SecureStorage {
public:
  SecureStorage() = default;
  ~SecureStorage() = default;

  bool writeCredential(const std::string &key, const std::string &secret);
  std::optional<std::string> readCredential(const std::string &key);
  bool deleteCredential(const std::string &key);

private:
#if defined(_WIN32)
  static std::wstring toWide(const std::string &str);
  static std::string toNarrow(const std::wstring &wstr);
#endif
};

} // namespace devdialogue::services
