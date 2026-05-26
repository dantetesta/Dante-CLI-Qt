#include "Logger.h"
#include "../persistence/AppPaths.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QtGlobal>
#include <QMutex>
#include <QDir>

namespace dante::telemetry {

namespace {
    QFile g_logFile;
    QMutex g_mutex;
    QString g_logPath;

    void handler(QtMsgType type, const QMessageLogContext&, const QString& msg) {
        QMutexLocker lk(&g_mutex);
        if (!g_logFile.isOpen()) return;
        const char* tag = "?";
        switch (type) {
            case QtDebugMsg:    tag = "DEBUG"; break;
            case QtInfoMsg:     tag = "INFO";  break;
            case QtWarningMsg:  tag = "WARN";  break;
            case QtCriticalMsg: tag = "CRIT";  break;
            case QtFatalMsg:    tag = "FATAL"; break;
        }
        const QString line = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)
            + " [" + tag + "] " + msg + "\n";
        QTextStream(&g_logFile) << line;
        g_logFile.flush();
        // Also mirror to stderr in dev (RelWithDebInfo too).
        fputs(line.toUtf8().constData(), stderr);
    }
}

void install() {
    QDir dir(persistence::appDataDir().filePath("logs"));
    if (!dir.exists()) dir.mkpath(".");
    const QString stamp = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    g_logPath = dir.filePath("dante-" + stamp + ".log");
    g_logFile.setFileName(g_logPath);
    g_logFile.open(QIODevice::Append | QIODevice::Text);
    qInstallMessageHandler(handler);
    qInfo() << "Logger installed —" << g_logPath;
}

QString currentLogPath() { return g_logPath; }

} // namespace dante::telemetry
