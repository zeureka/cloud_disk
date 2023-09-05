#include "frmlogin.h"
#include "ui_frmlogin.h"

#include "./common/logininfoinstance.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QMessageBox>
#include <QRegExp>

FrmLogin::FrmLogin(QWidget *parent) : QWidget(parent), ui(new Ui::FrmLogin) {
    ui->setupUi(this);
    this->index = new Index;
    ui->stackedWidget->setCurrentWidget(ui->login_page);
    setWindowTitle("登录");

    // 加载图片信息，显示文件列表的时候用，在此初始化
    this->m_common.getFileTypeList();

    bool success = connect(this->index, &Index::changeAccount, this, [=](){
        qDebug() << "Executing lambda function";
        this->index->hide();
        this->show();
    });
    if (!success) {
        qDebug() << "Failed to connect signal and slot";
    }
}

FrmLogin::~FrmLogin() {
    delete ui;
}

void FrmLogin::on_login_btn_clicked() {
    QString login_account = ui->login_account_le->text().trimmed();
    QString login_password = ui->login_password_le->text().trimmed();

    if ("" == login_account || "" == login_password) {
        QMessageBox::warning(this, "用户登录", "账号或密码不能为空!!!");
        ui->login_password_le->clear();
        return;
    }

    // 设置登录的JSON包，密码经过md5加密
    QByteArray loginData = setLoginJson(login_account, m_common.getStrMd5(login_password));

    // 发送 http 请求协议
    QNetworkAccessManager* manager = Common::getNetManager();

    // http 协议请求头
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));
    request.setHeader(QNetworkRequest::ContentLengthHeader, QVariant(loginData.size()));

    // 设置登录的url
    QString url = QString("http://%1:%2/login").arg(IP).arg(PORT);
    request.setUrl(QUrl(url));
    QNetworkReply* reply = manager->post(request, loginData);
    if (nullptr == reply) {
#if DEBUGPRINTF
        cout << "login reply is nullptr";
#endif
        return;
    }

#if DEBUGPRINTF
    cout << "post url: " << url;
    cout << "login post data: " << loginData;
#endif

    connect(reply, &QNetworkReply::finished, this, [=](){
        // 服务器接受出错了
        if (reply->error() != QNetworkReply::NoError) {
#if DEBUGPRINTF
            cout << reply->errorString();
#endif
            // 释放资源
            reply->deleteLater();
            return;
        }

        /**
         * 读取返回的数据
         * 成功: {"code": "000"}
         * 失败: {"code": "001"}
         */

        // 读出server回写的数据
        QByteArray jsonData = reply->readAll();

#if DEBUGPRINTF
        cout << "server return value: " << jsonData;
#endif

        QStringList infoList = getLoginStatus(jsonData);

        if ("000" == infoList.at(0)) {
            ui->login_account_le->clear();
            ui->login_password_le->clear();
#if DEBUGPRINTF
            cout << "登录成功";
#endif

            // 存储当前用户信息
            LoginInfoInstance* info = LoginInfoInstance::getInstance();
            info->setLoginInfo(login_account, infoList.at(1));

            // 隐藏当前窗口
            this->hide();
            // 打开云盘首页
            index->showMainWindow();
        } else if ("001" == infoList.at(1)) {
            QMessageBox::critical(this, "用户登录", "账号或密码不正确!!!");
        }

        reply->deleteLater();
    });
}


