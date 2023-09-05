#include "mybutton.h"
#include <QPainter>

MyButton::MyButton(QWidget *parent) : QWidget{parent} {
    // init
    this->startXStep = 8;
    this->startYStep = 8;
    this->widthStep = 16;
    this->heightStep = 16;
}

MyButton::MyButton(const QString& imgPath, QWidget* parent) : QWidget(parent){
    // init
    this->startXStep = 8;
    this->startYStep = 8;
    this->widthStep = 16;
    this->heightStep = 16;
    this->imagePath = imgPath;
}

void MyButton::paintEvent(QPaintEvent*) {
    QPainter paint(this);
    paint.drawPixmap(0, 0, this->width(), this->height(), QPixmap(this->imagePath));
}

void MyButton::setImage(const QString& str) {
    this->imagePath = str;
    this->update();
}

void MyButton::mousePressEvent(QMouseEvent*) {
    // 按下鼠标左键，按钮窗口放大显示
    this->setFixedSize(this->width() + this->widthStep, this->height() + this->heightStep);
    this->setGeometry(this->x() - this->startXStep, this->y() - this->startYStep, this->width() + this->widthStep, this->height() + this->heightStep);
    emit pressed();
}

void MyButton::mouseReleaseEvent(QMouseEvent*) {
    // 释放鼠标左键，按钮窗口大小还原
    this->setFixedSize(this->width() - this->widthStep, this->height() - this->heightStep);
    this->setGeometry(this->x() + this->startXStep, this->y() + this->startYStep, this->width() - this->widthStep, this->height() - this->heightStep);
    emit released();
}
