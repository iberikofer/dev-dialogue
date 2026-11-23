#pragma once

#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <unordered_map>

namespace devdialogue::viewmodels {

// Real-time resource monitor scoped strictly to Dev Dialogue's own process tree
// (DevDialogue.exe + WebView2 children, DevDialogue.Api.exe, and Ollama).
class SystemMonitorViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool open READ isOpen WRITE setOpen NOTIFY openChanged)
    Q_PROPERTY(double totalCpuPercent READ totalCpuPercent NOTIFY metricsUpdated)
    Q_PROPERTY(double totalWorkingSetMb READ totalWorkingSetMb NOTIFY metricsUpdated)
    Q_PROPERTY(double totalPrivateMb READ totalPrivateMb NOTIFY metricsUpdated)
    Q_PROPERTY(double desktopCpuPercent READ desktopCpuPercent NOTIFY metricsUpdated)
    Q_PROPERTY(double desktopWorkingSetMb READ desktopWorkingSetMb NOTIFY metricsUpdated)
    Q_PROPERTY(double desktopPrivateMb READ desktopPrivateMb NOTIFY metricsUpdated)
    Q_PROPERTY(double apiCpuPercent READ apiCpuPercent NOTIFY metricsUpdated)
    Q_PROPERTY(double apiWorkingSetMb READ apiWorkingSetMb NOTIFY metricsUpdated)
    Q_PROPERTY(double apiPrivateMb READ apiPrivateMb NOTIFY metricsUpdated)
    Q_PROPERTY(double ollamaCpuPercent READ ollamaCpuPercent NOTIFY metricsUpdated)
    Q_PROPERTY(double ollamaWorkingSetMb READ ollamaWorkingSetMb NOTIFY metricsUpdated)
    Q_PROPERTY(double ollamaPrivateMb READ ollamaPrivateMb NOTIFY metricsUpdated)
    Q_PROPERTY(int processCount READ processCount NOTIFY metricsUpdated)
    Q_PROPERTY(QVariantList cpuHistory READ cpuHistory NOTIFY metricsUpdated)

public:
    explicit SystemMonitorViewModel(QObject* parent = nullptr);

    bool isOpen() const { return m_open; }
    void setOpen(bool open);
    Q_INVOKABLE void toggle();
    Q_INVOKABLE void refreshNow();

    double totalCpuPercent() const { return m_totalCpuPercent; }
    double totalWorkingSetMb() const { return m_totalWorkingSetMb; }
    double totalPrivateMb() const { return m_totalPrivateMb; }
    double desktopCpuPercent() const { return m_desktopCpuPercent; }
    double desktopWorkingSetMb() const { return m_desktopWorkingSetMb; }
    double desktopPrivateMb() const { return m_desktopPrivateMb; }
    double apiCpuPercent() const { return m_apiCpuPercent; }
    double apiWorkingSetMb() const { return m_apiWorkingSetMb; }
    double apiPrivateMb() const { return m_apiPrivateMb; }
    double ollamaCpuPercent() const { return m_ollamaCpuPercent; }
    double ollamaWorkingSetMb() const { return m_ollamaWorkingSetMb; }
    double ollamaPrivateMb() const { return m_ollamaPrivateMb; }
    int processCount() const { return m_processCount; }
    QVariantList cpuHistory() const { return m_cpuHistory; }

signals:
    void openChanged();
    void metricsUpdated();

private:
    struct CpuSample {
        unsigned long long procTime100Ns{0};
        unsigned long long wallTime100Ns{0};
    };

    void sampleProcesses();

    QTimer m_timer;
    bool m_open{false};
    int m_cpuCores{1};
    std::unordered_map<unsigned long, CpuSample> m_prevCpuSamples;

    double m_totalCpuPercent{0.0};
    double m_totalWorkingSetMb{0.0};
    double m_totalPrivateMb{0.0};
    double m_desktopCpuPercent{0.0};
    double m_desktopWorkingSetMb{0.0};
    double m_desktopPrivateMb{0.0};
    double m_apiCpuPercent{0.0};
    double m_apiWorkingSetMb{0.0};
    double m_apiPrivateMb{0.0};
    double m_ollamaCpuPercent{0.0};
    double m_ollamaWorkingSetMb{0.0};
    double m_ollamaPrivateMb{0.0};
    int m_processCount{0};
    QVariantList m_cpuHistory;
};

} // namespace devdialogue::viewmodels
