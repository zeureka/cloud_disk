#include "myinfo.h"
#include "ui_myinfo.h"
#include "common/logininfoinstance.h"
#include "selfwidget/changepassword.h"
#include "selfwidget/editmaterial.h"

#include <QMap>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QMessageBox>
#include <QTimer>

MyInfo::MyInfo(QWidget *parent) : QWidget(parent), ui(new Ui::MyInfo) {
    ui->setupUi(this);

    // 初始化数据
    this->m_manager = Common::getNetManager();
    this->m_files_number = 0;
    this->m_share_files_number = 0;
    this->m_phone = "";
    this->m_email = "";

    addActionMenu();
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, [=](){
        this->m_menu->exec(QCursor::pos());
    });

//    QTimer* timer = new QTimer;
//    QMetaObject::Connection connection = connect(timer, &QTimer::timeout, [this, timer, &connection](){
//        cout << LoginInfoInstance::getInstance()->getAccount();
//        if (LoginInfoInstance::getInstance()->getAccount() != "") {
//            this->getMyInfo();
//            timer->stop();
//            delete timer;
//            disconnect(connection);
//        }
//    });
//    timer->start(1000);
}

MyInfo::~MyInfo() {
    delete ui;
}

void MyInfo::addActionMenu(){
    this->m_menu = new MyMenu(this);
    QList<QAction*> list;
    list.append(new QAction("刷新", this));
    this->m_menu->addActions(list);
    connect(list.at(0), &QAction::triggered, this, [=](){
#if DEBUGPRINTF
        cout << "刷新动作";
#endif
        getMyInfo();
    });
}

void MyInfo::getMyInfo() {
    LoginInfoInstance* login = LoginInfoInstance::getInstance();

    // 设置 account json 数据
    QMap<QString, QVariant> mmap;
    mmap.insert("account", login->getAccount());
    QJsonDocument doc = QJsonDocument::fromVariant(mmap);
    if (doc.isNull()) {
#if DEBUGPRINTF
        cout << "set my info json failed";
#endif
        return;
    }

    QByteArray data = doc.toJson();

    QNetworkRequest request;
    QString url = QString("http://%1:%2/myinfo?cmd=info").arg(IP).arg(PORT);
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = this->m_manager->post(request, data);

    if (nullptr == reply) {
#if DEBUGPRINTF
        cout << "get my info reply is nullptr";
#endif
        return;
    }

    // 获取请求数据完成时，发送信号finished()
    connect(reply, &QNetworkReply::finished, this, [=](){
        if (QNetworkReply::NoError != reply->error()) {
#if DEBUGPRINTF
            cout << "get my info failed, error = " << reply->errorString();
#endif
            reply->deleteLater();
            return;
        }

        QByteArray info = reply->readAll();
        reply->deleteLater();
#if DEBUGPRINTF
        cout << "my info: " << info.data();
#endif

        /**
         * 获取我的信息
         * 成功: 返回数据
         * 失败: {"code" : "028"}
        */

        if ("028" != this->m_common.getStatusCode(info)) {
            QJsonParseError error;
            QJsonDocument doc = QJsonDocument::fromJson(info, &error);

            if (QJsonParseError::NoError == error.error) {
                if (doc.isNull() || doc.isEmpty()) {
#if DEBUGPRINTF
                    cout << "my info is null or empty";
#endif
                    return;
                }
                if (doc.isObject()) {
                    // 获取 my info 的 JSON对象
                    QJsonObject obj = doc.object();
                    this->m_files_number = obj.value("files_number").toInt();
                    this->m_share_files_number = obj.value("share_files_number").toInt();
                    this->m_phone = obj.value("phone").toString();
                    this->m_email = obj.value("email").toString();
                }
                refreshPage();
            }
        }
    });
}

void MyInfo::refreshPage() {
    ui->account->setText(LoginInfoInstance::getInstance()->getAccount());
    ui->my_files_number->setText(QString::number(this->m_files_number));
    ui->my_share_file_number->setText(QString::number(this->m_share_files_number));
    ui->my_phone->setText(this->m_phone);
    ui->my_email->setText(this->m_email);
}

