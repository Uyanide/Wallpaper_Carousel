/*
 * @Author: Uyanide pywang0608@foxmail.com
 * @Date: 2025-08-05 01:34:52
 * @LastEditTime: 2025-08-05 12:17:37
 * @Description:
 */
#include "config.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QStandardPaths>

#include "logger.h"
using namespace GeneralLogger;

static QString expandPath(const QString &path);

const QString Config::s_DefaultConfigFileName = "config.json";

Config::Config(const QString &configDir, const QStringList &searchDirs, QObject *parent) : QObject(parent) {
    info(QString("Loading configuration from: %1").arg(configDir));
    _loadConfig(configDir + QDir::separator() + s_DefaultConfigFileName);

    info(QString("Additional search directories: %1").arg(searchDirs.join(", ")));
    m_configItems.wallpaperDirs.append(searchDirs);

    info("Loading wallpapers ...");
    _loadWallpapers();
}

Config::~Config() {
}

void Config::_loadConfig(const QString &configPath) {
    QFile configFile(configPath);
    if (!configFile.open(QIODevice::ReadOnly)) {
        error(QString("Failed to open config file: %1").arg(configPath));
        return;
    }
    QByteArray configData = configFile.readAll();
    configFile.close();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(configData);
    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        error(QString("Invalid JSON format in config file"));
        return;
    }

    static const auto parseJsonArray = [](const QJsonObject &obj, const QString &key, QStringList &list) {
        if (obj.contains(key) && obj[key].isArray()) {
            QJsonArray array = obj[key].toArray();
            for (const QJsonValue &value : array) {
                if (value.isString()) {
                    list.append(::expandPath(value.toString()));
                }
            }
        } else {
            warn(QString("Key '%1' not found or not an array in config").arg(key));
        }
    };

    const auto jsonObj = jsonDoc.object();
    if (!jsonObj.contains("wallpaper") || !jsonObj["wallpaper"].isObject()) {
        warn("Key 'wallpaper' not fount or not an object in config");
        return;
    }
    const auto wallpaperObj = jsonObj.value("wallpaper").toObject();
    parseJsonArray(wallpaperObj, "paths", m_configItems.wallpaperPaths);
    parseJsonArray(wallpaperObj, "dirs", m_configItems.wallpaperDirs);
    parseJsonArray(wallpaperObj, "excludes", m_configItems.wallpaperExcludes);
}

void Config::_loadWallpapers() {
    m_wallpapers.clear();

    QSet<QString> paths;

    info(QString("Loading wallpapers from %1 specified paths").arg(m_configItems.wallpaperPaths.size()), LogIndent::STEP);
    for (const QString &path : m_configItems.wallpaperPaths) {
        paths.insert(path);
    }

    info(QString("Loading wallpapers from %1 specified directories").arg(m_configItems.wallpaperDirs.size()), LogIndent::STEP);
    for (const QString &dirPath : m_configItems.wallpaperDirs) {
        QDir dir(dirPath);
        if (dir.exists()) {
            QStringList files = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
            for (const QString &file : files) {
                QString filePath = dir.filePath(file);
                paths.insert(filePath);
            }
        } else {
            warn(QString("Directory '%1' does not exist").arg(dirPath));
        }
    }

    info(QString("Excluding %1 specified paths").arg(m_configItems.wallpaperExcludes.size()), LogIndent::STEP);
    for (const QString &exclude : m_configItems.wallpaperExcludes) {
        paths.remove(exclude);
    }

    m_wallpapers.reserve(paths.size());
    for (const QString &path : paths) {
        if (isValidImageFile(path)) {
            m_wallpapers.append(path);
        }
    }
    info(QString("Found %1 wallpapers").arg(paths.size()));
}

bool Config::isValidImageFile(const QString &filePath) {
    static const QStringList validExtensions = {
        ".jpg",
        ".jpeg",
        ".png",
        ".bmp",
        ".gif",
        ".webp",
        ".tiff",
        ".avif",
        ".heic",
        ".heif"};

    // check if exist
    if (!QFile::exists(filePath)) {
        warn(QString("File does not exist: %1").arg(filePath));
        return false;
    }
    // check if normal file
    QFileInfo fileInfo(filePath);
    if (!fileInfo.isFile() || !fileInfo.isReadable()) {
        warn(QString("Invalid file: %1").arg(filePath));
        return false;
    }
    // check if valid extension
    for (const QString &ext : validExtensions) {
        if (filePath.endsWith(ext, Qt::CaseInsensitive)) {
            return true;
        }
    }
    warn(QString("Unsupported file type: %1").arg(filePath));
    return false;
}

static QString expandPath(const QString &path) {
    QString expandedPath = path;

    if (expandedPath.startsWith("~/")) {
        expandedPath.replace(0, 1, QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    } else if (expandedPath == "~") {
        expandedPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QRegularExpression envVarRegex(R"(\$([A-Za-z_][A-Za-z0-9_]*))");
    QRegularExpressionMatchIterator i = envVarRegex.globalMatch(expandedPath);

    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString varName               = match.captured(1);
        QString varValue              = env.value(varName);
        if (!varValue.isEmpty()) {
            expandedPath.replace(match.captured(0), varValue);
        }
    }

    return QDir::cleanPath(expandedPath);
}