#include "services/QtKeychainSecureStorage.hpp"
#include "core/Constants.hpp"
#include <QDebug>
#include <QEventLoop>
#include <QString>

#if __has_include(<qt6keychain/keychain.h>)
#include <qt6keychain/keychain.h>
#define HAS_QTKEYCHAIN 1
#elif __has_include(<keychain.h>)
#include <keychain.h>
#define HAS_QTKEYCHAIN 1
#endif

namespace devdialogue::services {

QtKeychainSecureStorage::QtKeychainSecureStorage(std::string serviceName)
    : m_serviceName(std::move(serviceName)) {
#ifndef HAS_QTKEYCHAIN
#ifndef NDEBUG
    qWarning("QtKeychain is not compiled. Using secure in-memory credential storage.");
#else
    qWarning("Release mode: QtKeychain provider not available at compile-time. Using in-memory storage (zero plaintext files).");
#endif
#endif
}

bool QtKeychainSecureStorage::writeCredential(std::string_view key, std::string_view secret) {
    std::string keyStr(key);
    std::string secretStr(secret);

    std::lock_guard<std::mutex> lock(m_mutex);
    m_memoryCache[keyStr] = secretStr;

#ifdef HAS_QTKEYCHAIN
    QKeychain::WritePasswordJob job(QString::fromStdString(m_serviceName));
    job.setAutoDelete(false);
    job.setKey(QString::fromStdString(keyStr));
    job.setTextData(QString::fromStdString(secretStr));

    QEventLoop loop;
    QObject::connect(&job, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
    job.start();
    loop.exec();

    if (job.error()) {
        qWarning() << "QtKeychain writeCredential error for key" << QString::fromStdString(keyStr) << ":" << job.errorString();
        return false;
    }
    return true;
#else
    return true;
#endif
}

std::optional<std::string> QtKeychainSecureStorage::readCredential(std::string_view key) {
    std::string keyStr(key);

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_memoryCache.find(keyStr);
    if (it != m_memoryCache.end()) {
        return it->second;
    }

#ifdef HAS_QTKEYCHAIN
    QKeychain::ReadPasswordJob job(QString::fromStdString(m_serviceName));
    job.setAutoDelete(false);
    job.setKey(QString::fromStdString(keyStr));

    QEventLoop loop;
    QObject::connect(&job, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
    job.start();
    loop.exec();

    if (job.error()) {
        return std::nullopt;
    }

    std::string secret = job.textData().toStdString();
    m_memoryCache[keyStr] = secret;
    return secret;
#else
    return std::nullopt;
#endif
}

bool QtKeychainSecureStorage::deleteCredential(std::string_view key) {
    std::string keyStr(key);

    std::lock_guard<std::mutex> lock(m_mutex);
    m_memoryCache.erase(keyStr);

#ifdef HAS_QTKEYCHAIN
    QKeychain::DeletePasswordJob job(QString::fromStdString(m_serviceName));
    job.setAutoDelete(false);
    job.setKey(QString::fromStdString(keyStr));

    QEventLoop loop;
    QObject::connect(&job, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
    job.start();
    loop.exec();

    return job.error() == QKeychain::NoError;
#else
    return true;
#endif
}

void QtKeychainSecureStorage::clearAll() {
    deleteCredential(core::constants::StorageKeyAccessToken);
    deleteCredential(core::constants::StorageKeyRefreshToken);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_memoryCache.clear();
}

} // namespace devdialogue::services
