/*
 * @Author: Uyanide pywang0608@foxmail.com
 * @Date: 2025-08-05 01:22:53
 * @LastEditTime: 2025-08-05 16:49:22
 * @Description:
 */
#ifndef IMAGES_CAROUSEL_H
#define IMAGES_CAROUSEL_H

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

struct ImageData {
    QString path;
    QPixmap pixmap;

    ImageData() = default;

    explicit ImageData(const QString& p);
};

class ImageItem : public QLabel {
    Q_OBJECT

  public:
    explicit ImageItem(const ImageData* data,
                       const int itemWidth,
                       const int itemHeight,
                       const int itemFocusWidth,
                       const int itemFocusHeight,
                       QWidget* parent = nullptr);

  public slots:
    void focusImage();
    void unfocusImage();

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
};

namespace Ui {
class ImagesCarousel;
}

class ImagesCarousel : public QWidget {
    Q_OBJECT

  public:
    explicit ImagesCarousel(QWidget* parent = nullptr);
    ~ImagesCarousel();

    static constexpr int s_itemWidth         = 320;
    static constexpr int s_itemHeight        = 200;
    static constexpr int s_itemFocusWidth    = 480;
    static constexpr int s_itemFocusHeight   = 300;
    static constexpr int s_animationDuration = 300;

  public slots:
    void addImageToQueue(const ImageData* data);
    void appendImage(const QString& path);
    void focusNextImage();
    void focusPrevImage();
    void unfocusCurrImage();
    void focusCurrImage();

  private slots:
    void updateImages();

  private:
    Ui::ImagesCarousel* ui;
    QMutex m_queueMutex;
    QQueue<ImageItem*> m_imageQueue;
    QList<ImageItem*> m_loadedImages;
    QTimer* m_updateTimer;
    int m_currentIndex = 0;
    QPropertyAnimation* m_scrollAnimation;

  signals:
    void imageLoaded(ImageData* imageData);
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
