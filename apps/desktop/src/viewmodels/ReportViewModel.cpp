#include "viewmodels/ReportViewModel.hpp"

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QFontMetrics>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QPointer>
#include <QStandardPaths>
#include <QUrl>
#include <QVariantMap>
#include <algorithm>
#include <variant>

#include "viewmodels/DemoData.hpp"

namespace devdialogue::viewmodels {

namespace {

// Local demo sessions and the sample interviews never reached the server,
// so there is nothing to fetch.
bool isOfflineSessionId(const QString& interviewId) {
    return interviewId.isEmpty() || interviewId.startsWith(QLatin1String("session-")) ||
           interviewId.startsWith(QLatin1String("mock-"));
}

QStringList toStringList(const std::vector<std::string>& items) {
    QStringList result;
    result.reserve(static_cast<qsizetype>(items.size()));
    for (const std::string& item : items) {
        result.append(QString::fromStdString(item));
    }
    return result;
}

} // namespace

ReportViewModel::ReportViewModel(std::shared_ptr<services::InterviewService> interviewService, QObject* parent)
    : QObject(parent), m_interviewService(std::move(interviewService)) {}

QStringList ReportViewModel::strengths() const {
    return toStringList(m_report.strengths);
}

QStringList ReportViewModel::weaknesses() const {
    return toStringList(m_report.weaknesses);
}

QVariantList ReportViewModel::recommendations() const {
    QVariantList result;
    for (const models::RecommendationItem& item : m_report.recommendations) {
        result.append(QVariantMap{
            {QStringLiteral("topic"), QString::fromStdString(item.topic)},
            {QStringLiteral("resource"), QString::fromStdString(item.resource)},
        });
    }
    return result;
}

QVariantList ReportViewModel::questionScores() const {
    QVariantList result;
    for (const models::QuestionScoreItem& item : m_report.questionScores) {
        const auto getCrit = [&](const std::string& key) {
            const auto it = item.criteria.find(key);
            return it != item.criteria.end() ? it->second : -1;
        };
        const int acc = getCrit("accuracy");
        const int comp = getCrit("completeness");
        const int term = getCrit("terminology");
        const int clar = getCrit("clarity");
        const int vis = getCrit("visual");
        const bool hasCriteria = acc >= 0 && comp >= 0 && term >= 0 && clar >= 0;

        result.append(QVariantMap{
            {QStringLiteral("question"), QString::fromStdString(item.question)},
            {QStringLiteral("score"), item.score},
            {QStringLiteral("comment"), QString::fromStdString(item.comment)},
            {QStringLiteral("responseSeconds"), item.responseSeconds},
            {QStringLiteral("userAnswer"), QString::fromStdString(item.userAnswer)},
            {QStringLiteral("hasCriteria"), hasCriteria},
            {QStringLiteral("accuracy"), acc},
            {QStringLiteral("completeness"), comp},
            {QStringLiteral("terminology"), term},
            {QStringLiteral("clarity"), clar},
            {QStringLiteral("hasVisual"), vis >= 0},
            {QStringLiteral("visual"), vis},
        });
    }
    return result;
}

void ReportViewModel::setUiLanguage(const QString& language) {
    if (m_uiLanguage == language) {
        return;
    }
    m_uiLanguage = language;
    emit uiLanguageChanged();
    if (m_hasReport && m_followsUiLanguage) {
        apply(demo::report(m_interviewId, m_uiLanguage), true);
    }
}

void ReportViewModel::load(const QString& interviewId) {
    const int generation = ++m_generation;
    m_interviewId = interviewId;
    setLoading(true);

    // Sample interviews and interviews finished offline exist only in the
    // demo history.
    if (isOfflineSessionId(interviewId) || demo::finishedInterview(interviewId) != nullptr) {
        applyDemo(interviewId);
        return;
    }
    // Any other interview lives on the server; a stand-in report would show
    // numbers that are not its own.
    if (!m_interviewService || !backendLikelyOnline()) {
        fail(tr("The server can't be reached, so this report can't be loaded."));
        return;
    }

    QPointer<ReportViewModel> self(this);
    m_interviewService->getInterview(
        interviewId, [self, generation](const std::variant<models::InterviewDetail, models::ApiError>& result) {
            if (!self || generation != self->m_generation) {
                return;
            }
            const auto* detail = std::get_if<models::InterviewDetail>(&result);
            if (detail == nullptr) {
                const auto& error = std::get<models::ApiError>(result);
                if (error.isNetworkDown) {
                    self->fail(tr("The server can't be reached, so this report can't be loaded."));
                } else if (!error.message.empty()) {
                    self->fail(tr("The report could not be loaded: %1").arg(QString::fromStdString(error.message)));
                } else {
                    self->fail(tr("The report could not be loaded."));
                }
                return;
            }
            self->setContext(*detail);
            self->m_followsUiLanguage = false;
            if (detail->feedback.has_value()) {
                self->apply(*detail->feedback, false);
            } else {
                // Not evaluated yet: score what the transcript holds.
                self->apply(demo::reportFromTranscript(support::enumToString(detail->direction), detail->transcript,
                                                       support::enumToString(detail->language)),
                            true);
            }
        });
}

void ReportViewModel::applyDemo(const QString& interviewId) {
    models::InterviewSummary context;
    for (const models::InterviewSummary& item : demo::interviews()) {
        if (QString::fromStdString(item.id) == interviewId) {
            context = item;
            break;
        }
    }
    setContext(context);
    // Recorded interviews are reported in their own language.
    m_followsUiLanguage = demo::finishedInterview(interviewId) == nullptr;
    apply(demo::report(interviewId, m_uiLanguage), true);
}

void ReportViewModel::setContext(const models::InterviewSummary& interview) {
    if (interview.id.empty()) {
        m_direction.clear();
        m_level.clear();
        m_dateText.clear();
        return;
    }
    m_direction = support::enumToString(interview.direction);
    m_level = support::enumToString(interview.level);
    m_dateText = support::formatDateTime(interview.createdAt);
}

void ReportViewModel::setLoading(bool value) {
    if (m_loading != value) {
        m_loading = value;
        emit loadingChanged();
    }
}

void ReportViewModel::apply(const models::InterviewFeedbackReport& report, bool isDemo) {
    m_report = report;
    m_isDemoData = isDemo;
    m_hasReport = true;
    m_errorMessage.clear();
    emit reportChanged();
    setLoading(false);
}

void ReportViewModel::fail(const QString& message) {
    m_report = {};
    m_isDemoData = false;
    m_hasReport = false;
    m_followsUiLanguage = false;
    m_errorMessage = message;
    setContext({});
    emit reportChanged();
    setLoading(false);
}

namespace {

QString formatDurationText(int seconds) {
    if (seconds <= 0) {
        return QStringLiteral("—");
    }
    if (seconds < 60) {
        return QStringLiteral("%1s").arg(seconds);
    }
    const int mins = seconds / 60;
    const int rem = seconds % 60;
    return rem == 0 ? QStringLiteral("%1m").arg(mins) : QStringLiteral("%1m %2s").arg(mins).arg(rem);
}

QColor scoreTierColor(int score) {
    if (score >= 80) return QColor(QStringLiteral("#10B981"));
    if (score >= 60) return QColor(QStringLiteral("#3B82F6"));
    if (score >= 35) return QColor(QStringLiteral("#F59E0B"));
    return QColor(QStringLiteral("#EF4444"));
}

int measureWrappedHeight(QPainter& painter, const QFont& font, const QString& text, int width) {
    painter.setFont(font);
    const QRect bounds = painter.boundingRect(QRect(0, 0, width, 10000), Qt::TextWordWrap, text);
    return bounds.height();
}

} // namespace

QString ReportViewModel::exportPdf(const QString& targetPath) {
    if (!m_hasReport) {
        m_exportError = tr("No report loaded to export.");
        emit exportChanged();
        emit pdfExportFailed(m_exportError);
        return {};
    }

    QString filePath = targetPath.trimmed();
    if (filePath.isEmpty()) {
        QString dir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        if (dir.isEmpty()) {
            dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        }
        if (dir.isEmpty()) {
            dir = QDir::homePath();
        }
        QDir().mkpath(dir);
        const QString dirTag = m_direction.isEmpty() ? QStringLiteral("interview") : m_direction;
        const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss"));
        filePath = dir + QStringLiteral("/DevDialogue_Report_%1_%2.pdf").arg(dirTag, stamp);
    } else {
        QDir().mkpath(QFileInfo(filePath).absolutePath());
    }

    QPdfWriter writer(filePath);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(300);
    writer.setPageMargins(QMarginsF(14, 14, 14, 14), QPageLayout::Millimeter);
    writer.setTitle(QStringLiteral("Dev Dialogue Interview Report"));
    writer.setCreator(QStringLiteral("Dev Dialogue Desktop"));

    QPainter painter(&writer);
    if (!painter.isActive()) {
        m_exportError = tr("Could not create PDF file at %1").arg(filePath);
        emit exportChanged();
        emit pdfExportFailed(m_exportError);
        return {};
    }

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const int pageW = painter.viewport().width();
    const int pageH = painter.viewport().height();
    const int marginX = 70;
    const int contentW = pageW - marginX * 2;
    const int bottomLimit = pageH - 130;
    int y = 50;
    int pageNum = 1;

    const QFont titleFont(QStringLiteral("Segoe UI"), 20, QFont::Bold);
    const QFont h2Font(QStringLiteral("Segoe UI"), 12, QFont::Bold);
    const QFont bodyFont(QStringLiteral("Segoe UI"), 9.5);
    const QFont boldBodyFont(QStringLiteral("Segoe UI"), 9.5, QFont::DemiBold);
    const QFont smallFont(QStringLiteral("Segoe UI"), 8.5);
    const QFont monoBoldFont(QStringLiteral("Consolas"), 10, QFont::Bold);

    const auto drawFooter = [&]() {
        painter.setPen(QColor(QStringLiteral("#94A3B8")));
        painter.setFont(smallFont);
        painter.drawText(QRect(marginX, pageH - 85, contentW / 2, 40), Qt::AlignLeft | Qt::AlignVCenter,
                         QStringLiteral("Generated by Dev Dialogue · AI Interviewer Vladyslav"));
        painter.drawText(QRect(marginX + contentW / 2, pageH - 85, contentW / 2, 40), Qt::AlignRight | Qt::AlignVCenter,
                         QStringLiteral("Page %1").arg(pageNum));
    };

    const auto ensureSpace = [&](int neededHeight) {
        if (y + neededHeight > bottomLimit) {
            drawFooter();
            writer.newPage();
            ++pageNum;
            y = 60;
        }
    };

    // 1. Dark Executive Header Banner
    const int headerH = 290;
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(QStringLiteral("#0F172A")));
    painter.drawRoundedRect(QRect(marginX, y, contentW, headerH), 28, 28);

