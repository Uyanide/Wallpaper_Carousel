/*
 * @Author: Uyanide pywang0608@foxmail.com
 * @Date: 2025-08-05 01:34:52
 * @LastEditTime: 2025-08-07 22:03:05
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
    enum class SortType : int {
        None = 0,
        Name,
        Date,
        Size,
    };

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

    [[nodiscard]] bool isStyleNoLoadingScreen() const { return m_configItems.styleNoLoadingScreen; }

    [[nodiscard]] SortType getSortType() const { return m_configItems.sortType; }

    [[nodiscard]] bool isSortReverse() const { return m_configItems.sortReverse; }

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
        double styleAspectRatio   = 1.6;
        int styleImageWidth       = 320;
        int styleImageFocusWidth  = 480;
        int styleWindowWidth      = 720;
        int styleWindowHeight     = 500;
        bool styleNoLoadingScreen = false;
        SortType sortType         = SortType::None;
        bool sortReverse          = false;
    } m_configItems;

    QStringList m_wallpapers;
};

#endif  // CONFIG_H
