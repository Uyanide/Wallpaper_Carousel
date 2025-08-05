/*
 * @Author: Uyanide pywang0608@foxmail.com
 * @Date: 2025-08-05 10:43:31
 * @LastEditTime: 2025-08-05 11:49:27
 * @Description:
 */
#ifndef GENERAL_LOGGER_H
#define GENERAL_LOGGER_H

#include <QString>
#include <QTextStream>

namespace GeneralLogger {

inline constexpr const char* colorInfoMsg[]{"\033[32m", "\033[0m", "\033[0m"};

enum LogIndent : qint32 {
    GENERAL = 0,
    STEP    = 1,
    DETAIL  = 2,
};

#ifdef GENERAL_LOGGER_DISABLED
#define ENSURE_ENABLED return;
#else
#define ENSURE_ENABLED
extern QTextStream g_logStream;
#endif

inline void
info(const QString& msg,
     const LogIndent indent = GENERAL,
     const bool color       = true) {
    ENSURE_ENABLED

    g_logStream << (color ? "\033[92m" : "") << "[INFO] ";
    for (qint32 i = 0; i < indent; i++) g_logStream << "  ";
    g_logStream << (color ? colorInfoMsg[indent] : "") << msg << (color ? "\033[0m\n" : "\n");
    g_logStream.flush();
}

inline void
warn(const QString& msg,
     const LogIndent indent = GENERAL,
     const bool color       = true) {
    ENSURE_ENABLED

    g_logStream << (color ? "\033[93m" : "") << "[WARN] ";
    for (uint32_t i = 0; i < indent; i++) g_logStream << "  ";
    g_logStream << (color ? "\033[33m" : "") << msg << (color ? "\033[0m\n" : "\n");
    g_logStream.flush();
}

inline void
error(const QString& msg,
      const LogIndent indent = GENERAL,
      const bool color       = true) {
    ENSURE_ENABLED

    g_logStream << (color ? "\033[91m" : "") << "[ERROR] ";
    for (uint32_t i = 0; i < indent; i++) g_logStream << "  ";
    g_logStream << (color ? "\033[31m" : "") << msg << (color ? "\033[0m\n" : "\n");
    g_logStream.flush();
}

#undef ENSURE_ENABLED

};  // namespace GeneralLogger

#endif  // GENERAL_LOGGER_H
