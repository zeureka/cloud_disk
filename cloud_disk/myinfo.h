#ifndef MYINFO_H
#define MYINFO_H

#include "common/common.h"
#include "selfwidget/mymenu.h"

#include <QWidget>
#include <QNetworkAccessManager>


namespace Ui {
class MyInfo;
}

class MyInfo : public QWidget {
    Q_OBJECT

public:
    explicit MyInfo(QWidget *parent = nullptr);
    ~MyInfo();

    // 添加菜单项
    void addActionMenu();
    // 获取我的信息
    void getMyInfo();
    // 刷新 My Information 页面
    void refreshPage();
    // 修改资料
    void changeMaterial(const QByteArray& data);
    // 修改密码
    void changePassword(const QByteArray& data);
    // 清理信息
    void clearInfo();

signals:
    void loginAgainSignal();

private slots:
    void on_quit_login_clicked();
    void on_edit_material_clicked();
    void on_change_password_clicked();

private:
    Ui::MyInfo *ui;
    Common m_common;
    QNetworkAccessManager* m_manager;
    MyMenu* m_menu;

    int m_files_number;
    int m_share_files_number;
    QString m_phone;
    QString m_email;
};

#endif // MYINFO_H
