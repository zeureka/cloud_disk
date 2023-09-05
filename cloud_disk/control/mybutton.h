#ifndef MYBUTTON_H
#define MYBUTTON_H

#include <QWidget>

class MyButton : public QWidget {
    Q_OBJECT
public:
    explicit MyButton(QWidget *parent = nullptr);
    explicit MyButton(const QString& imgPath, QWidget *parent = nullptr);

    // 设置背景图片路径
    void setImage(const QString& str = "");

protected:
    // 刷新背景图片
    void paintEvent(QPaintEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);

private:
    QString imagePath;
    int startXStep;
    int startYStep;
    int widthStep;
    int heightStep;

signals:
    void pressed();
    void released();
};

#endif // MYBUTTON_H