void FrmLogin::on_enroll_btn_clicked() {
    QString enroll_account = ui->enroll_account_le->text().trimmed();
    QString enroll_password = ui->enroll_password_le->text().trimmed();
    QString enroll_isPassword = ui->enroll_ispassword_le->text().trimmed();
    QString enroll_phone = ui->enroll_phone_le->text().trimmed();
    QString enroll_email = ui->enroll_email_le->text().trimmed();

    if ("" == enroll_account || "" == enroll_password || "" == enroll_isPassword || "" == enroll_phone || "" == enroll_email) {
        QMessageBox::warning(this, "用户注册", "注册选项不能为空!!!");
        clear_data();
        return;
    }

    if (enroll_password != enroll_isPassword) {
        QMessageBox::warning(this, "用户注册", "请输入相同的密码进行确认!!!");
        clear_data();
        return;
    }

    QRegExp exp;
    exp.setPattern(ACCOUNT_REG);
    if (!exp.exactMatch(enroll_account)) {
        QMessageBox::warning(this, "用户注册", "用户名格式错误!!!");
        clear_data();
        return;
    }

    exp.setPattern(PASSWORD_REG);
    if (!exp.exactMatch(enroll_password)) {
        QMessageBox::warning(this, "用户注册", "密码格式错误!!!");
        clear_data();
        return;
    }


    exp.setPattern(PHONE_REG);
    if (!exp.exactMatch(enroll_phone)) {
        QMessageBox::warning(this, "用户注册", "电话格式错误!!!");
        clear_data();
        return;
    }

    exp.setPattern(EMAIL_REG);
    if (!exp.exactMatch(enroll_email)) {
        QMessageBox::warning(this, "用户注册", "邮箱格式错误!!!");
        clear_data();
        return;
    }

    // 将注册信息打包成JSON数据
    QByteArray enrollData = setEnrollJson(enroll_account, this->m_common.getStrMd5(enroll_password), enroll_phone, enroll_email);

#if DEBUGPRINTF
    cout << "enroll json data: " << enrollData;
#endif

    // 发送 http 请求协议
    QNetworkAccessManager* manager = Common::getNetManager();

    // http 协议请求头
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));
    request.setHeader(QNetworkRequest::ContentLengthHeader, QVariant(enrollData.size()));

    // 设置注册url
    QString url = QString("http://%1:%2/enroll").arg(IP).arg(PORT);
    request.setUrl(QUrl(url));
    QNetworkReply* reply = manager->post(request, enrollData);
    if (nullptr == reply) {
#if DEBUGPRINTF
        cout << "enroll reply error";
#endif
        return;
    }

    connect(reply, &QNetworkReply::finished, [=](){
        /**
         * 读取返回的数据
         * 成功: {"code": "002"}
         * 用户已存在: {"code": "003"}
         * 失败: {"code": "004"}
         */

        QByteArray jsonData = reply->readAll();
        QString statusCode = m_common.getStatusCode(jsonData);

#if DEBUGPRINTF
        cout << "enroll jsonData = " << jsonData;
        cout << "enroll statusCode = " << statusCode;
#endif

        if ("002" == statusCode) {
            this->clear_data();
            QMessageBox::warning(this, "用户注册", "用户注册成功!!!");
            ui->stackedWidget->setCurrentWidget(ui->login_page);
            ui->login_account_le->setFocus();
            setWindowTitle("登录");
        } else if ("003" == statusCode) {
            QMessageBox::warning(this, "用户注册", QString("[%1] 用户名已存在!!!").arg(enroll_account));
        } else if ("004" == statusCode) {
            QMessageBox::critical(this, "用户注册", "注册失败!!!");
        }

        reply->deleteLater();
    });
}


void FrmLogin::on_to_login_btn_clicked() {
    this->clear_data();
    ui->stackedWidget->setCurrentWidget(ui->login_page);
    ui->login_account_le->setFocus();
    setWindowTitle("登录");
}


void FrmLogin::on_to_enroll_btn_clicked() {
    ui->stackedWidget->setCurrentWidget(ui->enroll_page);
    ui->enroll_account_le->setFocus();
    setWindowTitle("注册");
}


void FrmLogin::clear_data() {
    ui->enroll_account_le->clear();
    ui->enroll_password_le->clear();
    ui->enroll_ispassword_le->clear();
    ui->enroll_phone_le->clear();
    ui->enroll_email_le->clear();
    ui->enroll_account_le->setFocus();
}

QByteArray FrmLogin::setLoginJson(QString account, QString password) {
    QMap<QString, QVariant> loginJson;
    loginJson.insert("account", account);
    loginJson.insert("password", password);

    /**
     * json 数据
     * {
     * 		"account": "mio",
     * 		"password"" "xxxxx"
     * }
     */

    QJsonDocument jsonDoc = QJsonDocument::fromVariant(loginJson);
    if (jsonDoc.isNull()) {
#if DEBUGPRINTF
        cout << "jsonDoc.isNull() \n";
#endif
        return "";
    }

    return jsonDoc.toJson();
}

QByteArray FrmLogin::setEnrollJson(QString account, QString password, QString phone, QString email) {
    QMap<QString, QVariant> enrollJson;
    enrollJson.insert("account", account);
    enrollJson.insert("password", password);
    enrollJson.insert("phone", phone);
    enrollJson.insert("email", email);

    /**
     * json 数据
     * {
     * 		"account": "mio",
     * 		"password": "xxxxx",
     *		"phone": "xxxxx",
     *		"email": "xxx@xx.xx"
     * }
     */

    QJsonDocument jsonDoc = QJsonDocument::fromVariant(enrollJson);
    if ( jsonDoc.isNull() ) {
#if DEBUGPRINTF
        cout << " jsonDocument.isNull() \n";
#endif
        return "";
    }

    return jsonDoc.toJson();

}

QStringList FrmLogin::getLoginStatus(QByteArray loginStatus) {
    QJsonParseError error;
    QStringList list;

    // 将来源数据json转化为JsonDocument
    // 由QByteArray对象构造一个QJsonDocument对象，用于我们的读写操作
    QJsonDocument doc = QJsonDocument::fromJson(loginStatus, &error);
    if (error.error == QJsonParseError::NoError) {
        if (doc.isNull() || doc.isEmpty()) {
#if DEBUGPRINTF
            cout << "doc.isNull() || doc.isEmpty()";
#endif
            return list;
        }

        if( doc.isObject() ) {
            //取得最外层这个大对象
            QJsonObject obj = doc.object();

#if DEBUGPRINTF
            cout << "服务器返回的数据" << obj;
#endif

            //状态码
            list.append(obj.value("code").toString());
            //登陆token
            list.append(obj.value("token").toString());
        }
    } else {
#if DEBUGPRINTF
        cout << "err = " << error.errorString();
#endif
    }

    return list;
}
