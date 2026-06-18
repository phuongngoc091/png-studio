#include "LogViewService.h"
#include "core/PathUtils.h"
#include "core/Logger.h"
#include <QThreadPool>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QPointer>
#include <QCoreApplication>

namespace LAStudio {

LogViewService::LogViewService(QObject *parent)
    : QObject(parent)
{
}

void LogViewService::requestLoadLogs()
{
    if (m_loading) return;
    m_loading = true;
    emit loadingChanged();

    QPointer<LogViewService> weakThis(this);
    QThreadPool::globalInstance()->start([weakThis]() {
        QString logPath = PathUtils::logsDir() + QStringLiteral("/app.log");
        QFile file(logPath);
        QString rawContent;
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            in.setEncoding(QStringConverter::Utf8);
            rawContent = in.readAll();
            file.close();
        } else {
            rawContent = QStringLiteral("Failed to open app.log");
        }

        QStringList lines = rawContent.split(QLatin1Char('\n'));
        QStringList formattedLines;
        static const QRegularExpression regex(
            QStringLiteral(R"(^\[([\d\-:\.\s]+)\]\s+\[([A-Z]{3})\]\s+\[(T:0x[0-9a-fA-F]+)\]\s+\[([^\]]+)\](?:\s+\[([^\]]+)\])?\s+(.*)$)")
        );

        for (const QString &line : lines) {
            QString trimmed = line.trimmed();
            if (trimmed.isEmpty()) continue;
            QString escaped = trimmed.toHtmlEscaped();
            auto match = regex.match(escaped);
            if (match.hasMatch()) {
                QString ts = match.captured(1);
                QString level = match.captured(2);
                QString thread = match.captured(3);
                QString category = match.captured(4);
                QString location = match.captured(5);
                QString msg = match.captured(6);
                QString levelColor = QStringLiteral("#abb2bf");
                bool isBold = false;
                if (level == QLatin1String("DBG")) levelColor = QStringLiteral("#5c6370");
                else if (level == QLatin1String("INF")) levelColor = QStringLiteral("#98c379");
                else if (level == QLatin1String("WRN")) levelColor = QStringLiteral("#d19a66");
                else if (level == QLatin1String("ERR") || level == QLatin1String("FTL")) {
                    levelColor = QStringLiteral("#e06c75");
                    isBold = true;
                }
                QString formatted = QStringLiteral("<font color='#5c6370'>[%1]</font> "
                                                   "<font color='%2'><b>[%3]</b></font> "
                                                   "<font color='#c678dd'>[%4]</font> "
                                                   "<font color='#61afef'>[%5]</font> ").arg(ts, levelColor, level, thread, category);
                if (!location.isEmpty()) formatted += QStringLiteral("<font color='#e5c07b'>[%1]</font> ").arg(location);
                formatted += QStringLiteral("<font color='%1'>%2%3%4</font>")
                                 .arg(isBold ? levelColor : QStringLiteral("#abb2bf"), isBold ? QStringLiteral("<b>") : QString(), msg, isBold ? QStringLiteral("</b>") : QString());
                formattedLines.append(formatted);
            } else {
                QString fallbackColor = QStringLiteral("#abb2bf");
                if (escaped.contains(QLatin1String("[DBG]"))) fallbackColor = QStringLiteral("#5c6370");
                else if (escaped.contains(QLatin1String("[WRN]"))) fallbackColor = QStringLiteral("#d19a66");
                else if (escaped.contains(QLatin1String("[ERR]"))) fallbackColor = QStringLiteral("#e06c75");
                else if (escaped.contains(QLatin1String("[FTL]"))) fallbackColor = QStringLiteral("#e06c75");
                formattedLines.append(QStringLiteral("<font color='%1'>%2</font>").arg(fallbackColor, escaped));
            }
        }
        QString finalHtml = formattedLines.join(QStringLiteral("<br>"));

        QCoreApplication* app = QCoreApplication::instance();
        if (app) {
            QMetaObject::invokeMethod(app, [weakThis, finalHtml]() {
                if (!weakThis) return;
                weakThis->m_logContent = finalHtml;
                weakThis->m_loading = false;
                weakThis->m_lastLogPosition = 0; // reset pos as we read entire file
                emit weakThis->logContentChanged();
                emit weakThis->loadingChanged();
            });
        }
    });
}

