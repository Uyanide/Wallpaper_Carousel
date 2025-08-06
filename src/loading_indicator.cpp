/*
 * @Author: Uyanide pywang0608@foxmail.com
 * @Date: 2025-08-07 00:32:25
 * @LastEditTime: 2025-08-07 01:09:49
 * @Description: A loading indicator widget with a progress bar.
 */
#include "loading_indicator.h"

LoadingIndicator::LoadingIndicator(QWidget *parent) : QWidget(parent),
                                                      ui(new Ui::LoadingIndicator) {
    ui->setupUi(this);
}

LoadingIndicator::~LoadingIndicator() {
    delete ui;
}
