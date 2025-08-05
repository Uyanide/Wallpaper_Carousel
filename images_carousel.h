#ifndef IMAGES_CAROUSEL_H
#define IMAGES_CAROUSEL_H

#include <QWidget>

namespace Ui {
class ImagesCarousel;
}

class ImagesCarousel : public QWidget {
    Q_OBJECT

  public:
    explicit ImagesCarousel(QWidget *parent = nullptr);
    ~ImagesCarousel();

  private:
    Ui::ImagesCarousel *ui;
};

#endif  // IMAGES_CAROUSEL_H
