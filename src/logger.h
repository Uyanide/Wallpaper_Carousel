/*
 * @Author: Uyanide pywang0608@foxmail.com
 * @Date: 2025-08-05 10:43:31
 * @LastEditTime: 2025-08-07 21:26:09
 * @Description: A simple thread-safe logger for general use.
 */
#ifndef GENERAL_LOGGER_H
#define GENERAL_LOGGER_H

#include <QObject>
#include <QString>
#include <QTextStream>

namespace GeneralLogger {

enum LogIndent : qint32 {
    GENERAL = 0,
    STEP    = 1,
    DETAIL  = 2,
};

void info(const QString& msg,
          const LogIndent indent = GENERAL);

void warn(const QString& msg,
          const LogIndent indent = GENERAL);

void error(const QString& msg,
           const LogIndent indent = GENERAL);
}  // namespace GeneralLogger

#ifndef GENERAL_LOGGER_DISABLED
class Logger : public QObject {
    Q_OBJECT

  public:
    static Logger* instance(FILE* stream                    = nullptr,
                            GeneralLogger::LogIndent indent = GeneralLogger::DETAIL,
                            QObject* parent                 = nullptr);

    static bool isColored();

  private:
    explicit Logger(FILE* stream,
                    GeneralLogger::LogIndent indent,
                    QObject* parent);

  private slots:
    void _log(const QString& msg,
              const QString& levelString,
              const QString& levelColorString,
              const QString& textColorString,
              const GeneralLogger::LogIndent indent);

  private:
    FILE* m_stream;
    QTextStream m_logStream;
    GeneralLogger::LogIndent m_indent;

  signals:
    void logSig(const QString& msg,
                const QString& levelString,
                const QString& levelColorString,
                const QString& textColorString,
                const GeneralLogger::LogIndent indent);
};

#endif  // GENERAL_LOGGER_DISABLED
#endif  // GENERAL_LOGGER_H
