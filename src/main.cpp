/*
 * @Author: Uyanide pywang0608@foxmail.com
 * @Date: 2025-08-05 00:37:58
 * @LastEditTime: 2025-08-07 22:21:29
 * @Description: Entry point.
 */
#include <qapplication.h>

#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QTextStream>

#include "config.h"
#include "logger.h"
#include "main_window.h"

static QString getConfigDir() {
    auto configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (configDir.isEmpty()) {
        configDir = QDir::homePath() + QDir::separator() + ".config" + QDir::separator() + "wallpaper_chooser";
    }
    QDir().mkpath(configDir);
    return configDir;
}

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

#ifndef GENERAL_LOGGER_DISABLED
    Logger::instance(stderr, GeneralLogger::LogIndent::DETAIL, &a);
#endif  // GENERAL_LOGGER_DISABLED

    Config config(getConfigDir());

    MainWindow w(config);
    w.show();

    return a.exec();
}
