#ifndef EDITMATERIAL_H
#define EDITMATERIAL_H

#include "../common/common.h"
#include <QWidget>

namespace Ui {
class EditMaterial;
}

class EditMaterial : public QWidget
{
    Q_OBJECT

public:
    explicit EditMaterial(QWidget *parent = nullptr);
    ~EditMaterial();

    void setMaterialJson(QString account, QString phone, QString email);
    QByteArray getMaterialJson() const;
    void clearData();

    // 重写closeEvent()函数
    void closeEvent(QCloseEvent* event);

signals:
    void finished();

private slots:
    void on_save_clicked();
    void on_concel_clicked();

private:
    Ui::EditMaterial *ui;

    QByteArray data;
};

#endif // EDITMATERIAL_H
