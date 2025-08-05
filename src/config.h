/*
 * @Author: Uyanide pywang0608@foxmail.com
 * @Date: 2025-08-05 01:34:52
 * @LastEditTime: 2025-08-05 20:12:47
 * @Description: Configuration manager.
 */
#ifndef CONFIG_H
#define CONFIG_H

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

    [[nodiscard]] const QString& getActionsConfirm() const { return m_configItems.actionsConfirm; }

    [[nodiscard]] double getStyleAspectRatio() const { return m_configItems.styleAspectRatio; }

    [[nodiscard]] int getStyleImageWidth() const { return m_configItems.styleImageWidth; }

    [[nodiscard]] int getStyleImageFocusWidth() const { return m_configItems.styleImageFocusWidth; }

    [[nodiscard]] int getStyleWindowWidth() const { return m_configItems.styleWindowWidth; }

    [[nodiscard]] int getStyleWindowHeight() const { return m_configItems.styleWindowHeight; }

    static const QString s_DefaultConfigFileName;

  private:
    void
    _loadConfig(const QString& configPath);
    void _loadWallpapers();

  private:
    struct _ConfigItems {
        QStringList wallpaperPaths;
        QStringList wallpaperDirs;
        QStringList wallpaperExcludes;
        QString actionsConfirm;
        double styleAspectRatio  = 1.6;
        int styleImageWidth      = 320;
        int styleImageFocusWidth = 480;
        int styleWindowWidth     = 800;
        int styleWindowHeight    = 600;
    } m_configItems;

    QStringList m_wallpapers;
};

#endif  // CONFIG_H