void LogViewService::clearLogFile()
{
    Logger::clear();
    resetLogPosition();
}

QString LogViewService::readNewLogs()
{
    QString logPath = PathUtils::logsDir() + QStringLiteral("/app.log");
    QFile file(logPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    qint64 fileSize = file.size();
    if (fileSize < m_lastLogPosition) {
        m_lastLogPosition = 0;
    }

    if (!file.seek(m_lastLogPosition)) {
        return QString();
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QString content = in.readAll();
    m_lastLogPosition = file.pos();

    if (content.isEmpty()) {
        return QString();
    }

    QStringList lines = content.split(QLatin1Char('\n'));
    QStringList formattedLines;
    static const QRegularExpression regex(
        QStringLiteral(R"(^\[([\d\-:\.\s]+)\]\s+\[([A-Z]{3})\]\s+\[(T:0x[0-9a-fA-F]+)\]\s+\[([^\]]+)\](?:\s+\[([^\]]+)\])?\s+(.*)$)")
    );

    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;
        QString escaped = trimmed.toHtmlEscaped();
        auto match = regex.match(escaped);
        if (match.hasMatch()) {
            QString ts = match.captured(1);
            QString level = match.captured(2);
            QString thread = match.captured(3);
            QString category = match.captured(4);
            QString location = match.captured(5);
            QString msg = match.captured(6);
            QString levelColor = QStringLiteral("#abb2bf");
            bool isBold = false;
            if (level == QLatin1String("DBG")) levelColor = QStringLiteral("#5c6370");
            else if (level == QLatin1String("INF")) levelColor = QStringLiteral("#98c379");
            else if (level == QLatin1String("WRN")) levelColor = QStringLiteral("#d19a66");
            else if (level == QLatin1String("ERR") || level == QLatin1String("FTL")) {
                levelColor = QStringLiteral("#e06c75");
                isBold = true;
            }
            QString formatted = QStringLiteral("<font color='#5c6370'>[%1]</font> "
                                               "<font color='%2'><b>[%3]</b></font> "
                                               "<font color='#c678dd'>[%4]</font> "
                                               "<font color='#61afef'>[%5]</font> ").arg(ts, levelColor, level, thread, category);
            if (!location.isEmpty()) formatted += QStringLiteral("<font color='#e5c07b'>[%1]</font> ").arg(location);
            formatted += QStringLiteral("<font color='%1'>%2%3%4</font>")
                             .arg(isBold ? levelColor : QStringLiteral("#abb2bf"), isBold ? QStringLiteral("<b>") : QString(), msg, isBold ? QStringLiteral("</b>") : QString());
            formattedLines.append(formatted);
        } else {
            QString fallbackColor = QStringLiteral("#abb2bf");
            if (escaped.contains(QLatin1String("[DBG]"))) fallbackColor = QStringLiteral("#5c6370");
            else if (escaped.contains(QLatin1String("[WRN]"))) fallbackColor = QStringLiteral("#d19a66");
            else if (escaped.contains(QLatin1String("[ERR]"))) fallbackColor = QStringLiteral("#e06c75");
            else if (escaped.contains(QLatin1String("[FTL]"))) fallbackColor = QStringLiteral("#e06c75");
            formattedLines.append(QStringLiteral("<font color='%1'>%2</font>").arg(fallbackColor, escaped));
        }
    }

    return formattedLines.join(QStringLiteral("<br>"));
}

void LogViewService::resetLogPosition()
{
    m_lastLogPosition = 0;
}

QString LogViewService::readLogFile() const
{
    QString logPath = PathUtils::logsDir() + QStringLiteral("/app.log");
    QFile file(logPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QStringLiteral("Failed to open app.log");
    }
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    return in.readAll();
}

} // namespace LAStudio