    // Top accent line
    painter.setBrush(QColor(QStringLiteral("#3B82F6")));
    painter.drawRoundedRect(QRect(marginX, y, contentW, 14), 6, 6);

    painter.setPen(QColor(QStringLiteral("#F8FAFC")));
    painter.setFont(titleFont);
    painter.drawText(QRect(marginX + 64, y + 48, contentW - 360, 95), Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("Dev Dialogue Report"));

    const QString trackLabel = m_direction.isEmpty() ? QStringLiteral("Technical") : m_direction.toUpper();
    const QString levelLabel = m_level.isEmpty() ? QString()
        : (m_level.at(0).toUpper() + m_level.mid(1));
    const QString subtitleText = QStringList({trackLabel, levelLabel, m_dateText}).join(QStringLiteral("  ·  "));
    painter.setPen(QColor(QStringLiteral("#94A3B8")));
    painter.setFont(bodyFont);
    painter.drawText(QRect(marginX + 64, y + 150, contentW - 360, 70), Qt::AlignLeft | Qt::AlignVCenter,
                     subtitleText);

    // Score Badge on right of header
    const QColor tierCol = scoreTierColor(m_report.score);
    const QRect scoreBox(marginX + contentW - 260, y + 52, 200, 184);
    painter.setPen(QPen(tierCol, 5));
    painter.setBrush(QColor(QStringLiteral("#1E293B")));
    painter.drawRoundedRect(scoreBox, 24, 24);
    painter.setPen(tierCol);
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 22, QFont::Bold));
    painter.drawText(QRect(scoreBox.x(), scoreBox.y() + 18, scoreBox.width(), 105),
                     Qt::AlignHCenter | Qt::AlignVCenter, QString::number(m_report.score));
    painter.setPen(QColor(QStringLiteral("#94A3B8")));
    painter.setFont(smallFont);
    painter.drawText(QRect(scoreBox.x(), scoreBox.y() + 120, scoreBox.width(), 44),
                     Qt::AlignHCenter | Qt::AlignVCenter, QStringLiteral("/ 100 PTS"));

    y += headerH + 46;

    // 2. KPI Strip (3 cards: Questions, Avg Response Time, Total Duration)
    const int kpiH = 148;
    const int kpiGap = 28;
    const int kpiW = (contentW - kpiGap * 2) / 3;
    const struct KpiEntry { QString label; QString value; } kpis[3] = {
        {tr("QUESTIONS"), QString::number(static_cast<int>(m_report.questionScores.size()))},
        {tr("AVG RESPONSE"), formatDurationText(m_report.averageResponseSeconds)},
        {tr("SESSION DURATION"), formatDurationText(m_report.totalDurationSeconds)}
    };
    for (int i = 0; i < 3; ++i) {
        const QRect box(marginX + i * (kpiW + kpiGap), y, kpiW, kpiH);
        painter.setPen(QPen(QColor(QStringLiteral("#E2E8F0")), 3));
        painter.setBrush(QColor(QStringLiteral("#F8FAFC")));
        painter.drawRoundedRect(box, 18, 18);
        painter.setPen(QColor(QStringLiteral("#64748B")));
        painter.setFont(smallFont);
        painter.drawText(QRect(box.x() + 28, box.y() + 22, box.width() - 56, 38),
                         Qt::AlignLeft | Qt::AlignVCenter, kpis[i].label);
        painter.setPen(QColor(QStringLiteral("#0F172A")));
        painter.setFont(QFont(QStringLiteral("Segoe UI"), 14, QFont::Bold));
        painter.drawText(QRect(box.x() + 28, box.y() + 66, box.width() - 56, 56),
                         Qt::AlignLeft | Qt::AlignVCenter, kpis[i].value);
    }
    y += kpiH + 44;

    // 3. Summary & Timing Analysis Card
    const QString summaryTxt = summary();
    const QString timingTxt = timingAnalysis();
    const int sumH = measureWrappedHeight(painter, bodyFont, summaryTxt, contentW - 88);
    const int timH = timingTxt.isEmpty() ? 0 : measureWrappedHeight(painter, bodyFont, timingTxt, contentW - 88);
    const int overviewBoxH = 110 + sumH + (timH > 0 ? 75 + timH : 20);
    ensureSpace(overviewBoxH);

    painter.setPen(QPen(QColor(QStringLiteral("#CBD5E1")), 3));
    painter.setBrush(QColor(QStringLiteral("#FFFFFF")));
    painter.drawRoundedRect(QRect(marginX, y, contentW, overviewBoxH), 20, 20);
    painter.setPen(QColor(QStringLiteral("#0F172A")));
    painter.setFont(h2Font);
    painter.drawText(QRect(marginX + 44, y + 28, contentW - 88, 52), Qt::AlignLeft | Qt::AlignVCenter,
                     tr("AI Assessment Summary"));
    painter.setPen(QColor(QStringLiteral("#334155")));
    painter.setFont(bodyFont);
    painter.drawText(QRect(marginX + 44, y + 88, contentW - 88, sumH + 12), Qt::TextWordWrap, summaryTxt);

    if (timH > 0) {
        const int ty = y + 108 + sumH;
        painter.setPen(QColor(QStringLiteral("#2563EB")));
        painter.setFont(boldBodyFont);
        painter.drawText(QRect(marginX + 44, ty, contentW - 88, 42), Qt::AlignLeft | Qt::AlignVCenter,
                         tr("Response Pacing & Timing"));
        painter.setPen(QColor(QStringLiteral("#475569")));
        painter.setFont(bodyFont);
        painter.drawText(QRect(marginX + 44, ty + 46, contentW - 88, timH + 12), Qt::TextWordWrap, timingTxt);
    }
    y += overviewBoxH + 40;

    // 4. Strengths & Areas to Improve
    const auto drawListSection = [&](const QString& heading, const QStringList& items, const QColor& accent,
                                     const QColor& bg) {
        if (items.isEmpty()) {
            return;
        }
        int totalTextH = 0;
        for (const QString& item : items) {
            totalTextH += measureWrappedHeight(painter, bodyFont, QStringLiteral("•  ") + item, contentW - 104) + 18;
        }
        const int boxH = 104 + totalTextH;
        ensureSpace(boxH);

        painter.setPen(QPen(accent.lighter(150), 3));
        painter.setBrush(bg);
        painter.drawRoundedRect(QRect(marginX, y, contentW, boxH), 20, 20);
        painter.setPen(Qt::NoPen);
        painter.setBrush(accent);
        painter.drawRoundedRect(QRect(marginX, y + 22, 12, boxH - 44), 6, 6);

        painter.setPen(accent.darker(120));
        painter.setFont(h2Font);
        painter.drawText(QRect(marginX + 48, y + 26, contentW - 96, 52), Qt::AlignLeft | Qt::AlignVCenter, heading);

        int iy = y + 86;
        painter.setPen(QColor(QStringLiteral("#1E293B")));
        painter.setFont(bodyFont);
        for (const QString& item : items) {
            const QString bullet = QStringLiteral("•  ") + item;
            const int ih = measureWrappedHeight(painter, bodyFont, bullet, contentW - 104);
            painter.drawText(QRect(marginX + 48, iy, contentW - 104, ih + 10), Qt::TextWordWrap, bullet);
            iy += ih + 18;
        }
        y += boxH + 34;
    };

    drawListSection(tr("Strengths"), strengths(), QColor(QStringLiteral("#059669")), QColor(QStringLiteral("#ECFDF5")));
    drawListSection(tr("Areas to Improve"), weaknesses(), QColor(QStringLiteral("#D97706")), QColor(QStringLiteral("#FFFBEB")));

    // 5. Question-by-Question Breakdown
    if (!m_report.questionScores.empty()) {
        ensureSpace(120);
        painter.setPen(QColor(QStringLiteral("#0F172A")));
        painter.setFont(QFont(QStringLiteral("Segoe UI"), 14, QFont::Bold));
        painter.drawText(QRect(marginX, y, contentW, 60), Qt::AlignLeft | Qt::AlignVCenter, tr("Question Breakdown"));
        y += 74;

        int index = 1;
        for (const models::QuestionScoreItem& item : m_report.questionScores) {
            const QString qTitle = QStringLiteral("%1. %2").arg(index++).arg(QString::fromStdString(item.question));
            const QString ansText = item.userAnswer.empty()
                ? QString()
                : tr("Your answer: \"%1\"").arg(QString::fromStdString(item.userAnswer));
            const QString cmtText = QString::fromStdString(item.comment);

            QString critLine;
            if (!item.criteria.empty()) {
                const auto getC = [&](const char* k) {
                    const auto it = item.criteria.find(k);
                    return it != item.criteria.end() ? it->second : -1;
                };
                const int a = getC("accuracy"), c = getC("completeness"), t = getC("terminology"), l = getC("clarity");
                if (a >= 0 && c >= 0 && t >= 0 && l >= 0) {
                    critLine = QStringLiteral("Accuracy %1  ·  Completeness %2  ·  Terminology %3  ·  Clarity %4")
                        .arg(a).arg(c).arg(t).arg(l);
                }
            }

            const int qH = measureWrappedHeight(painter, boldBodyFont, qTitle, contentW - 340);
            const int aH = ansText.isEmpty() ? 0 : measureWrappedHeight(painter, smallFont, ansText, contentW - 88) + 16;
            const int crH = critLine.isEmpty() ? 0 : measureWrappedHeight(painter, smallFont, critLine, contentW - 88) + 16;
            const int cmH = measureWrappedHeight(painter, bodyFont, cmtText, contentW - 88);
            const int cardH = std::max(140, 74 + qH + aH + crH + cmH + 40);

            ensureSpace(cardH + 24);

            painter.setPen(QPen(QColor(QStringLiteral("#E2E8F0")), 3));
            painter.setBrush(QColor(QStringLiteral("#FFFFFF")));
            painter.drawRoundedRect(QRect(marginX, y, contentW, cardH), 18, 18);

            // Question text
            painter.setPen(QColor(QStringLiteral("#0F172A")));
            painter.setFont(boldBodyFont);
            painter.drawText(QRect(marginX + 40, y + 26, contentW - 340, qH + 12), Qt::TextWordWrap, qTitle);

            // Time pill & Score pill
            if (item.responseSeconds > 0) {
                const QRect tPill(marginX + contentW - 285, y + 24, 110, 48);
                painter.setPen(QPen(QColor(QStringLiteral("#CBD5E1")), 2));
                painter.setBrush(QColor(QStringLiteral("#F1F5F9")));
                painter.drawRoundedRect(tPill, 12, 12);
                painter.setPen(QColor(QStringLiteral("#475569")));
                painter.setFont(smallFont);
                painter.drawText(tPill, Qt::AlignCenter, formatDurationText(item.responseSeconds));
            }

            const QColor qCol = scoreTierColor(item.score);
            const QRect sPill(marginX + contentW - 160, y + 24, 120, 48);
            painter.setPen(QPen(qCol, 2));
            painter.setBrush(qCol.lighter(185));
            painter.drawRoundedRect(sPill, 12, 12);
            painter.setPen(qCol.darker(130));
            painter.setFont(monoBoldFont);
            painter.drawText(sPill, Qt::AlignCenter, QStringLiteral("%1/100").arg(item.score));

            int cy = y + 48 + std::max(qH, 42);
            if (!critLine.isEmpty()) {
                painter.setPen(QColor(QStringLiteral("#2563EB")));
                painter.setFont(smallFont);
                painter.drawText(QRect(marginX + 40, cy, contentW - 88, crH), Qt::TextWordWrap, critLine);
                cy += crH;
            }
            if (!ansText.isEmpty()) {
                painter.setPen(QColor(QStringLiteral("#64748B")));
                painter.setFont(smallFont);
                painter.drawText(QRect(marginX + 40, cy, contentW - 88, aH), Qt::TextWordWrap, ansText);
                cy += aH;
            }
            painter.setPen(QColor(QStringLiteral("#334155")));
            painter.setFont(bodyFont);
            painter.drawText(QRect(marginX + 40, cy, contentW - 88, cmH + 12), Qt::TextWordWrap, cmtText);

            y += cardH + 26;
        }
    }

    // 6. Recommendations
    if (!m_report.recommendations.empty()) {
        int recH = 96;
        for (const models::RecommendationItem& r : m_report.recommendations) {
            recH += measureWrappedHeight(painter, boldBodyFont, QString::fromStdString(r.topic), contentW - 96) + 52;
        }
        ensureSpace(recH);

        painter.setPen(QPen(QColor(QStringLiteral("#BFDBFE")), 3));
        painter.setBrush(QColor(QStringLiteral("#EFF6FF")));
        painter.drawRoundedRect(QRect(marginX, y, contentW, recH), 20, 20);
        painter.setPen(QColor(QStringLiteral("#1D4ED8")));
        painter.setFont(h2Font);
        painter.drawText(QRect(marginX + 44, y + 26, contentW - 88, 50), Qt::AlignLeft | Qt::AlignVCenter,
                         tr("What to Study Next"));

        int ry = y + 84;
        for (const models::RecommendationItem& r : m_report.recommendations) {
            const QString t = QString::fromStdString(r.topic);
            const QString u = QString::fromStdString(r.resource);
            const int th = measureWrappedHeight(painter, boldBodyFont, t, contentW - 96);
            painter.setPen(QColor(QStringLiteral("#0F172A")));
            painter.setFont(boldBodyFont);
            painter.drawText(QRect(marginX + 44, ry, contentW - 96, th + 6), Qt::TextWordWrap, t);
            ry += th + 4;
            painter.setPen(QColor(QStringLiteral("#2563EB")));
            painter.setFont(smallFont);
            painter.drawText(QRect(marginX + 44, ry, contentW - 96, 38), Qt::AlignLeft | Qt::AlignVCenter, u);
            ry += 44;
        }
    }

    drawFooter();
    painter.end();

    m_lastExportPath = filePath;
    m_exportError.clear();
    emit exportChanged();
    emit pdfExported(filePath);
    return filePath;
}

void ReportViewModel::openLastPdf() const {
    if (!m_lastExportPath.isEmpty()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_lastExportPath));
    }
}

} // namespace devdialogue::viewmodels
