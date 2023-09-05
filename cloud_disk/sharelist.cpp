#include "sharelist.h"
#include "ui_sharelist.h"
#include "common/common.h"
#include "selfwidget/filepropertyinfo.h"
#include "selfwidget/mymenu.h"
#include "common/logininfoinstance.h"
#include "common/downloadtask.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFileDialog>
#include <QMessageBox>


ShareList::ShareList(QWidget *parent) : QWidget(parent), ui(new Ui::ShareList) {
    ui->setupUi(this);

    initListWidget();
    addActionMenu();
    this->m_manager = Common::getNetManager();

    // 定时检查下载队列中是否有任务
    connect(&this->m_downloadTimer, &QTimer::timeout, this, [=](){
        downloadFilesAction();
    });

    // 启动定时器, 每500ms检查一次
    this->m_downloadTimer.start(500);

    refreshFiles();
}

ShareList::~ShareList() {
    // 清空文件列表
    clearShareFileList();

    // 清空所有Item文件
    clearItems();

    delete ui;
}

// 初始化listWidget属性
void ShareList::initListWidget() {
    ui->listWidget->setViewMode(QListView::ListMode);
    // 统一项目大小
    ui->listWidget->setUniformItemSizes(true);
    // 设置QListWidget的调整模式为adjust, 自适应布局
    ui->listWidget->setResizeMode(QListView::Adjust);
    // 设置item宽度和高度
    ui->listWidget->setIconSize(QSize(ui->listWidget->width() - 20, 50));
    // 设置列表固定不能拖动
    ui->listWidget->setMovement(QListView::Static);
    // 设置图标之间的间距
    ui->listWidget->setSpacing(10);

    // listItem 右键菜单
    ui->listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->listWidget, &QListWidget::customContextMenuRequested, this, [=](const QPoint& pos){
        QListWidgetItem* item = ui->listWidget->itemAt(pos);
        // 没有点击Item
        if (nullptr == item) {
            // 弹出菜单
            this->m_menuEmpty->exec(QCursor::pos());
        } else {
            ui->listWidget->setCurrentItem(item);
            this->m_menuItem->exec(QCursor::pos());
        }
    });
}

// 添加菜单项
void ShareList::addActionMenu() {
    // ===========> 点击Item时出现的菜单 <==============
    this->m_menuItem = new MyMenu(this);
    QList<QAction*> list1;

    list1.append(new QAction("下载", this));
    list1.append(new QAction("属性", this));
    list1.append(new QAction("取消分享", this));
    list1.append(new QAction("转存文件", this));

    this->m_menuItem->addActions(list1);

    // ===========> 点击空白出时出现的菜单 <============
    this->m_menuEmpty = new MyMenu(this);
    QList<QAction*> list2;

    list2.append(new QAction("刷新", this));
    this->m_menuEmpty->addActions(list2);

    // ===========> 信号与槽 <==============
    connect(list1.at(0), &QAction::triggered, this, [=](){
#if DEBUGPRINTF
        cout << "下载动作";
#endif
        addDownloadFiles();
    });

    connect(list1.at(1), &QAction::triggered, this, [=](){
#if DEBUGPRINTF
        cout << "属性动作";
#endif
        dealSelectedFile(CMD::Property);
    });

    connect(list1.at(2), &QAction::triggered, this, [=](){
#if DEBUGPRINTF
        cout << "取消分享";
#endif
        dealSelectedFile(CMD::Cancel);
    });

    connect(list1.at(3), &QAction::triggered, this, [=](){
#if DEBUGPRINTF
        cout << "转存文件";
#endif
        dealSelectedFile(CMD::Save);
    });

    connect(list2.at(0), &QAction::triggered, this, [=](){
#if DEBUGPRINTF
        cout << "刷新动作";
#endif
        refreshFiles();
    });
}

// 清空分享文件列表
void ShareList::clearShareFileList() {
    int n = this->m_shareFileList.size();
    for (int i = 0; i < n; ++i) {
        FileInfo* tmp = this->m_shareFileList.takeFirst();
        delete tmp;
        tmp = nullptr;
    }
}

// 清空所有item项目
void ShareList::clearItems() {
    int n = ui->listWidget->count();
    for (int i = 0; i < n; ++i) {
        QListWidgetItem* item = ui->listWidget->takeItem(0);
        delete item;
        item = nullptr;
    }
}

