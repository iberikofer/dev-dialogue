#include "viewmodels/SystemMonitorViewModel.hpp"

#include <QCoreApplication>
#include <algorithm>
#include <cmath>
#include <unordered_set>
#include <vector>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#endif

namespace devdialogue::viewmodels {

namespace {

constexpr int MaxHistorySamples = 28;

#ifdef Q_OS_WIN
unsigned long long fileTimeTo100Ns(const FILETIME& ft) {
    ULARGE_INTEGER u;
    u.LowPart = ft.dwLowDateTime;
    u.HighPart = ft.dwHighDateTime;
    return u.QuadPart;
}
#endif

} // namespace

SystemMonitorViewModel::SystemMonitorViewModel(QObject* parent)
    : QObject(parent) {
#ifdef Q_OS_WIN
    SYSTEM_INFO sysInfo{};
    GetSystemInfo(&sysInfo);
    m_cpuCores = std::max(1, static_cast<int>(sysInfo.dwNumberOfProcessors));
#endif
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &SystemMonitorViewModel::sampleProcesses);
}

void SystemMonitorViewModel::setOpen(bool open) {
    if (m_open == open) {
        return;
    }
    m_open = open;
    emit openChanged();
    if (m_open) {
        sampleProcesses();
        m_timer.start();
    } else {
        m_timer.stop();
    }
}

void SystemMonitorViewModel::toggle() {
    setOpen(!m_open);
}

void SystemMonitorViewModel::refreshNow() {
    sampleProcesses();
}

void SystemMonitorViewModel::sampleProcesses() {
#ifdef Q_OS_WIN
    const DWORD selfPid = GetCurrentProcessId();
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) {
        return;
    }

    struct ProcEntry {
        DWORD pid{0};
        DWORD ppid{0};
        QString exeLower;
    };
    std::vector<ProcEntry> allProcs;
    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe)) {
        do {
            allProcs.push_back(ProcEntry{
                pe.th32ProcessID,
                pe.th32ParentProcessID,
                QString::fromWCharArray(pe.szExeFile).toLower()
            });
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);

    // Collect our desktop PID + all WebView2 descendant processes spawned by DevDialogue.exe
    std::unordered_set<DWORD> desktopTreePids{selfPid};
    bool added = true;
    while (added) {
        added = false;
        for (const auto& p : allProcs) {
            if (desktopTreePids.count(p.pid) == 0 &&
                desktopTreePids.count(p.ppid) > 0 &&
                p.exeLower.contains(QStringLiteral("webview2"))) {
                desktopTreePids.insert(p.pid);
                added = true;
            }
        }
    }

    FILETIME nowFt{};
    GetSystemTimeAsFileTime(&nowFt);
    const unsigned long long now100Ns = fileTimeTo100Ns(nowFt);

    double dCpu = 0.0, dWs = 0.0, dPriv = 0.0;
    double aCpu = 0.0, aWs = 0.0, aPriv = 0.0;
    double oCpu = 0.0, oWs = 0.0, oPriv = 0.0;
    int count = 0;

    std::unordered_map<unsigned long, CpuSample> nextSamples;

    for (const auto& p : allProcs) {
        enum class Bucket { None, Desktop, Api, Ollama };
        Bucket bucket = Bucket::None;

        if (desktopTreePids.count(p.pid) > 0 || p.exeLower == QLatin1String("devdialogue.exe")) {
            bucket = Bucket::Desktop;
        } else if (p.exeLower == QLatin1String("devdialogue.api.exe")) {
            bucket = Bucket::Api;
        } else if (p.exeLower == QLatin1String("ollama.exe") ||
                   p.exeLower == QLatin1String("ollama_llama_server.exe") ||
                   p.exeLower.startsWith(QLatin1String("ollama"))) {
            bucket = Bucket::Ollama;
        }

        if (bucket == Bucket::None) {
            continue;
        }

        HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, p.pid);
        if (!hProc) {
            hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, p.pid);
        }
        if (!hProc) {
            continue;
        }

        ++count;

        double wsMb = 0.0;
        double privMb = 0.0;
        PROCESS_MEMORY_COUNTERS_EX pmc{};
        pmc.cb = sizeof(pmc);
        if (GetProcessMemoryInfo(hProc, reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc))) {
            wsMb = static_cast<double>(pmc.WorkingSetSize) / (1024.0 * 1024.0);
            privMb = static_cast<double>(pmc.PrivateUsage) / (1024.0 * 1024.0);
        }

        double cpuPct = 0.0;
        FILETIME createFt{}, exitFt{}, kernelFt{}, userFt{};
        if (GetProcessTimes(hProc, &createFt, &exitFt, &kernelFt, &userFt)) {
            const unsigned long long proc100Ns = fileTimeTo100Ns(kernelFt) + fileTimeTo100Ns(userFt);
            nextSamples[p.pid] = CpuSample{proc100Ns, now100Ns};

            const auto it = m_prevCpuSamples.find(p.pid);
            if (it != m_prevCpuSamples.end() && now100Ns > it->second.wallTime100Ns) {
                const double dProc = static_cast<double>(proc100Ns - it->second.procTime100Ns);
                const double dWall = static_cast<double>(now100Ns - it->second.wallTime100Ns);
                cpuPct = std::clamp((dProc / (dWall * m_cpuCores)) * 100.0, 0.0, 100.0);
            }
        }

        CloseHandle(hProc);

        switch (bucket) {
        case Bucket::Desktop:
            dCpu += cpuPct;
            dWs += wsMb;
            dPriv += privMb;
            break;
        case Bucket::Api:
            aCpu += cpuPct;
            aWs += wsMb;
            aPriv += privMb;
            break;
        case Bucket::Ollama:
            oCpu += cpuPct;
            oWs += wsMb;
            oPriv += privMb;
            break;
        default:
            break;
        }
    }

    m_prevCpuSamples = std::move(nextSamples);

    auto round1 = [](double v) { return std::round(v * 10.0) / 10.0; };
    m_desktopCpuPercent = round1(dCpu);
    m_desktopWorkingSetMb = round1(dWs);
    m_desktopPrivateMb = round1(dPriv);
    m_apiCpuPercent = round1(aCpu);
    m_apiWorkingSetMb = round1(aWs);
    m_apiPrivateMb = round1(aPriv);
    m_ollamaCpuPercent = round1(oCpu);
    m_ollamaWorkingSetMb = round1(oWs);
    m_ollamaPrivateMb = round1(oPriv);

    m_totalCpuPercent = round1(std::clamp(dCpu + aCpu + oCpu, 0.0, 100.0));
    m_totalWorkingSetMb = round1(dWs + aWs + oWs);
    m_totalPrivateMb = round1(dPriv + aPriv + oPriv);
    m_processCount = count;

    m_cpuHistory.append(m_totalCpuPercent);
    while (m_cpuHistory.size() > MaxHistorySamples) {
        m_cpuHistory.removeFirst();
    }

    emit metricsUpdated();
#endif
}

} // namespace devdialogue::viewmodels
