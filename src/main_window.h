/*
 * @Author: Uyanide pywang0608@foxmail.com
 * @Date: 2025-08-05 00:37:58
 * @LastEditTime: 2025-08-05 19:53:51
 * @Description: MainWindow implementation.
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "config.h"
#include "images_carousel.h"

QT_BEGIN_NAMESPACE

namespace Ui {
class MainWindow;
}

QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    MainWindow(const Config &config, QWidget *parent = nullptr);
    ~MainWindow();

  public slots:
    void onConfirm();
    void onCancel();

  protected:
    void keyPressEvent(QKeyEvent *event) override;

  private:
    void _setupUI();

  private:
    Ui::MainWindow *ui;
    ImagesCarousel *m_carousel = nullptr;
    const Config &m_config;
};
#endif  // MAINWINDOW_H