// item文件展示
void ShareList::refreshFileItems() {
    clearItems();
    int n = this->m_shareFileList.size();
    for (int i = 0; i < n; ++i) {
        FileInfo* tmp = this->m_shareFileList.at(i);
        QListWidgetItem* item = tmp->item;
        ui->listWidget->addItem(item);
    }
}

// 显示共享的文件列表
void ShareList::refreshFiles() {
    // ===========> 获取共享文件数目 <===========
    this->m_accountFilesCount = 0;
    QString url = QString("http://%1:%2/sharefiles?cmd=count").arg(IP).arg(PORT);
    QNetworkReply* reply = this->m_manager->get(QNetworkRequest(QUrl(url)));

    if (nullptr == reply) {
#if DEBUGPRINTF
        cout << "refresh files reply is nullptr";
#endif
        return;
    }

    // 获取请求的数据完成时，发送信号finished()
    connect(reply, &QNetworkReply::finished, this, [=](){
        if (QNetworkReply::NoError != reply->error()) {
#if DEBUGPRINTF
            cout << "refresh files reply error; error = " << reply->errorString();
#endif
            reply->deleteLater();
            return;
        }

        // 服务器返回用户分享文件个数
        QByteArray array = reply->readAll();
        reply->deleteLater();

        // 清空文件列表信息
        clearShareFileList();

        this->m_accountFilesCount = array.toLong();
        if (this->m_accountFilesCount > 0) {
            // 说明有共享文件
            // 从第0个开始取，每次取10个
            this->m_start = 0;
            this->m_count = 10;
            getAccountFilesList();
        } else {
            refreshFileItems();
        }
    });
}

// 设置JSON包
QByteArray ShareList::setFilesListJson(int start, int count) {
    QMap<QString, QVariant> mmap;
    mmap.insert("start", start);
    mmap.insert("count", count);

    QJsonDocument doc = QJsonDocument::fromVariant(mmap);
    if (doc.isNull()) {
#if DEBUGPRINTF
        cout << "set Files List Json failed";
#endif
        return {};
    }

    return doc.toJson();
}

// 获取共享文件列表
void ShareList::getAccountFilesList() {
    // 遍历数目，递归结束条件处理
    if (this->m_accountFilesCount <= 0) {
#if DEBUGPRINTF
        cout << "获取用户文件列表条件结束";
#endif
        refreshFileItems();
        return;
    } else if (this->m_count > this->m_accountFilesCount) {
        // 如果请求的文件数量大于用户的文件数量
        this->m_count = this->m_accountFilesCount;
    }

    QNetworkRequest request;

    QString url = QString("http://%1:%2/sharefiles?cmd=normal").arg(IP).arg(PORT);
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QByteArray data = setFilesListJson(this->m_start, this->m_count);

    // 修改文件起点位置
    this->m_start += this->m_count;
    this->m_accountFilesCount -= this->m_count;

    QNetworkReply* reply = this->m_manager->post(request, data);
    if (nullptr == reply) {
#if DEBUGPRINTF
        cout << "get share files list reply is nullptr";
#endif
        return;
    }

    // 获取请求数据完成时，发送信号finished()
    connect(reply, &QNetworkReply::finished, this, [=](){
        if (QNetworkReply::NoError != reply->error()) {
#if DEBUGPRINTF
            cout << "get share file list error; error = " << reply->errorString();
#endif
            reply->deleteLater();
            return;
        }

        QByteArray array = reply->readAll();
        reply->deleteLater();

        // 没有错误
        if ("015" != this->m_common.getStatusCode(array)) {
            // 处理文件列表的json信息
            getFileJsonInfo(array);
            // 获取共享文件列表
            getAccountFilesList();
        }
    });
}

