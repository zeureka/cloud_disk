#ifndef DATAPROGRESS_H
#define DATAPROGRESS_H

#include <QWidget>

namespace Ui {
class DataProgress;
}

class DataProgress : public QWidget {
    Q_OBJECT

public:
    explicit DataProgress(QWidget *parent = nullptr);
    ~DataProgress();

    // 设置文件名
    void setFileName(QString name = "test");

    // 设置进度条当前的value，最大为max
    void setProgressValue(int value = 0, int max = 100);

private:
    Ui::DataProgress *ui;
};

#endif // DATAPROGRESS_H