void MyInfo::changeMaterial(const QByteArray& data) {
    /**
     * data format
     * {
     * 		"acccount" : "mio",
     * 		"phone" : "xxxx",
     * 		"email" : "xxxx"
     * }
    */

    QNetworkRequest request;
    QString url = QString("http://%1:%2/myinfo?cmd=changematerial").arg(IP).arg(PORT);
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = this->m_manager->post(request, data);
    if (nullptr == reply) {
#if DEBUGPRINTF
        cout << "Change Material reply is nullptr";
#endif
        return;
    }

    // 获取请求数据完成时，发送信号finished()
    connect(reply, &QNetworkReply::finished, this, [=](){
        if (QNetworkReply::NoError != reply->error()) {
#if DEBUGPRINTF
            cout << "change material faile, error = " << reply->errorString();
#endif
            reply->deleteLater();
            return;
        }

        QByteArray info = reply->readAll();
        reply->deleteLater();

        /**
         * 编辑资料
         * 成功: {"code": "023"}
         * 失败: {"code": "024"}
        */

        if ("023" == this->m_common.getStatusCode(info)) {
            QMessageBox::information(this, "编辑资料", "资料修改成功!!!");
            getMyInfo();
        } else if ("024" == this->m_common.getStatusCode(info)) {
            QMessageBox::warning(this, "编辑资料", "资料修改失败!!!");
        }
    });
}

void MyInfo::changePassword(const QByteArray& data) {
    /**
     * data format
     * {
     * 		"account" : "mio",
     * 		"oldPassword" : "xxxx",
     * 		"newPassword" : "xxxx"
     * }
     */

    QNetworkRequest request;
    QString url = QString("http://%1:%2/myinfo?cmd=changepassword").arg(IP).arg(PORT);
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = this->m_manager->post(request, data);
    if (nullptr == reply) {
#if DEBUGPRINTF
        cout << "Change password is nullptr";
#endif
        return;
    }

    // 获取请求数据完成时，发送信号finished()
    connect(reply, &QNetworkReply::finished, this, [=](){
        if (QNetworkReply::NoError != reply->error()) {
#if DEBUGPRINTF
            cout << "change password reply failed, error = " << reply->errorString();
#endif
            reply->deleteLater();
            return;
        }

        QByteArray info = reply->readAll();
        reply->deleteLater();

        /**
         * 修改密码
         * 成功: {"code" : "025"}
         * 原密码错误: {"code" : "026"}
         * 失败: {"code" : "027"}
        */

        if ("025" == this->m_common.getStatusCode(info)) {
            QMessageBox::information(this, "修改密码", "密码修改成功，请重新登录");
            emit loginAgainSignal();
        } else if ("026" == this->m_common.getStatusCode(info)) {
            QMessageBox::warning(this, "修改密码", "原密码错误!!!");
        } else if ("027" == this->m_common.getStatusCode(info)) {
            QMessageBox::warning(this, "修改密码", "服务器错误，修改密码失败!!!");
        }
    });
}

void MyInfo::clearInfo(){
    ui->account->clear();
    ui->my_files_number->clear();
    ui->my_share_file_number->clear();
    ui->my_phone->clear();
    ui->my_email->clear();
}

void MyInfo::on_quit_login_clicked() {
#if DEBUGPRINTF
    cout << "Emitting loginAgainSignal";
#endif
    emit loginAgainSignal();
}

void MyInfo::on_edit_material_clicked() {
    EditMaterial* win = new EditMaterial;
    QEventLoop loop;
    QByteArray data;

    // 循环检测窗口是否关闭
    auto connection = connect(win, &EditMaterial::finished, &loop, [&loop, &data, &win](){
        const QByteArray tmp = win->getMaterialJson();
        data = tmp;
        delete win;
        win = nullptr;
        loop.quit();
    });

    win->show();
    loop.exec();

    // 销毁信号槽
    disconnect(connection);

    // 没做修改，直接结束此操作
    if ("" == data) {
        return;
    }

    changeMaterial(data);
}

void MyInfo::on_change_password_clicked() {
    ChangePassword* win = new ChangePassword;
    QEventLoop loop;
    QByteArray data;

    // 循环检测窗口是否关闭
    auto connection = connect(win, &ChangePassword::finished, &loop, [&loop, &data, &win](){
        const QByteArray tmp = win->getPasswordJson();
        data = tmp;
        delete win;
        win = nullptr;
        loop.quit();
    });

    win->show();
    loop.exec();

    // 销毁信号槽
    disconnect(connection);

    // 没做修改，直接结束此操作
    if ("" == data) {
        return;
    }

    changePassword(data);
}

