#include "services/SecureStorage.hpp"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <wincred.h>
#include <string>
#else
#include <map>
#include <mutex>
#endif

namespace devdialogue::services {

#if defined(_WIN32)

namespace {
constexpr wchar_t kTargetPrefix[] = L"DevDialogue_";
}

std::wstring SecureStorage::toWide(const std::string &str) {
  if (str.empty())
    return {};
  const int sizeNeeded = MultiByteToWideChar(
      CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), nullptr, 0);
  std::wstring wstr(static_cast<std::size_t>(sizeNeeded), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()),
                      wstr.data(), sizeNeeded);
  return wstr;
}

std::string SecureStorage::toNarrow(const std::wstring &wstr) {
  if (wstr.empty())
    return {};
  const int sizeNeeded = WideCharToMultiByte(
      CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), nullptr, 0,
      nullptr, nullptr);
  std::string str(static_cast<std::size_t>(sizeNeeded), '\0');
  WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()),
                      str.data(), sizeNeeded, nullptr, nullptr);
  return str;
}

bool SecureStorage::writeCredential(const std::string &key,
                                    const std::string &secret) {
  std::wstring targetName = kTargetPrefix + toWide(key);

  CREDENTIALW cred{};
  cred.Type = CRED_TYPE_GENERIC;
  cred.TargetName = targetName.data();
  cred.CredentialBlobSize = static_cast<DWORD>(secret.size());
  cred.CredentialBlob =
      secret.empty()
          ? nullptr
          : reinterpret_cast<LPBYTE>(const_cast<char *>(secret.data()));
  cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
  cred.UserName = const_cast<LPWSTR>(L"DevDialogue");

  return CredWriteW(&cred, 0) == TRUE;
}

std::optional<std::string> SecureStorage::readCredential(
    const std::string &key) {
  std::wstring targetName = kTargetPrefix + toWide(key);

  PCREDENTIALW pCred = nullptr;
  if (CredReadW(targetName.c_str(), CRED_TYPE_GENERIC, 0, &pCred) != TRUE)
    return std::nullopt;

  std::string secret(
      reinterpret_cast<const char *>(pCred->CredentialBlob),
      static_cast<std::size_t>(pCred->CredentialBlobSize));
  CredFree(pCred);
  return secret;
}

bool SecureStorage::deleteCredential(const std::string &key) {
  std::wstring targetName = kTargetPrefix + toWide(key);
  return CredDeleteW(targetName.c_str(), CRED_TYPE_GENERIC, 0) == TRUE;
}

#else

namespace {
std::map<std::string, std::string> &store() {
  static std::map<std::string, std::string> instance;
  return instance;
}
std::mutex &storeMutex() {
  static std::mutex instance;
  return instance;
}
} // namespace

bool SecureStorage::writeCredential(const std::string &key,
                                    const std::string &secret) {
  std::lock_guard<std::mutex> lock(storeMutex());
  store()[key] = secret;
  return true;
}

std::optional<std::string> SecureStorage::readCredential(
    const std::string &key) {
  std::lock_guard<std::mutex> lock(storeMutex());
  auto it = store().find(key);
  if (it == store().end())
    return std::nullopt;
  return it->second;
}

bool SecureStorage::deleteCredential(const std::string &key) {
  std::lock_guard<std::mutex> lock(storeMutex());
  return store().erase(key) > 0;
}

#endif

} // namespace devdialogue::services
