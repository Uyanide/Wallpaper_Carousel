/*
 * @Author: Uyanide pywang0608@foxmail.com
 * @Date: 2025-08-05 01:22:53
 * @LastEditTime: 2025-08-06 02:12:46
 * @Description: Animated carousel widget for displaying and selecting images.
 */
#include "images_carousel.h"

#include <pthread.h>
#include <qboxlayout.h>
#include <qdebug.h>
#include <qobject.h>

#include <QLabel>
#include <QMetaObject>
#include <QScrollArea>
#include <QScrollBar>
#include <QVector>
#include <functional>

#include "logger.h"
#include "ui_images_carousel.h"

using namespace GeneralLogger;

ImagesCarousel::ImagesCarousel(const double itemAspectRatio,
                               const int itemWidth,
                               const int itemFocusWidth,
                               const Config::SortType sortType,
                               const bool sortReverse,
                               QWidget* parent)
    : QWidget(parent),
      ui(new Ui::ImagesCarousel),
      m_updateTimer(new QTimer(this)),
      m_itemWidth(itemWidth),
      m_itemHeight(static_cast<int>(itemWidth / itemAspectRatio)),
      m_itemFocusWidth(itemFocusWidth),
      m_itemFocusHeight(static_cast<int>(itemFocusWidth / itemAspectRatio)),
      m_sortType(sortType),
      m_sortReverse(sortReverse) {
    ui->setupUi(this);
    m_scrollArea = dynamic_cast<ImagesCarouselScrollArea*>(ui->scrollArea);

    m_imagesLayout = dynamic_cast<QHBoxLayout*>(ui->scrollAreaWidgetContents->layout());

    // Load initial images
    connect(m_updateTimer,
            &QTimer::timeout,
            this,
            &ImagesCarousel::_updateImages);
    connect(this,
            &ImagesCarousel::imagesLoaded,
            this,
            [this]() {
                _focusCurrImage();
                disconnect(this, &ImagesCarousel::imagesLoaded, this, nullptr);
            });
    m_updateTimer->start(100);

    // Auto focus when scrolling
    m_scrollDebounceTimer = new QTimer(this);
    m_scrollDebounceTimer->setSingleShot(true);
    m_scrollDebounceTimer->setInterval(200);
    connect(m_scrollDebounceTimer,
            &QTimer::timeout,
            this,
            [this]() {
                _onScrollBarValueChanged(m_pendingScrollValue);
            });
    connect(ui->scrollArea->horizontalScrollBar(),
            &QScrollBar::valueChanged,
            this,
            [this](int value) {
                m_pendingScrollValue = value;
                if (m_suppressAutoFocus) {
                    return;
                }
                m_scrollDebounceTimer->start();
            });
}

ImagesCarousel::~ImagesCarousel() {
    delete ui;
    for (auto item : std::as_const(m_imageQueue)) {
        delete item;
    }
    // memory of items in m_loadedImages managed by Qt parent-child system
    // ...
    if (m_scrollAnimation) {
        m_scrollAnimation->stop();
        m_scrollAnimation->deleteLater();
    }
}

void ImagesCarousel::appendImages(const QStringList& paths) {
    {
        QMutexLocker locker(&m_imageCountMutex);
        m_imageCount += paths.size();
    }
    for (const QString& path : paths) {
        ImageLoader* loader = new ImageLoader(path, this);
        QThreadPool::globalInstance()->start(loader);
    }
}

ImageLoader::ImageLoader(const QString& path, ImagesCarousel* carousel)
    : m_path(path),
      m_carousel(carousel),
      m_initWidth(carousel->m_itemFocusWidth),
      m_initHeight(carousel->m_itemFocusHeight) {
    setAutoDelete(true);
}

void ImagesCarousel::_addImageToQueue(const ImageData* data) {
    QMutexLocker locker(&m_queueMutex);
    auto imageItem = new ImageItem(
        data,
        m_itemWidth,
        m_itemHeight,
        m_itemFocusWidth,
        m_itemFocusHeight,
        this);
    m_imageQueue.enqueue(imageItem);
}

void ImagesCarousel::_updateImages() {
    QMutexLocker locker(&m_queueMutex);

    static const QVector<std::function<bool(const ImageItem*, const ImageItem*)>> cmpFuncs = {
        [](auto, auto) {
            return false;
        },  // None
        [](auto a, auto b) {
            return a->getFileName() < b->getFileName();
        },
        [](auto a, auto b) {
            return a->getFileDate() < b->getFileDate();
        },
        [](auto a, auto b) {
            return a->getFileSize() < b->getFileSize();
        },
    };

    int processCount = 0;
    while (!m_imageQueue.isEmpty() && processCount < 5) {
        ImageItem* item = m_imageQueue.dequeue();

        // insert into correct position based on sort type and direction
        // currently O(n^2), but better as O(n * (n + log(n))) with vector and binary search
        qint64 inserPos = m_loadedImages.size();
        if (m_sortType != Config::SortType::None) {
            for (auto it = m_loadedImages.rbegin();
                 it != m_loadedImages.rend() &&
                 cmpFuncs[static_cast<int>(m_sortType)](*it, item) == m_sortReverse;
                 ++it, --inserPos);
        }
        m_loadedImages.insert(inserPos, item);
        m_imagesLayout->insertWidget(inserPos, item);
        processCount++;
    }

    {
        QMutexLocker countLocker(&m_imageCountMutex);
        if (m_loadedImages.size() >= m_imageCount) {
            emit imagesLoaded();
        }
    }
}

