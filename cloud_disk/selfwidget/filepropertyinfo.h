#ifndef FILEPROPERTYINFO_H
#define FILEPROPERTYINFO_H

#include <QDialog>
#include "../common/common.h"

namespace Ui {
class FilePropertyInfo;
}

class FilePropertyInfo : public QDialog
{
    Q_OBJECT

public:
    explicit FilePropertyInfo(QWidget *parent = nullptr);
    ~FilePropertyInfo();

    void setInfo(FileInfo* info);

private:
    Ui::FilePropertyInfo *ui;
};

#endif // FILEPROPERTYINFO_H
