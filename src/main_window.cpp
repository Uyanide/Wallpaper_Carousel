/*
 * @Author: Uyanide pywang0608@foxmail.com
 * @Date: 2025-08-05 00:37:58
 * @LastEditTime: 2025-10-25 12:04:38
 * @Description: MainWindow implementation.
 */
#include "main_window.h"

#include <QDir>
#include <QKeyEvent>
#include <QProcess>
#include <QPushButton>

#include "./ui_main_window.h"
#include "images_carousel.h"
#include "logger.h"

using namespace GeneralLogger;

static QString splitNameFromPath(const QString& path) {
    QFileInfo fileInfo(path);
    return fileInfo.fileName();
}

MainWindow::MainWindow(const Config& config, QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_config(config) {
    ui->setupUi(this);
    _setupUI();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::_setupUI() {
    // change window title
    setWindowTitle("Wallpaper Carousel");
    // create images carousel
    m_carousel = new ImagesCarousel(
        m_config.getStyleConfig(),
        m_config.getSortConfig(),
        this);
    ui->mainLayout->insertWidget(2, m_carousel);
    connect(m_carousel,
            &ImagesCarousel::imageFocused,
            this,
            &MainWindow::_onImageFocused);
    connect(this, &MainWindow::stop, m_carousel, &ImagesCarousel::onStop);
    m_carouselIndex = ui->stackedWidget->addWidget(m_carousel);

    // create loading indicator
    m_loadingIndicator = new LoadingIndicator(this);
    connect(m_carousel,
            &ImagesCarousel::loadingStarted,
            this,
            &MainWindow::_onLoadingStarted);
    connect(m_carousel,
            &ImagesCarousel::loadingCompleted,
            this,
            &MainWindow::_onLoadingCompleted);
    connect(m_carousel,
            &ImagesCarousel::imageLoaded,
            m_loadingIndicator,
            &LoadingIndicator::setValue);
    m_loadingIndicatorIndex = ui->stackedWidget->addWidget(m_loadingIndicator);

    // set window size
    setMinimumSize(m_config.getStyleConfig().windowWidth, m_config.getStyleConfig().windowHeight);
    setMaximumSize(m_config.getStyleConfig().windowWidth, m_config.getStyleConfig().windowHeight);

    connect(ui->confirmButton,
            &QPushButton::clicked,
            this,
            &MainWindow::_onConfirmPressed);
    connect(ui->cancelButton,
            &QPushButton::clicked,
            this,
            &MainWindow::_onCancelPressed);
    ui->confirmButton->setFocusPolicy(Qt::NoFocus);
    ui->cancelButton->setFocusPolicy(Qt::NoFocus);

    m_carousel->appendImages(m_config.getWallpapers());
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        _onCancelPressed();
        return;
    }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        _onConfirmPressed();
        return;
    }

    switch (m_state) {
        case Init:
            MainWindow::keyPressEvent(event);
            break;
        case Loading:
            if (m_config.getStyleConfig().noLoadingScreen) {
                switch (event->key()) {
                    case Qt::Key_Space:
                    case Qt::Key_Tab:
                    case Qt::Key_Right:
                        m_carousel->focusNextImage();
                        break;
                    case Qt::Key_Left:
                        m_carousel->focusPrevImage();
                        break;
                    default:
                        QMainWindow::keyPressEvent(event);
                }
            } else {
                event->ignore();
            }
            break;
        case Ready:
            switch (event->key()) {
                case Qt::Key_Space:
                case Qt::Key_Tab:
                case Qt::Key_Right:
                    m_carousel->focusNextImage();
                    break;
                case Qt::Key_Left:
                    m_carousel->focusPrevImage();
                    break;
                default:
                    QMainWindow::keyPressEvent(event);
            }
            break;
        default:
            event->ignore();
            break;
    }
}

void MainWindow::_onCancelPressed() {
    switch (m_state) {
        case Loading:
            // case loading screen is disabled, quit the app
            if (m_config.getStyleConfig().noLoadingScreen) {
                info("Stopping loading and quitting app.");
                connect(
                    m_carousel,
                    &ImagesCarousel::stopped,
                    this,
                    &MainWindow::onCancel);
                m_state = Stopping;
                emit stop();
            }
            // otherwise, stop loading and display the loaded images
            else {
                info("Stopping loading.");
                connect(
                    m_carousel,
                    &ImagesCarousel::stopped,
                    this,
                    [this]() {
                        _onLoadingCompleted(m_carousel->getLoadedImagesCount());
                        m_carousel->focusCurrImage();
                    });
                m_state = Stopping;
                emit stop();
            }
            break;
        case Ready:
            info("Quitting app.");
            onCancel();
            break;
        default:
            break;
    }
}

void MainWindow::_onConfirmPressed() {
    switch (m_state) {
        case Loading:
            // case loading screen is disabled, confirm the selection
            if (m_config.getStyleConfig().noLoadingScreen) {
                info("Stopping loading and confirming selection.");
                connect(
                    m_carousel,
                    &ImagesCarousel::stopped,
                    this,
                    &MainWindow::onConfirm);
                m_state = Stopping;
                emit stop();
            }
            break;
        case Ready:
            info("Confirming selection.");
            onConfirm();
            break;
        default:
            break;
    }
}

void MainWindow::wheelEvent(QWheelEvent* event) {
    if (m_state != Ready && m_config.getStyleConfig().noLoadingScreen) {
        event->ignore();
        return;
    }
    if (event->angleDelta().y() > 0) {
        m_carousel->focusPrevImage();
    } else if (event->angleDelta().y() < 0) {
        m_carousel->focusNextImage();
    } else {
        QMainWindow::wheelEvent(event);
    }
}

void MainWindow::onConfirm() {
    close();
    const auto path = m_carousel->getCurrentImagePath();
    if (path.isEmpty()) {
        warn("No image selected");
        return;
    }
    info(QString("Selected image: %1").arg(path));
    const auto cmdOrig = m_config.getActionConfig().confirm;
    if (cmdOrig.isEmpty()) {
        warn("No action defined for confirmation");
        return;
    }
    const auto cmd = cmdOrig.arg(path);
    info(QString("Executing command: %1").arg(cmd));

    const auto arguments = QProcess::splitCommand(cmd);

    if (QProcess::execute(arguments.first(), arguments.mid(1))) {
        error(QString("Failed to execute command: %1").arg(cmd));
        return;
    }
}

void MainWindow::onCancel() {
    close();
}

void MainWindow::_onImageFocused(const QString& path, const int index, const int count) {
    ui->topLabel->setText(QString("%1 (%2/%3)").arg(splitNameFromPath(path)).arg(index + 1).arg(count));
}

void MainWindow::_onLoadingStarted(const qsizetype amount) {
    m_state = Loading;
    if (m_config.getStyleConfig().noLoadingScreen) {
        return;
    }
    m_loadingIndicator->setMaximum(amount);
    ui->stackedWidget->setCurrentIndex(m_loadingIndicatorIndex);
}

void MainWindow::_onLoadingCompleted(const qsizetype amount) {
    info(QString("Loading completed, loaded %1 images").arg(amount));
    ui->stackedWidget->setCurrentIndex(m_carouselIndex);
    m_state = Ready;
}
