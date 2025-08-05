/*
 * @Author: Uyanide pywang0608@foxmail.com
 * @Date: 2025-08-05 01:22:53
 * @LastEditTime: 2025-08-06 01:32:56
 * @Description: Animated carousel widget for displaying and selecting images.
 */
#ifndef IMAGES_CAROUSEL_H
#define IMAGES_CAROUSEL_H

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

class ImagesCarousel;

/**
 * @brief Data structure to hold image information
 *        and can be safely created and passed between threads.
 */
struct ImageData {
    QFileInfo file;
    QPixmap pixmap;

    explicit ImageData(const QString& p, const int initWidth, const int initHeight);
};

/**
 * @brief Image label that displays an image
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

    [[nodiscard]] const QPixmap& getPixmap() const { return m_data->pixmap; }

    [[nodiscard]] qint64 getFileSize() const { return m_data->file.size(); }

  public:
    void setFocus(bool focus = true);

  private:
    const ImageData* m_data;
    QSize m_itemSize;
    QSize m_itemFocusSize;
    QPropertyAnimation* m_scaleAnimation = nullptr;
};

class ImageLoader : public QRunnable {
  public:
    ImageLoader(const QString& path, ImagesCarousel* carousel);
    void run() override;

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
    explicit ImagesCarousel(const double itemAspectRatio,
                            const int itemWidth,
                            const int itemFocusWidth,
                            const Config::SortType sortType,
                            const bool sortReverse,
                            QWidget* parent = nullptr);
    ~ImagesCarousel();

    static constexpr int s_animationDuration = 300;

    [[nodiscard]] QString getCurrentImagePath() const {
        if (m_currentIndex < 0 || m_currentIndex >= m_loadedImages.size()) {
            return "";
        }
        return m_loadedImages[m_currentIndex]->getFileFullPath();
    }

    const int m_itemWidth             = 320;
    const int m_itemHeight            = 180;
    const int m_itemFocusWidth        = 480;
    const int m_itemFocusHeight       = 270;
    const Config::SortType m_sortType = Config::SortType::None;
    const bool m_sortReverse          = false;

  public slots:
    void focusNextImage();
    void focusPrevImage();

  private slots:
    void _unfocusCurrImage();
    void _onScrollBarValueChanged(int value);

  public:
    void
    appendImages(const QStringList& paths);

  private:
    Q_INVOKABLE void _addImageToQueue(const ImageData* data);
    void _focusCurrImage();
    void _updateImages();

  private:
    Ui::ImagesCarousel* ui;
    QMutex m_queueMutex;
    QQueue<ImageItem*> m_imageQueue;
    QList<ImageItem*> m_loadedImages;
    QTimer* m_updateTimer;
    int m_currentIndex                    = 0;
    QPropertyAnimation* m_scrollAnimation = nullptr;
    QHBoxLayout* m_imagesLayout           = nullptr;
    QMutex m_imageCountMutex;
    int m_imageCount              = 0;
    bool m_suppressAutoFocus      = false;
    int m_pendingScrollValue      = 0;
    QTimer* m_scrollDebounceTimer = nullptr;

  signals:
    void imageFocused(const QString& path, const int index, const int count);
    void imagesLoaded();
};

class ImagesCarouselScrollArea : public QScrollArea {
    Q_OBJECT

  public:
    explicit ImagesCarouselScrollArea(QWidget* parent = nullptr)
        : QScrollArea(parent) {
        // setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        // setWidgetResizable(true);
    }

  protected:
    void keyPressEvent(QKeyEvent* event) override {
        if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right) {
            event->ignore();
        } else {
            QScrollArea::keyPressEvent(event);
        }
    }
};

#endif  // IMAGES_CAROUSEL_H
