#ifndef TRANSFERLIST_H
#define TRANSFERLIST_H

#include <QWidget>
#include "common/common.h"

namespace Ui {
class TransferList;
}

class TransferList : public QWidget {
    Q_OBJECT

public:
    explicit TransferList(QWidget *parent = nullptr);
    ~TransferList();

    // 显示数据传输记录
    void displayDataRecord(QString path = Common::m_recordPath);

    // 显示上传窗口
    void showUpload();

    // 显示下载窗口
    void showDownload();

private:
    Ui::TransferList *ui;

signals:
    void currentTabSignal(QString);

private slots:
    void on_clear_btn_clicked();
};

#endif // TRANSFERLIST_H