// 解析文件列表JSON信息，存放在文件列表中
void ShareList::getFileJsonInfo(QByteArray data) {
    /**
     * {
     *   "account": "mio",
     *   "md5": "xxx",
     *   "create_time": "xxxx-xx-xx xx:xx:xx",
     *   "file_name": "test.png",
     *   "share_status": 0,
     *   "downloads": 0,
     *   "url": "http://IP:PORT/group1/M00/00/00/xxx.png",
     *   "size": xxxx,
     *   "type": "png"
     * }
     */
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (QJsonParseError::NoError == error.error) {
        if (doc.isNull() || doc.isEmpty()) {
#if DEBUGPRINTF
            cout << "fileJsonInfo is null or is empty";
#endif
            return;
        }

        if (doc.isObject()) {
            // 
            QJsonObject obj = doc.object();

            QJsonArray array = obj.value("files").toArray();
            int size = array.size();
#if DEBUGPRINTF
            cout << "share file size = " << size;
#endif

            for (int i = 0; i < size; ++i) {
                QJsonObject tmp = array[i].toObject();
                FileInfo* info = new FileInfo;
                info->account = tmp.value("account").toString();
                info->md5 = tmp.value("md5").toString();
                info->createTime = tmp.value("create_time").toString();
                info->fileName = tmp.value("file_name").toString();
                info->shareStatus = tmp.value("share_status").toInt();
                info->downloads = tmp.value("downloads").toInt();
                info->url = tmp.value("url").toString();
                info->size = tmp.value("size").toInt();
                info->type = tmp.value("type").toString();
                info->item = new QListWidgetItem(QIcon(this->m_common.getFileType(info->type + ".png")), info->fileName);

                // 
                this->m_shareFileList.append(info);
            }
        }
    } else {
#if DEBUGPRINTF
        cout << "get file json info error, error = " << error.errorString();
#endif
    }
}

// 添加需要下载的文件到下载任务队列中
void ShareList::addDownloadFiles() {
    QListWidgetItem* item = ui->listWidget->currentItem();
    if (nullptr == item) {
#if DEBUGPRINTF
        cout << "current item is nullptr";
#endif
        return;
    }

    // 跳转到传输文件的下载页面
    emit gotoTransfer(TransferStatus::Download);

    DownloadTask* dt = DownloadTask::getInstance();
    if (nullptr == dt) {
#if DEBUGPRINTF
        cout << "DownloadTask::getInstance() is nullptr";
#endif
        return;
    }

    int n = this->m_shareFileList.size();
    for (int i = 0; i < n; ++i) {
        if (item == this->m_shareFileList.at(i)->item) {
            QString fileSavePath = QFileDialog::getSaveFileName(this, "选择保存文件路径", this->m_shareFileList.at(i)->fileName);
            if (fileSavePath.isEmpty()) {
#if DEBUGPRINTF
                cout << "save file Path is empty";
#endif
                return;
            }

            /**
             * 下载文件：
             * 成功：{"code":"009"}
             * 失败：{"code":"010"}

             * 追加任务到下载队列
             * 参数：info：下载文件信息， filePathName：文件保存路径
             * 成功：0
             * 失败：
             *  -1: 下载的文件是否已经在下载队列中
             *  -2: 打开文件失败
             */

            int res = dt->appendDownloadList(this->m_shareFileList.at(i), fileSavePath, true);
            if (-1 == res) {
                QMessageBox::information(this, "下载文件", "该文件已在下载队列中!!!");
            } else if (-2 == res) {
                LoginInfoInstance* login = LoginInfoInstance::getInstance();
                // 记录下载失败日志
                this->m_common.writeRecord(login->getAccount(), this->m_shareFileList.at(i)->fileName, "010");
            }

            break;
        }
    }
}

