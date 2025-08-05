/*
 * @Author: Uyanide pywang0608@foxmail.com
 * @Date: 2025-08-05 00:37:58
 * @LastEditTime: 2025-08-05 12:18:00
 * @Description:
 */
#include "mainwindow.h"

#include <QDir>
#include <QKeyEvent>
#include <QPushButton>

#include "./ui_mainwindow.h"

MainWindow::MainWindow(const QString &configDir, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {

    m_config = new Config(configDir, {}, this);

    ui->setupUi(this);
    _setupUI();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::_setupUI() {
    connect(ui->confirmButton, &QPushButton::clicked, this, &MainWindow::onConfirm);
    connect(ui->cancelButton, &QPushButton::clicked, this, &MainWindow::onCancel);
    ui->confirmButton->setFocusPolicy(Qt::NoFocus);
    ui->cancelButton->setFocusPolicy(Qt::NoFocus);
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        onCancel();
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        onConfirm();
    } else {
        QMainWindow::keyPressEvent(event);
    }
}

void MainWindow::onConfirm() {
    close();
}

void MainWindow::onCancel() {
    close();
}