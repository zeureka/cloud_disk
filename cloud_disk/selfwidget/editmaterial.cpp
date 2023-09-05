#include "editmaterial.h"
#include "ui_editmaterial.h"
#include "../common/logininfoinstance.h"

#include <QMessageBox>
#include <QRegExp>

EditMaterial::EditMaterial(QWidget *parent) : QWidget(parent), ui(new Ui::EditMaterial) {
    ui->setupUi(this);
    this->data = "";
}

EditMaterial::~EditMaterial() {
    delete ui;
}

void EditMaterial::setMaterialJson(QString account, QString phone, QString email) {
    QMap<QString, QVariant> mmap;
    mmap.insert("account", account);
    mmap.insert("phone", phone);
    mmap.insert("email", email);

    QJsonDocument doc = QJsonDocument::fromVariant(mmap);
    if (doc.isNull()) {
        cout << "set Material Json failed";
        return;
    }

    this->data = doc.toJson();
}

QByteArray EditMaterial::getMaterialJson() const {
    return this->data;
}

void EditMaterial::clearData(){
    ui->new_email->clear();
    ui->new_phone->clear();
}

void EditMaterial::closeEvent(QCloseEvent *event) {
    emit finished();
    QWidget::closeEvent(event);
}

void EditMaterial::on_save_clicked() {
    QString phone = ui->new_phone->text().trimmed();
    QString email = ui->new_email->text().trimmed();

    if ("" == phone || "" == email) {
        QMessageBox::warning(this, "编辑资料", "电话号码或邮箱地址不能为空!!!");
        return;
    }

    QRegExp exp;
    exp.setPattern(PHONE_REG);
    if (!exp.exactMatch(phone)) {
        QMessageBox::warning(this, "编辑资料", "电话号码格式错误!!!");
        return;
    }

    exp.setPattern(EMAIL_REG);
    if (!exp.exactMatch(email)) {
        QMessageBox::warning(this, "编辑资料", "邮箱地址格式错误!!!");
        return;
    }

    LoginInfoInstance* login = LoginInfoInstance::getInstance();
    setMaterialJson(login->getAccount(), phone, email);
    clearData();
    this->close();
}

void EditMaterial::on_concel_clicked() {
    clearData();
    this->close();
}