// 下载任务处理，取出下载队列的队首任务，下载完后，再取下一个任务
void ShareList::downloadFilesAction(){
    DownloadTask* dt = DownloadTask::getInstance();
    if (nullptr == dt) {
#if DEBUGPRINTF
        cout << "DownloadTask::getInstance() is nullptr";
#endif
        return;
    }

    // 没有任务，exit
    if (dt->isEmpty()) {
        return;
    }

    // 同时只能有一个下载任务， exit
    if (dt->isDownload()) {
        return;
    }

    // 必须是共享任务才能下载，否则 exit
    if (!dt->isShareTask()) {
        return;
    }

    // 取出第一个任务
    DownloadInfo* tmp = dt->takeTask();
    QUrl url = tmp->url;
    QFile* file = tmp->file;
    QString md5 = tmp->md5;
    QString fileName = tmp->fileName;
    DataProgress *dp = tmp->dp;

    // 发送get请求
    QNetworkReply* reply = this->m_manager->get(QNetworkRequest(url));
    if (nullptr == reply) {
        // dt->dealDownloadTask();
#if DEBUGPRINTF
        cout << "get download file reply error, error = ";
#endif
        return;
    }

    // 获取请求数据完成时，发送信号finished()
    connect(reply, &QNetworkReply::finished, this, [=](){
#if DEBUGPRINTF
        cout << QString("[%1] 下载完成").arg(fileName);
#endif
        reply->deleteLater();
        dt->dealDownloadTask();

        // 下载文件成功，记录日志
        LoginInfoInstance* login = LoginInfoInstance::getInstance();
        this->m_common.writeRecord(login->getAccount(), fileName, "010");
        dealFileDownloads(md5, fileName);
    });

    // 当有可用数据时，reply发出readyRead()信号
    connect(reply, &QNetworkReply::readyRead, this, [=](){
        // 如果文件存在，则写入文件
        if (nullptr != file) {
            file->write(reply->readAll());
        }
    });

    // 可用数据更新时
    connect(reply, &QNetworkReply::downloadProgress, this, [=](qint64 bytesRead, qint64 totalBytes){
        // 设置进度条
#if DEBUGPRINTF
        cout << bytesRead << " --- " << totalBytes;
#endif
        dp->setProgressValue(bytesRead, totalBytes);
    });
}

// 设置JSON包
QByteArray ShareList::setShareFileJson(QString account, QString md5, QString fileName) {
    QMap<QString, QVariant> mmap;
    mmap.insert("account", account);
    mmap.insert("md5", md5);
    mmap.insert("fileName", fileName);

    QJsonDocument doc = QJsonDocument::fromVariant(mmap);
    if (doc.isNull()) {
#if DEBUGPRINTF
        cout << "set Share File Json failed";
#endif
        return {};
    }

    return doc.toJson();
}

// 处理下载downloads字段
void ShareList::dealFileDownloads(QString md5, QString fileName) {
    LoginInfoInstance* login = LoginInfoInstance::getInstance();
    QNetworkRequest request;
    QString url = QString("http://%1:%2/dealsharefile?cmd=downloads").arg(IP).arg(PORT);
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 设置JSON包
    QByteArray data = setShareFileJson(login->getAccount(), md5, fileName);

    // 发送post请求
    QNetworkReply* reply = this->m_manager->post(request, data);
    if (nullptr == reply) {
#if DEBUGPRINTF
        cout << "deal file downloads reply is nullptr";
#endif
        return;
    }

    // 获取请求数据完成时，发送信号finished()
    connect(reply, &QNetworkReply::finished, this, [=](){
        if (QNetworkReply::NoError != reply->error()) {
#if DEBUGPRINTF
            cout << "deal file downloads reply error; error = " << reply->errorString();
#endif
            reply->deleteLater();
            return;
        }

        QByteArray array = reply->readAll();
        reply->deleteLater();

        /**
         * 下载文件downloads字段处理
         * 成功：{"code":"016"}
         * 失败：{"code":"017"}
         */

        if ("016" == this->m_common.getStatusCode(array)) {
            int n = this->m_shareFileList.size();
            for (int i = 0; i < n; ++i) {
                FileInfo* info = this->m_shareFileList.at(i);
                if (info->md5 == md5 && info->fileName == fileName) {
                    int downloads = info->downloads;
                    info->downloads = downloads + 1;
                    break;
                }
            }
        } else {
#if DEBUGPRINTF
            cout << "下载文件downloads字段处理失败";
#endif
        }
    });
}

// 处理选中的文件
void ShareList::dealSelectedFile(CMD cmd) {
    // 获取当前选中的Item
    QListWidgetItem* item = ui->listWidget->currentItem();
    if (nullptr == item) {
        return;
    }

    int size = this->m_shareFileList.size();
    for (int i = 0; i < size; ++i) {
        if (item == this->m_shareFileList.at(i)->item) {
            if (CMD::Property == cmd) {
                getFileProperty(this->m_shareFileList.at(i));
            } if (CMD::Cancel == cmd) {
                CancelShareFile(this->m_shareFileList.at(i));
            } if (CMD::Save == cmd) {
                SaveFileToMyList(this->m_shareFileList.at(i));
            }
            break;
        }
    }
}

