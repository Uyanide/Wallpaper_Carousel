/*
 * @Author: Uyanide pywang0608@foxmail.com
 * @Date: 2025-08-07 00:32:25
 * @LastEditTime: 2025-08-07 01:00:29
 * @Description: LoadingIndicator implementation.
 */
#ifndef LOADING_INDICATOR_H
#define LOADING_INDICATOR_H

#include <QWidget>

#include "ui_loading_indicator.h"

namespace Ui {
class LoadingIndicator;
}

class LoadingIndicator : public QWidget {
    Q_OBJECT

  public:
    explicit LoadingIndicator(QWidget *parent = nullptr);
    ~LoadingIndicator();

    void setMaximum(int max) { ui->progressBar->setMaximum(max); }

    void setValue(int value) { ui->progressBar->setValue(value); }

  private:
    Ui::LoadingIndicator *ui;
};

#endif  // LOADING_INDICATOR_H
