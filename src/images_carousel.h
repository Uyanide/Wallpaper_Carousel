/*
 * @Author: Uyanide pywang0608@foxmail.com
 * @Date: 2025-08-05 01:22:53
 * @LastEditTime: 2025-08-05 19:47:43
 * @Description: Animated carousel widget for displaying and selecting images.
 */
#ifndef IMAGES_CAROUSEL_H
#define IMAGES_CAROUSEL_H

#include <qtmetamacros.h>

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

class ImagesCarousel;

/**
 * @brief Data structure to hold image information
 *        and can be safely created and passed between threads.
 */
struct ImageData {
    QString path;
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

    [[nodiscard]] const QString& getPath() const { return m_data->path; }

    [[nodiscard]] const QPixmap& getPixmap() const { return m_data->pixmap; }

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
                            QWidget* parent = nullptr);
    ~ImagesCarousel();

    static constexpr int s_animationDuration = 300;

    [[nodiscard]] QString getCurrentImagePath() const {
        if (m_currentIndex < 0 || m_currentIndex >= m_loadedImages.size()) {
            return "";
        }
        return m_loadedImages[m_currentIndex]->getPath();
    }

    const int m_itemWidth       = 320;
    const int m_itemHeight      = 180;
    const int m_itemFocusWidth  = 480;
    const int m_itemFocusHeight = 270;

  public slots:
    void focusNextImage();
    void focusPrevImage();

  private slots:
    void _unfocusCurrImage();

  public:
    void appendImage(const QString& path);

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
    int m_currentIndex = 0;
    QPropertyAnimation* m_scrollAnimation;
};

class ImagesCarouselScrollArea : public QScrollArea {
    Q_OBJECT

  public:
    explicit ImagesCarouselScrollArea(QWidget* parent = nullptr)
        : QScrollArea(parent) {
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setWidgetResizable(true);
    }

  protected:
    void keyPressEvent(QKeyEvent* event) override {
        event->ignore();
    }
};

#endif  // IMAGES_CAROUSEL_H