// 获取文件属性
void ShareList::getFileProperty(FileInfo* info) {
    // 创建对话框
    FilePropertyInfo dlg;
    dlg.setInfo(info);

    // 模态显示
    dlg.exec();
}

// 取消已经分享的文件
void ShareList::CancelShareFile(FileInfo* info) {
    LoginInfoInstance* login = LoginInfoInstance::getInstance();

    // 如果文件的拥有着不是此次登录的用户，则无法取消分享
    if (login->getAccount() != info->account) {
        QMessageBox::warning(this, "取消分享", "权限不足，你不是这个文件的拥有者！！！");
        return;
    }

    QNetworkRequest request;
    QString url = QString("http://%1:%2/dealsharefile?cmd=cancel").arg(IP).arg(PORT);
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    /**
     * "account": "mio",
     * "md5": "xxx",
     * "fileName": "xxx"
     */

    QByteArray data = setShareFileJson(login->getAccount(), info->md5, info->fileName);

    // 发送post请求
    QNetworkReply* reply = this->m_manager->post(request, data);
    if (nullptr == reply) {
#if DEBUGPRINTF
        cout << "cancel share file reply is nullptr";
#endif
        return;
    }

    // 获取请求的数据完成时，发送信号finished()
    connect(reply, &QNetworkReply::finished, this, [=](){
        if (QNetworkReply::NoError != reply->error()) {
#if DEBUGPRINTF
            cout << "cancel share file reply error, Error = " << reply->errorString();
#endif
            reply->deleteLater();
            return;
        }

        QByteArray array = reply->readAll();
        reply->deleteLater();

        /**
         * 取消分享：
         * 成功：{"code":"018"}
         * 失败：{"code":"019"}
         */

        if ("018" == this->m_common.getStatusCode(array)) {
            // 取消分享成功，将此文件和此文件Item移除
            int n = this->m_shareFileList.size();
            for (int i = 0; i < n; ++i) {
                if (info == this->m_shareFileList.at(i)) {
                    QListWidgetItem* item = info->item;
                    // 从视图中移除Item
                    ui->listWidget->removeItemWidget(item);
                    delete item;

                    // 从文件列表中删除此文件
                    this->m_shareFileList.removeAt(i);
                    delete info;

                    break;
                }
            }
            QMessageBox::information(this, "取消分享", "此文件已成功取消分享！！！");
        } else {
            QMessageBox::warning(this, "取消分享", "取消分享文件操作失败！！！");
        }
    });
}

// 转存文件
void ShareList::SaveFileToMyList(FileInfo* info) {
    LoginInfoInstance* login = LoginInfoInstance::getInstance();

    if (info->account == login->getAccount()) {
        QMessageBox::information(this, "转存文件", "你是此文件的拥有者，无需转存！！！");
        return;
    }

    QNetworkRequest request;
    QString url = QString("http://%1:%2/dealsharefile?cmd=save").arg(IP).arg(PORT);
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 发送post请求
    QByteArray data = setShareFileJson(login->getAccount(), info->md5, info->fileName);
    QNetworkReply* reply = this->m_manager->post(request, data);
    if (nullptr == reply) {
#if DEBUGPRINTF
        cout << "save file to my lit error";
#endif
        return;
    }

    // 获取请求数据完成时，发送信号finished()
    connect(reply, &QNetworkReply::finished, this, [=](){
        if (QNetworkReply::NoError != reply->error()) {
#if DEBUGPRINTF
            cout << "save file to my list error, Error = " << reply->errorString();
#endif
            reply->deleteLater();
            return;
        }

        QByteArray array = reply->readAll();
        reply->deleteLater();

        /**
         * 转存文件：
         * 成功：{"code":"020"}
         * 文件已存在：{"code":"021"}
         * 失败：{"code":"022"}
         */

        if ("020" == this->m_common.getStatusCode(array)) {
            QMessageBox::information(this, "转存文件", "成功保存此文件！！！");
        } else if ("021" == this->m_common.getStatusCode(array)) {
            QMessageBox::warning(this, "转存文件", "保存文件失败，你已拥有此文件，无需转存！！！");
        } else if ("022" == this->m_common.getStatusCode(array)) {
            QMessageBox::warning(this, "转存文件", "保存文件失败！！！");
        }
    });
}
