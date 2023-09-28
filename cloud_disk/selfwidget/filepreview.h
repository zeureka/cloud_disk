#ifndef FILEPREVIEW_H
#define FILEPREVIEW_H

#include <QDialog>
#include <QNetworkAccessManager>
#include "../common/common.h"

namespace Ui {
class FilePreview;
}

class FilePreview : public QDialog {
    Q_OBJECT

public:
    explicit FilePreview(QWidget *parent = nullptr);
    ~FilePreview();

    void setInfo(FileInfo* info);

private:
    Ui::FilePreview *ui;
    QNetworkAccessManager* m_manager;
};

#endif // FILEPREVIEW_H