void ImageLoader::run() {
    auto data = new ImageData(m_path, m_initWidth, m_initHeight);
    QMetaObject::invokeMethod(m_carousel,
                              "_addImageToQueue",
                              Qt::QueuedConnection,
                              Q_ARG(const ImageData*, data));
}

ImageData::ImageData(const QString& p, const int initWidth, const int initHeight) : file(p) {
    if (!pixmap.load(p)) {
        warn(QString("Failed to load image from path: %1").arg(p));
    }
    // resize in "cover" mode
    const QSize targetSize(initWidth, initHeight);
    pixmap = pixmap.scaled(targetSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

    // Crop to center
    int x  = (pixmap.width() - targetSize.width()) / 2;
    int y  = (pixmap.height() - targetSize.height()) / 2;
    pixmap = pixmap.copy(x, y, targetSize.width(), targetSize.height());
}

void ImagesCarousel::focusNextImage() {
    _unfocusCurrImage();
    if (m_loadedImages.size() <= 1) return;
    m_currentIndex++;
    if (m_currentIndex >= m_loadedImages.size()) {
        m_currentIndex = 0;
    }
    _focusCurrImage();
}

void ImagesCarousel::focusPrevImage() {
    if (m_loadedImages.size() <= 1) return;
    _unfocusCurrImage();
    m_currentIndex--;
    if (m_currentIndex < 0) {
        m_currentIndex = m_loadedImages.size() - 1;
    }
    _focusCurrImage();
}

void ImagesCarousel::_unfocusCurrImage() {
    // bound check was (or should) done by caller
    m_loadedImages[m_currentIndex]->setFocus(false);
}

void ImagesCarousel::_focusCurrImage() {
    // bound check was (or should) done by caller
    m_loadedImages[m_currentIndex]->setFocus(true);
    emit imageFocused(m_loadedImages[m_currentIndex]->getFileFullPath(),
                      m_currentIndex,
                      m_loadedImages.size());
    auto hScrollBar  = ui->scrollArea->horizontalScrollBar();
    int spacing      = ui->scrollAreaWidgetContents->layout()->spacing();
    int centerOffset = (m_itemWidth + spacing) * m_currentIndex + m_itemFocusWidth / 2 - spacing;
    int leftOffset   = centerOffset - ui->scrollArea->width() / 2;
    if (leftOffset < 0) {
        leftOffset = 0;
    }

    if (m_scrollAnimation) {
        m_scrollAnimation->stop();
        m_scrollAnimation->deleteLater();
        m_scrollAnimation = nullptr;
    }

    m_scrollAnimation = new QPropertyAnimation(hScrollBar, "value");
    m_scrollAnimation->setDuration(300);
    m_scrollAnimation->setStartValue(hScrollBar->value());
    m_scrollAnimation->setEndValue(leftOffset);
    m_scrollAnimation->setEasingCurve(QEasingCurve::OutCubic);

    // Suppress auto focus during animation
    connect(m_scrollAnimation,
            &QPropertyAnimation::finished,
            this,
            [this]() {
                m_suppressAutoFocus = false;
                m_scrollArea->setBlockInput(false);
            });
    m_suppressAutoFocus = true;
    m_scrollArea->setBlockInput(true);
    m_scrollAnimation->start();
}

void ImagesCarousel::_onScrollBarValueChanged(int value) {
    // Stop the animation if it is running
    if (m_scrollAnimation && m_scrollAnimation->state() == QPropertyAnimation::Running) {
        m_scrollAnimation->stop();
        m_scrollAnimation->deleteLater();
        m_scrollAnimation = nullptr;
    }
    int centerOffset = (value + m_itemFocusWidth / 2);
    int itemOffset   = m_itemWidth + ui->scrollAreaWidgetContents->layout()->spacing();
    int index        = centerOffset / itemOffset;
    if (index < 0 || index >= m_loadedImages.size()) {
        return;  // Out of bounds
    }
    if (index == m_currentIndex) {
        return;  // Already focused
    }
    _unfocusCurrImage();
    m_currentIndex = index;
    _focusCurrImage();
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
    setScaledContents(true);
    setPixmap(data->pixmap);
    setFixedSize(itemWidth, itemHeight);
}

ImageItem::~ImageItem() {
    if (m_scaleAnimation) {
        m_scaleAnimation->stop();
        delete m_scaleAnimation;
        m_scaleAnimation = nullptr;
    }
    delete m_data;
}

void ImageItem::setFocus(bool focus) {
    if (m_scaleAnimation) {
        m_scaleAnimation->stop();
        delete m_scaleAnimation;
        m_scaleAnimation = nullptr;
    }
    m_scaleAnimation = new QPropertyAnimation(this, "size");
    m_scaleAnimation->setDuration(ImagesCarousel::s_animationDuration);
    m_scaleAnimation->setStartValue(size());
    m_scaleAnimation->setEndValue(focus ? m_itemFocusSize : m_itemSize);
    m_scaleAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_scaleAnimation,
            &QPropertyAnimation::valueChanged,
            this,
            [this](const QVariant& value) {
                setFixedSize(value.toSize());
            });
    m_scaleAnimation->start();
}
