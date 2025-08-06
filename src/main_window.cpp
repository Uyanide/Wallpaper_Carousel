/*
 * @Author: Uyanide pywang0608@foxmail.com
 * @Date: 2025-08-05 00:37:58
 * @LastEditTime: 2025-08-06 02:16:25
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

static QString splitNameFromPath(const QString &path) {
    QFileInfo fileInfo(path);
    return fileInfo.fileName();
}

MainWindow::MainWindow(const Config &config, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_config(config) {
    ui->setupUi(this);
    _setupUI();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::_setupUI() {
    // insert images carousel
    m_carousel = new ImagesCarousel(
        m_config.getStyleAspectRatio(),
        m_config.getStyleImageWidth(),
        m_config.getStyleImageFocusWidth(),
        m_config.getSortType(),
        m_config.isSortReverse(),
        this);
    ui->mainLayout->insertWidget(2, m_carousel);
    connect(m_carousel, &ImagesCarousel::imageFocused, this, &MainWindow::_onImageFocused);

    // set window size
    setMinimumSize(m_config.getStyleWindowWidth(), m_config.getStyleWindowHeight());
    setMaximumSize(m_config.getStyleWindowWidth(), m_config.getStyleWindowHeight());

    connect(ui->confirmButton, &QPushButton::clicked, this, &MainWindow::onConfirm);
    connect(ui->cancelButton, &QPushButton::clicked, this, &MainWindow::onCancel);
    ui->confirmButton->setFocusPolicy(Qt::NoFocus);
    ui->cancelButton->setFocusPolicy(Qt::NoFocus);

    m_carousel->appendImages(m_config.getWallpapers());
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        onCancel();
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        onConfirm();
    } else if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Tab || event->key() == Qt::Key_Right) {
        m_carousel->focusNextImage();
    } else if (event->key() == Qt::Key_Left) {
        m_carousel->focusPrevImage();
    } else {
        QMainWindow::keyPressEvent(event);
    }
}

void MainWindow::wheelEvent(QWheelEvent *event) {
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
    const auto cmdOrig = m_config.getActionsConfirm();
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

void MainWindow::_onImageFocused(const QString &path, const int index, const int count) {
    ui->topLabel->setText(QString("%1 (%2/%3)").arg(splitNameFromPath(path)).arg(index + 1).arg(count));
}