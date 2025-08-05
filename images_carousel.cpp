#include "images_carousel.h"

#include "ui_images_carousel.h"

ImagesCarousel::ImagesCarousel(QWidget *parent) : QWidget(parent),
                                                  ui(new Ui::ImagesCarousel) {
    ui->setupUi(this);
}

ImagesCarousel::~ImagesCarousel() {
    delete ui;
}
