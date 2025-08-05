/*
 * @Author: Uyanide pywang0608@foxmail.com
 * @Date: 2025-08-05 01:22:53
 * @LastEditTime: 2025-08-05 17:25:59
 * @Description: Animated carousel widget for displaying and selecting images.
 */
#include "images_carousel.h"

#include <pthread.h>
#include <qevent.h>
#include <qpropertyanimation.h>
#include <qvariantanimation.h>

#include <QLabel>
#include <QMetaObject>
#include <QScrollArea>
#include <QScrollBar>

#include "logger.h"
#include "ui_images_carousel.h"

using namespace GeneralLogger;

ImagesCarousel::ImagesCarousel(QWidget* parent)
    : QWidget(parent),
      ui(new Ui::ImagesCarousel),
      m_updateTimer(new QTimer(this)),
      m_scrollAnimation(nullptr) {
    ui->setupUi(this);

    connect(m_updateTimer, &QTimer::timeout, this, &ImagesCarousel::updateImages);
    m_updateTimer->start(100);
}

ImagesCarousel::~ImagesCarousel() {
    delete ui;
    for (auto item : m_imageQueue) {
        delete item;
    }
    if (m_scrollAnimation) {
        m_scrollAnimation->stop();
        delete m_scrollAnimation;
    }
}

void ImagesCarousel::appendImage(const QString& path) {
    ImageLoader* loader = new ImageLoader(path, this);
    QThreadPool::globalInstance()->start(loader);
}

ImageLoader::ImageLoader(const QString& path, ImagesCarousel* carousel)
    : m_path(path), m_carousel(carousel) {
    setAutoDelete(true);
}

void ImagesCarousel::addImageToQueue(const ImageData* data) {
    QMutexLocker locker(&m_queueMutex);
    auto imageItem = new ImageItem(data,
                                   s_itemWidth,
                                   s_itemHeight,
                                   s_itemFocusWidth,
                                   s_itemFocusHeight,
                                   this);
    m_imageQueue.enqueue(imageItem);
}

void ImagesCarousel::updateImages() {
    QMutexLocker locker(&m_queueMutex);

    int processCount = 0;
    while (!m_imageQueue.isEmpty() && processCount < 5) {
        ImageItem* item = m_imageQueue.dequeue();
        ui->scrollAreaWidgetContents->layout()->addWidget(item);
        m_loadedImages.append(item);
        if (m_loadedImages.size() == 1) {
            item->focusImage();
        } else {
            item->unfocusImage();
        }
        processCount++;
    }
}

void ImageLoader::run() {
    auto data = new ImageData(m_path);
    QMetaObject::invokeMethod(m_carousel,
                              "addImageToQueue",
                              Qt::QueuedConnection,
                              Q_ARG(const ImageData*, data));
}

ImageData::ImageData(const QString& p) : path(p) {
    path = p;
    if (!pixmap.load(p)) {
        warn(QString("Failed to load image from path: %1").arg(p));
    }
    // resize in "cover" mode
    const QSize targetSize(ImagesCarousel::s_itemWidth, ImagesCarousel::s_itemHeight);
    pixmap = pixmap.scaled(targetSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

    // Crop to center
    int x  = (pixmap.width() - targetSize.width()) / 2;
    int y  = (pixmap.height() - targetSize.height()) / 2;
    pixmap = pixmap.copy(x, y, targetSize.width(), targetSize.height());
}

void ImagesCarousel::focusNextImage() {
    unfocusCurrImage();
    if (m_loadedImages.size() <= 1) return;
    m_currentIndex++;
    if (m_currentIndex >= m_loadedImages.size()) {
        m_currentIndex = 0;
    }
    focusCurrImage();
}

void ImagesCarousel::focusPrevImage() {
    if (m_loadedImages.size() <= 1) return;
    unfocusCurrImage();
    m_currentIndex--;
    if (m_currentIndex < 0) {
        m_currentIndex = m_loadedImages.size() - 1;
    }
    focusCurrImage();
}

void ImagesCarousel::unfocusCurrImage() {
    m_loadedImages[m_currentIndex]->unfocusImage();
}

void ImagesCarousel::focusCurrImage() {
    m_loadedImages[m_currentIndex]->focusImage();
    auto hScrollBar  = ui->scrollArea->horizontalScrollBar();
    int spacing      = ui->scrollAreaWidgetContents->layout()->spacing();
    int centerOffset = (s_itemWidth + spacing) * m_currentIndex + s_itemFocusWidth / 2 - spacing;
    int leftOffset   = centerOffset - ui->scrollArea->width() / 2;
    if (leftOffset < 0) {
        leftOffset = 0;
    }

    if (m_scrollAnimation) {
        m_scrollAnimation->stop();
        delete m_scrollAnimation;
        m_scrollAnimation = nullptr;
    }

    m_scrollAnimation = new QPropertyAnimation(hScrollBar, "value");
    m_scrollAnimation->setDuration(300);
    m_scrollAnimation->setStartValue(hScrollBar->value());
    m_scrollAnimation->setEndValue(leftOffset);
    m_scrollAnimation->setEasingCurve(QEasingCurve::OutCubic);
    m_scrollAnimation->start();
}

ImageItem::ImageItem(const ImageData* data,
                     const int itemWidth,
                     const int itemHeight,
                     const int itemFocusWidth,
                     const int itemFocusHeight,
                     QWidget* parent)
    : QLabel(parent),
      m_data(data),
      m_itemSize(itemWidth, itemHeight),
      m_itemFocusSize(itemFocusWidth, itemFocusHeight) {
    setPixmap(data->pixmap);
    setFixedSize(ImagesCarousel::s_itemWidth, ImagesCarousel::s_itemHeight);
    setScaledContents(true);
}

void ImageItem::focusImage() {
    if (m_scaleAnimation) {
        m_scaleAnimation->stop();
        delete m_scaleAnimation;
        m_scaleAnimation = nullptr;
    }
    m_scaleAnimation = new QPropertyAnimation(this, "size");
    m_scaleAnimation->setDuration(ImagesCarousel::s_animationDuration);
    m_scaleAnimation->setStartValue(size());
    m_scaleAnimation->setEndValue(m_itemFocusSize);
    m_scaleAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_scaleAnimation,
            &QPropertyAnimation::valueChanged,
            this,
            [this](const QVariant& value) {
                setFixedSize(value.toSize());
            });
    m_scaleAnimation->start();
}

void ImageItem::unfocusImage() {
    if (m_scaleAnimation) {
        m_scaleAnimation->stop();
        delete m_scaleAnimation;
        m_scaleAnimation = nullptr;
    }
    m_scaleAnimation = new QPropertyAnimation(this, "size");
    m_scaleAnimation->setDuration(ImagesCarousel::s_animationDuration);
    m_scaleAnimation->setStartValue(size());
    m_scaleAnimation->setEndValue(m_itemSize);
    m_scaleAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_scaleAnimation,
            &QPropertyAnimation::valueChanged,
            this,
            [this](const QVariant& value) {
                setFixedSize(value.toSize());
            });
    m_scaleAnimation->start();
}