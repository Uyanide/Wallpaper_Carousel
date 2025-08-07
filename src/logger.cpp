/*
 * @Author: Uyanide pywang0608@foxmail.com
 * @Date: 2025-08-07 01:12:37
 * @LastEditTime: 2025-08-07 21:11:22
 * @Description: Implementation of logger.
 */
#include "logger.h"

#include <unistd.h>

#include <QCoreApplication>
#include <QObject>
#include <QProcessEnvironment>
#include <QString>
#include <QTextStream>
#include <QThreadPool>

#ifdef GENERAL_LOGGER_DISABLED
#define ENSURE_ENABLED return;
#else
#define ENSURE_ENABLED
#endif

Logger* Logger::instance(FILE* stream) {
    static Logger* logger{};
    if (!logger) {
        if (!stream) {
            stream = stderr;  // Default to stderr if no stream provided
        }
        logger = new Logger(stream);
        // Ensure logger runs in the main thread
        logger->moveToThread(QCoreApplication::instance()->thread());
    }
    return logger;
}

Logger::Logger(FILE* stream, QObject* parent)
    : QObject(parent), m_stream(stream), m_logStream(stream) {
    connect(this,
            &Logger::logSig,
            this,
            &Logger::_log,
            Qt::QueuedConnection);
}

void Logger::_log(
    const QString& msg,
    const QString& levelString,
    const QString& levelColorString,
    const QString& textColorString,
    const GeneralLogger::LogIndent indent) {
    ENSURE_ENABLED

    m_logStream << levelColorString << levelString << ' ';
    for (qint32 i = 0; i < indent; i++) m_logStream << "  ";
    m_logStream << textColorString << msg << (textColorString.isEmpty() ? "\n" : "\033[0m\n");
    m_logStream.flush();
}

bool Logger::isColored() {
    const auto stream = Logger::instance()->m_stream;
    if (!isatty(fileno(stream))) {
        return false;
    }
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString term            = env.value("TERM");
    if (term.isEmpty() || term == "dumb") {
        return false;
    }
    return true;
}

void GeneralLogger::info(
    const QString& msg,
    const GeneralLogger::LogIndent indent) {
    ENSURE_ENABLED
    constexpr const char* colorInfoMsg[]{"\033[32m", "\033[0m", "\033[0m"};
    const auto color = Logger::isColored();
    emit Logger::instance() -> logSig(msg,
                                      "[INFO]",
                                      color ? "\033[92m" : "",
                                      color ? colorInfoMsg[indent] : "",
                                      indent);
}

void GeneralLogger::warn(
    const QString& msg,
    const GeneralLogger::LogIndent indent) {
    ENSURE_ENABLED
    const auto color = Logger::isColored();
    emit Logger::instance() -> logSig(msg,
                                      "[WARN]",
                                      color ? "\033[93m" : "",
                                      color ? "\033[33m" : "",
                                      indent);
}

void GeneralLogger::error(
    const QString& msg,
    const GeneralLogger::LogIndent indent) {
    ENSURE_ENABLED
    const auto color = Logger::isColored();
    emit Logger::instance() -> logSig(msg,
                                      "[ERROR]",
                                      color ? "\033[91m" : "",
                                      color ? "\033[31m" : "",
                                      indent);
}