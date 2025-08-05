/*
 * @Author: Uyanide pywang0608@foxmail.com
 * @Date: 2025-08-05 00:37:58
 * @LastEditTime: 2025-08-05 17:34:37
 * @Description: Entry point.
 */
#include <qapplication.h>

#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QTextStream>

#include "logger.h"
#include "main_window.h"

QTextStream GeneralLogger::g_logStream(stderr);

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

    MainWindow w(::getConfigDir());
    w.show();

    return a.exec();
}
