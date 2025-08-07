/*
 * @Author: Uyanide pywang0608@foxmail.com
 * @Date: 2025-08-05 01:22:53
 * @LastEditTime: 2025-08-07 22:17:41
 * @Description: Animated carousel widget for displaying and selecting images.
 */
#include "images_carousel.h"

#include <pthread.h>
#include <qboxlayout.h>
#include <qdebug.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qpixmap.h>

#include <QLabel>
#include <QMetaObject>
#include <QScrollArea>
#include <QScrollBar>
#include <QVector>
#include <functional>

#include "logger.h"
#include "ui_images_carousel.h"

using namespace GeneralLogger;

ImagesCarousel::ImagesCarousel(const Config::StyleConfigItems& styleConfig,
                               const Config::SortConfigItems& sortConfig,
                               QWidget* parent)
    : QWidget(parent),
      ui(new Ui::ImagesCarousel),
      m_itemWidth(styleConfig.imageWidth),
      m_itemHeight(static_cast<int>(m_itemWidth / styleConfig.aspectRatio)),
      m_itemFocusWidth(styleConfig.imageFocusWidth),
      m_itemFocusHeight(static_cast<int>(styleConfig.imageFocusWidth / styleConfig.aspectRatio)),
      m_sortType(sortConfig.type),
      m_sortReverse(sortConfig.reverse) {
    ui->setupUi(this);
    m_scrollArea = dynamic_cast<ImagesCarouselScrollArea*>(ui->scrollArea);

    m_imagesLayout = dynamic_cast<QHBoxLayout*>(ui->scrollAreaWidgetContents->layout());

    // Load initial images
    connect(this,
            &ImagesCarousel::loadingCompleted,
            this,
            &ImagesCarousel::_onInitImagesLoaded);

    // Auto focus when scrolling
    m_scrollDebounceTimer = new QTimer(this);
    m_scrollDebounceTimer->setSingleShot(true);
    m_scrollDebounceTimer->setInterval(s_debounceInterval);
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

void ImagesCarousel::_onInitImagesLoaded() {
    disconnect(this, &ImagesCarousel::loadingCompleted, this, &ImagesCarousel::_onInitImagesLoaded);
    if (m_loadedImages.isEmpty()) {
        return;
    }
    _focusCurrImage();
}

ImagesCarousel::~ImagesCarousel() {
    delete ui;
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
    emit loadingStarted(paths.size());
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

void ImagesCarousel::_insertImage(const ImageData* data) {
    auto item = new ImageItem(
        data,
        m_itemWidth,
        m_itemHeight,
        m_itemFocusWidth,
        m_itemFocusHeight,
        this);

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

    // insert into correct position based on sort type and direction
    // currently O(n^2), but better as O(n * (n + log(n))) with vector and binary search
    qint64 inserPos = m_loadedImages.size();
    if (m_sortType != Config::SortType::None) {
        for (auto it = m_loadedImages.rbegin();
             it != m_loadedImages.rend() &&
             cmpFuncs[static_cast<int>(m_sortType)](*it, item) == m_sortReverse;
             (*it)->m_index++, ++it, --inserPos);
    }
    item->m_index = inserPos;
    connect(item,
            &ImageItem::clicked,
            this,
            &ImagesCarousel::_onItemClicked);
    m_loadedImages.insert(inserPos, item);
    m_imagesLayout->insertWidget(inserPos, item);

    emit imageLoaded(m_loadedImages.size());
    {
        QMutexLocker countLocker(&m_imageCountMutex);
        if (m_loadedImages.size() >= m_imageCount) {
            emit loadingCompleted(m_loadedImages.size());
        }
    }
}

void ImageLoader::run() {
    auto data = new ImageData(m_path, m_initWidth, m_initHeight);
    QMetaObject::invokeMethod(m_carousel,
                              "_insertImage",
                              Qt::QueuedConnection,
                              Q_ARG(const ImageData*, data));
}

ImageData::ImageData(const QString& p, const int initWidth, const int initHeight) : file(p) {
    if (!image.load(p)) {
        warn(QString("Failed to load image from path: %1").arg(p));
        return;
    }
    // resize in "cover" mode
    const QSize targetSize(initWidth, initHeight);
    image = image.scaled(targetSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

    // Crop to center
    int x = (image.width() - targetSize.width()) / 2;
    int y = (image.height() - targetSize.height()) / 2;
    image = image.copy(x, y, targetSize.width(), targetSize.height());
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
    m_scrollAnimation->setDuration(s_animationDuration);
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

void ImagesCarousel::_onItemClicked(int index) {
    // if (m_suppressAutoFocus) return;
    _unfocusCurrImage();
    m_currentIndex = index;
    if (index < 0 || index >= m_loadedImages.size()) {
        return;  // Out of bounds
    }
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
    if (data->image.isNull()) {
        setText(":(");
        setAlignment(Qt::AlignCenter);
    } else {
        setPixmap(QPixmap::fromImage(data->image));
    }
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
