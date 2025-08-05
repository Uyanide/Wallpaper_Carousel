/*
 * @Author: Uyanide pywang0608@foxmail.com
 * @Date: 2025-08-05 00:37:58
 * @LastEditTime: 2025-08-05 17:23:41
 * @Description: MainWindow implementation.
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "config.h"

QT_BEGIN_NAMESPACE

namespace Ui {
class MainWindow;
}

QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    MainWindow(const QString &configDir, QWidget *parent = nullptr);
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
    Config *m_config;
};
#endif  // MAINWINDOW_H
