#include "changepassword.h"
#include "ui_changepassword.h"
#include "../common/logininfoinstance.h"

#include <QMessageBox>
#include <QJsonDocument>
#include <QRegExp>
ChangePassword::ChangePassword(QWidget *parent) : QWidget(parent), ui(new Ui::ChangePassword) {
    ui->setupUi(this);
    this->data = "";
}

ChangePassword::~ChangePassword() {
    delete ui;
}

void ChangePassword::setPasswordJson(QString account, QString oldPassword, QString newPassword){
    QMap<QString, QVariant> mmap;
    mmap.insert("account", account);
    mmap.insert("oldPassword", oldPassword);
    mmap.insert("newPassword", newPassword);

    QJsonDocument doc = QJsonDocument::fromVariant(mmap);
    if (doc.isNull()) {
        cout << "QJsonDocument::fromVariant() error";
        return;
    }

    this->data = doc.toJson();

}

QByteArray ChangePassword::getPasswordJson() const {
    return this->data;
}

void ChangePassword::clearData(){
    ui->old_password->clear();
    ui->new_password->clear();
    ui->is_new_password->clear();
}

void ChangePassword::closeEvent(QCloseEvent *event){
    emit finished();
    QWidget::closeEvent(event);
}

void ChangePassword::on_save_clicked() {
    QString oldPassword = ui->old_password->text().trimmed();
    QString newPassword = ui->new_password->text().trimmed();
    QString isNewPassword = ui->is_new_password->text().trimmed();

    if ("" == oldPassword || "" == newPassword || "" == isNewPassword) {
        QMessageBox::warning(this, "修改密码", "选项不能为空!!!");
        return;
    }

    QRegExp exp;
    exp.setPattern(PASSWORD_REG);
    if (!exp.exactMatch(oldPassword)) {
        QMessageBox::warning(this, "修改密码", "密码错误!!!");
        return;
    }

    if (!exp.exactMatch(newPassword)) {
        QMessageBox::warning(this, "修改密码", "密码格式错误!!!");
        return;
    }

    if (oldPassword == newPassword) {
        QMessageBox::warning(this, "修改密码", "新密码不能与旧密码相同!!!");
        return;
    }

    if (newPassword != isNewPassword) {
        QMessageBox::warning(this, "修改密码", "请输入相同的密码进行确认!!!");
        return;
    }


    LoginInfoInstance* login = LoginInfoInstance::getInstance();
    Common common;
    setPasswordJson(login->getAccount(), common.getStrMd5(oldPassword), common.getStrMd5(newPassword));
    clearData();
    this->close();
}


void ChangePassword::on_concel_clicked() {
    clearData();
    this->close();
}

