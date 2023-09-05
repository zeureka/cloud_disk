#ifndef CHANGEPASSWORD_H
#define CHANGEPASSWORD_H

#include "../common/common.h"
#include <QWidget>

namespace Ui {
class ChangePassword;
}

class ChangePassword : public QWidget
{
    Q_OBJECT

public:
    explicit ChangePassword(QWidget *parent = nullptr);
    ~ChangePassword();

    void setPasswordJson(QString account, QString oldPassword, QString newPassword);
    QByteArray getPasswordJson() const;
    void clearData();

    // 重写closeEvent()函数
    void closeEvent(QCloseEvent* event);

signals:
    void finished();

private slots:
    void on_save_clicked();
    void on_concel_clicked();

private:
    Ui::ChangePassword *ui;

    QByteArray data;
};

#endif // CHANGEPASSWORD_H
