/*
 * @Author: Uyanide pywang0608@foxmail.com
 * @Date: 2025-08-05 01:22:53
 * @LastEditTime: 2025-08-08 04:17:53
 * @Description: Animated carousel widget for displaying and selecting images.
 */
#ifndef IMAGES_CAROUSEL_H
#define IMAGES_CAROUSEL_H

#include <qtmetamacros.h>

#include <QFileInfo>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMutex>
#include <QObject>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QQueue>
#include <QRunnable>
#include <QScrollArea>
#include <QThreadPool>
#include <QTimer>
#include <QWidget>

#include "config.h"

class ImageData;
class ImageItem;
class ImageLoader;
class ImagesCarousel;
class ImagesCarouselScrollArea;

/**
 * @brief Data structure to hold image information
 *        and can be safely created and passed between threads.
 */
struct ImageData {
    QFileInfo file;
    QImage image;

    explicit ImageData(const QString& p, const int initWidth, const int initHeight);
};

/**
 * @brief Image label that displays an image,
 *        which should always be created in the main thread.
 */
class ImageItem : public QLabel {
    Q_OBJECT

  public:
    explicit ImageItem(const ImageData* data,
                       const int itemWidth,
                       const int itemHeight,
                       const int itemFocusWidth,
                       const int itemFocusHeight,
                       QWidget* parent = nullptr);

    ~ImageItem() override;

    [[nodiscard]] QString getFileFullPath() const { return m_data->file.absoluteFilePath(); }

    [[nodiscard]] QString getFileName() const { return m_data->file.fileName(); }

    [[nodiscard]] QDateTime getFileDate() const { return m_data->file.lastModified(); }

    [[nodiscard]] const QImage& getThumbnail() const { return m_data->image; }

    [[nodiscard]] qint64 getFileSize() const { return m_data->file.size(); }

    void setFocus(bool focus = true);

    int m_index = 0;

  protected:
    void mousePressEvent(QMouseEvent* event) override {
        emit clicked(m_index);
        QLabel::mousePressEvent(event);
    }

  private:
    const ImageData* m_data;
    QSize m_itemSize;
    QSize m_itemFocusSize;
    QPropertyAnimation* m_scaleAnimation = nullptr;

  signals:
    void clicked(int index);
};

/**
 * @brief Worker class for loading images in a separate thread.
 */
class ImageLoader : public QRunnable {
  public:
    ImageLoader(const QString& path, ImagesCarousel* carousel);
    void run() override;  // friend to ImagesCarousel

  private:
    QString m_path;
    ImagesCarousel* m_carousel;
    const int m_initWidth;
    const int m_initHeight;
};

namespace Ui {
class ImagesCarousel;
}

class ImagesCarousel : public QWidget {
    Q_OBJECT

    friend void ImageLoader::run();

  public:
    explicit ImagesCarousel(const Config::StyleConfigItems& styleConfig,
                            const Config::SortConfigItems& sortConfig,
                            QWidget* parent = nullptr);
    ~ImagesCarousel();

    static constexpr int s_debounceInterval  = 200;
    static constexpr int s_animationDuration = 300;

    [[nodiscard]] QString getCurrentImagePath() const {
        if (m_currentIndex < 0 || m_currentIndex >= m_loadedImages.size()) {
            return "";
        }
        return m_loadedImages[m_currentIndex]->getFileFullPath();
    }

    // Should always be called in the main thread
    [[nodiscard]] qsizetype getLoadedImagesCount() {
        return m_loadedImages.size();
    }

    [[nodiscard]] qsizetype getAddedImagesCount() {
        QMutexLocker locker(&m_countMutex);
        return m_addedImagesCount;
    }

    // config items
    const int m_itemWidth;
    const int m_itemHeight;
    const int m_itemFocusWidth;
    const int m_itemFocusHeight;
    const Config::SortType m_sortType;
    const bool m_sortReverse;

  public slots:
    void focusNextImage();
    void focusPrevImage();
    void focusCurrImage();
    void unfocusCurrImage();
    void onStop();

  private slots:
    void _onScrollBarValueChanged(int value);
    void _onItemClicked(int index);
    void _onInitImagesLoaded();

  public:
    void appendImages(const QStringList& paths);

  private:
    Q_INVOKABLE void _insertImage(const ImageData* item);

  private:
    // UI elements
    Ui::ImagesCarousel* ui;
    QHBoxLayout* m_imagesLayout            = nullptr;
    ImagesCarouselScrollArea* m_scrollArea = nullptr;

    // Items and counters
    QVector<ImageItem*> m_loadedImages;  // m_loadedImages.size() may != m_loadedImagesCount
    int m_loadedImagesCount = 0;         // increase when _insertImage is called OR ImageLoader::run() is called with m_stopSign as true
    int m_addedImagesCount  = 0;         // increase when appendImages called
    QMutex m_countMutex;                 // for m_loadedImagesCount and m_addedImagesCount
    int m_currentIndex = 0;

    // Animations
    QPropertyAnimation* m_scrollAnimation = nullptr;

    // Auto focusing
    bool m_suppressAutoFocus      = false;
    int m_pendingScrollValue      = 0;
    QTimer* m_scrollDebounceTimer = nullptr;

    // Loading stopped by user
    QMutex m_stopSignMutex;
    bool m_stopSign = false;

  signals:
    void imageFocused(const QString& path, const int index, const int count);

    void loadingStarted(const qsizetype amount);
    void loadingCompleted(const qsizetype amount);
    void imageLoaded(const qsizetype count);

    void stopped();
};

class ImagesCarouselScrollArea : public QScrollArea {
    Q_OBJECT

  public:
    explicit ImagesCarouselScrollArea(QWidget* parent = nullptr)
        : QScrollArea(parent) {
        // setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        // setWidgetResizable(true);
    }

    void setBlockInput(bool block) { m_blockInput = block; }

  protected:
    void keyPressEvent(QKeyEvent* event) override {
        if (m_blockInput) {
            event->ignore();
            return;
        }
        if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right) {
            event->ignore();
        } else {
            QScrollArea::keyPressEvent(event);
        }
    }

    void wheelEvent(QWheelEvent* event) override {
        if (m_blockInput) {
            event->ignore();
            return;
        }
        if (event->angleDelta().y() != 0) {
            event->ignore();
        } else {
            QScrollArea::wheelEvent(event);
        }
    }

  private:
    bool m_blockInput = false;
};

#endif  // IMAGES_CAROUSEL_H
