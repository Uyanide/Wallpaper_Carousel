/*
 * @Author: Uyanide pywang0608@foxmail.com
 * @Date: 2025-08-05 01:34:52
 * @LastEditTime: 2025-08-05 12:14:04
 * @Description:
 */
#ifndef CONFIG_H
#define CONFIG_H

#include <qcontainerfwd.h>
#include <qtypes.h>

#include <QObject>
#include <QString>
#include <QStringList>

class Config : public QObject {
    Q_OBJECT

  public:
    Config(const QString& configDir, const QStringList& searchDirs = {}, QObject* parent = nullptr);

    ~Config();

    static bool isValidImageFile(const QString& filePath);

    [[nodiscard]] const QStringList& getWallpapers() const { return m_wallpapers; }

    [[nodiscard]] qint64 getWallpaperCount() const { return m_wallpapers.size(); }

    static const QString s_DefaultConfigFileName;

  private:
    void
    _loadConfig(const QString& configPath);
    void _loadWallpapers();

  private:
    struct ConfigItems {
        QStringList wallpaperPaths;
        QStringList wallpaperDirs;
        QStringList wallpaperExcludes;
    } m_configItems;

    QStringList m_wallpapers;
};

#endif  // CONFIG_H
