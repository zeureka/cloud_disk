#ifndef FRMLOGIN_H
#define FRMLOGIN_H

#include <QWidget>
#include "./common/common.h"
#include "index.h"

namespace Ui {
class FrmLogin;
}

class FrmLogin : public QWidget {
    Q_OBJECT

public:
    explicit FrmLogin(QWidget *parent = nullptr);
    ~FrmLogin();

    // 设置登录用户的信息包
    QByteArray setLoginJson(QString account, QString password);

    // 设置注册用户的信息包
    QByteArray setEnrollJson(QString account, QString password, QString phone, QString email);

    // 得到服务器回复的登录状态，状态吗返回值为 "000" 或 "001", 还有登录的 section
    QStringList getLoginStatus(QByteArray loginStatus);


private slots:
    void on_login_btn_clicked();
    void on_to_enroll_btn_clicked();
    void on_enroll_btn_clicked();
    void on_to_login_btn_clicked();

private:
    Ui::FrmLogin *ui;
    void clear_data();

private:
    Common m_common;	// 工具类对象
    Index* index;		// 主窗口指针
};

#endif // FRMLOGIN_H
