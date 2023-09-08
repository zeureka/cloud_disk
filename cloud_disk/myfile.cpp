#include "myfile.h"
#include "ui_myfile.h"
#include "common/common.h"
#include "common/downloadtask.h"
#include "common/logininfoinstance.h"
#include "selfwidget/filepropertyinfo.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

MyFile::MyFile(QWidget *parent) : QWidget(parent), ui(new Ui::MyFile) {
    ui->setupUi(this);

    this->m_manager = Common::getNetManager();
    initListWidget();
    addActionMenu();
    checkTaskList();

    refreshFiles();
}

MyFile::~MyFile() {
    delete ui;
}

void MyFile::initListWidget() { // 设置图标显示模式
    setItemViewMode();
    auto connection = [=](QListWidget* listWidget){
        // listItem 右键菜单
        listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(listWidget, &QListWidget::customContextMenuRequested, this, &MyFile::rightMenu);
    };

    connection(ui->total);
    connection(ui->vedio);
    connection(ui->audio);
    connection(ui->picture);
    connection(ui->book);
    connection(ui->document);
}

void MyFile::addActionMenu() {
    // =================> 菜单1 <===================
    // 右键单击文件时出现
    this->m_menuItem = new MyMenu(this);
    QList<QAction*> list1;

    // 初始化菜单项
    list1.append(new QAction("下载", this));
    list1.append(new QAction("删除", this));
    list1.append(new QAction("分享", this));
    list1.append(new QAction("属性", this));

    // 将菜单项添加到菜单中
    this->m_menuItem->addActions(list1);

    // =================> 菜单1 <===================
    // 右键单击空白处时出现
    this->m_menuEmpty = new MyMenu(this);
    QList<QAction*> list2;

    // 初始化菜单项
    list2.append(new QAction("按下载量升序", this));
    list2.append(new QAction("按下载量降序", this));
    list2.append(new QAction("刷新", this));
    list2.append(new QAction("上传", this));

    // 将菜单项添加到菜单就中
    this->m_menuEmpty->addActions(list2);

    // =========> 信号与槽 <===========
    connect(list1.at(0), &QAction::triggered, this, [=](){
#if DEBUGPRINTF
        cout << "下载动作";
#endif
        // 将选中的文件添加到下载队列中
        addDownloadFiles();
    });

    connect(list1.at(1), &QAction::triggered, this, [=](){
#if DEBUGPRINTF
        cout << "删除动作";
#endif
        // 处理选中的文件
        dealSelectFile("delete");
    });

    connect(list1.at(2), &QAction::triggered, this, [=](){
#if DEBUGPRINTF
        cout << "分享动作";
#endif
        // 处理选中的文件
        dealSelectFile("share");
    });

    connect(list1.at(3), &QAction::triggered, this, [=](){
#if DEBUGPRINTF
        cout << "属性动作";
#endif
        // 处理选中的文件
        dealSelectFile("property");
    });

    connect(list2.at(0), &QAction::triggered, this, [=](){
#if DEBUGPRINTF
        cout << "按下载量升序";
#endif
        // 按选中的格式刷新
        refreshFiles(Display::DownloadsAsc);
    });

    connect(list2.at(1), &QAction::triggered, this, [=](){
#if DEBUGPRINTF
        cout << "按下载量降序";
#endif
        // 按选中的格式刷新
        refreshFiles(Display::DownloadsDesc);
    });

    connect(list2.at(2), &QAction::triggered, this, [=](){
#if DEBUGPRINTF
        cout << "刷新动作";
#endif
        // 显示用户的文件列表
        refreshFiles();
    });

    connect(list2.at(3), &QAction::triggered, this, [=](){
#if DEBUGPRINTF
        cout << "上传文件动作";
#endif
        // 添加文件到上传队列中
        addUploadFiles();
    });
}

void MyFile::setItemViewMode(QListView::ViewMode mode) {
    auto setViewMode = [](QListWidget* listWidget, QListView::ViewMode mode){
        listWidget->setViewMode(mode);
        if (QListView::ListMode == mode) {
            listWidget->setIconSize(QSize(listWidget->width() - 10, 50));
            listWidget->setSpacing(5);
        } else {
            listWidget->setIconSize(QSize(50, 50));
            listWidget->setSpacing(15);
        }

        // 统一项目大小
        listWidget->setUniformItemSizes(true);
        // 设置QListWidget的调整模式为adjust, 自适应布局
        listWidget->setResizeMode(QListView::Adjust);
        // 设置列表固定不能拖动
        listWidget->setMovement(QListView::Static);
        listWidget->show();
    };

    setViewMode(ui->total, mode);
    setViewMode(ui->vedio, mode);
    setViewMode(ui->audio, mode);
    setViewMode(ui->picture, mode);
    setViewMode(ui->book, mode);
    setViewMode(ui->document, mode);
}

void MyFile::addUploadFiles() {
    emit gotoTransfer(TransferStatus::Uplaod);
    UploadTask* uploadList = UploadTask::getInstance();

    if (nullptr == uploadList) {
#if DEBUGPRINTF
        cout << "UploadTask::getInstance() == nullptr";
#endif
        return;
    }

    // 弹出一的对话框，让用户选择一个文件，返回绝对路径或空字符串
    QStringList list = QFileDialog::getOpenFileNames();
    for (int i = 0; i < list.size(); ++i) {
        int res = uploadList->appendUploadList(list.at(i));
#if DEBUGPRINTF
        cout << "addUploadList file path=" << list.at(i);
#endif
        if (-1 == res) {
            QMessageBox::warning(this, "上传文件", "文件大小不能超过100M!!!");
        } else if (-2 == res) {
            QMessageBox::warning(this, "上传文件", "该文件已经在上传队列中!!!");
        } else if (-3 == res) {
            QMessageBox::warning(this, "上传文件", "打开文件失败!!!");
        } else if (-4 == res) {
            QMessageBox::warning(this, "上传文件", "系统错误!!!");
#if DEBUGPRINTF
            cout << "上传文件时，获取布局失败";
#endif
        }
    }
}

QByteArray MyFile::setMd5Json(QString account, QString token, QString md5, QString fileName) {
    QMap<QString, QVariant> mmap;
    mmap.insert("account", account);
    mmap.insert("token", token);
    mmap.insert("md5", md5);
    mmap.insert("fileName", fileName);

    QJsonDocument jsonDocument = QJsonDocument::fromVariant(mmap);
    if (jsonDocument.isNull()) {
#if DEBUGPRINTF
        cout << "(account, token, md5, fileName), jsonDocument.isNull()";
#endif
            return {};
    }

#if DEBUGPRINTF
    cout << jsonDocument.toJson().data();
#endif
    return jsonDocument.toJson();
}

void MyFile::uploadFileAction() {
    UploadTask* uploadList = UploadTask::getInstance();
    if (nullptr == uploadList) {
#if DEBUGPRINTF
        cout << "UploadTask::getInstance() is nullptr";
#endif
        return;
    }

    // 如果上传队列为空，则无法上传任务，终止程序
    if (uploadList->isEmpty()) {
        return;
    }

    // 查看是否有上传任务，但任务上传，当前有任务则不能上传
    if (uploadList->isUpload()) {
        return;
    }

    // 获取登录信息实例
    LoginInfoInstance* login = LoginInfoInstance::getInstance();

    QNetworkRequest request;
    QString url = QString("http://%1:%2/md5").arg(IP).arg(PORT);
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 取出第0个上传任务，如果任务队列中没有任务，则设置第0个任务
    UploadFileInfo* info = uploadList->takeTask();
#if DEBUGPRINTF
    cout << "file path = " << info->path;
#endif

    // post数据包
    QByteArray array = setMd5Json(login->getAccount(), login->getToken(), info->md5, info->fileName);

    // 发送post请求
    QNetworkReply* reply = this->m_manager->post(request, array);
    if (nullptr == reply) {
#if DEBUGPRINTF
        cout << "reply is nullptr";
#endif
        return;
    }

    connect(reply, &QNetworkReply::finished, [=](){
        if (QNetworkReply::NoError != reply->error()) {
#if DEBUGPRINTF
            cout << reply->errorString();
#endif
            return;
        }

        QByteArray data = reply->readAll();
        reply->deleteLater();

        /**
         * 秒传文件：
         * 文件已存在：{"code":"005"}
         * 秒传成功：  {"code":"006"}
         * 秒传失败：  {"code":"007"}
         * token验证失败：{"code":"111"}
         */
#if DEBUGPRINTF
        cout << "return md5 status = " << data;
#endif
        if ("005" == this->m_common.getStatusCode(data)) {
            this->m_common.writeRecord(login->getAccount(), info->fileName, "005");
            uploadList->dealUploadTask();
        } else if ("006" == this->m_common.getStatusCode(data)) {
            // 秒传文件成功
            this->m_common.writeRecord(login->getAccount(), info->fileName, "006");;
            //删除已经完成的上传任务
            uploadList->dealUploadTask();
        } else if ("007" == this->m_common.getStatusCode(data)) {
            // 上传文件
            uploadFile(info);
        } else if ("111" == this->m_common.getStatusCode(data)) {
            QMessageBox::warning(this, "账户异常", "请重新登录!!!");
            emit loginAgainSignal();
            return;
        }
    });
}

void MyFile::uploadFile(UploadFileInfo *info) {
    // 取出上传任务
    QFile* file = info->file;
    QString fileName = info->fileName;
    QString md5 = info->md5;
    qint64 size = info->size;
    DataProgress* dp = info->dp;
    QString boundary = this->m_common.getBoundary();

    LoginInfoInstance* login = LoginInfoInstance::getInstance();
    QByteArray data;

    /**
     * ------WebKitFormBoundary88asdgewtgewx\r\n
     * Content-Disposition: form-data; account="milo"; filename="xxx.jpg"; md5="xxxx"; size=10240\r\n
     * Content-Type: application/octet-stream\r\n
     * \r\n
     * 真正的文件内容\r\n
     * ------WebKitFormBoundary88asdgewtgewx
     */

    // 第一行
    data.append(boundary.toUtf8());
    data.append("\r\n");
#if DEBUGPRINTF
    cout << "boundary" << data;
#endif

    // http头
    data.append(QString("Content-Disposition: form-data; ").toUtf8());
    data.append(QString("account=\"%1\"; ").arg(login->getAccount()).toUtf8());
    data.append(QString("filename=\"%1\"; ").arg(fileName).toUtf8());
    data.append(QString("md5=\"%1\"; ").arg(md5).toUtf8());
    data.append(QString("size=%1;").arg(size).toUtf8());
    data.append("\r\n");

    data.append(QString("Content-Type: qpplication/octet-stream").toUtf8());
    data.append("\r\n\r\n");

    // 文件内容
    data.append(file->readAll());
    data.append("\r\n");

    // 结束分隔线
    data.append(boundary.toUtf8());

    QNetworkRequest request;
    QString url = QString("http://%1:%2/upload").arg(IP).arg(PORT);
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 发送post请求
    QNetworkReply* reply = this->m_manager->post(request, data);

    if (nullptr == reply) {
#if DEBUGPRINTF
        cout << "upload reply is nullptr";
#endif
        return;
    }

    // 有可用数据更新时
    connect(reply, &QNetworkReply::uploadProgress, [=](qint64 bytesRead, qint64 totalBytes){
        if (0 != totalBytes) {
            // 设置进度条
            dp->setProgressValue(bytesRead / 1024, totalBytes / 1024);
        }
    });

    // 获取请求的数据完成时，发送信号SIGNAL
    connect(reply, &QNetworkReply::finished, [=](){
        if (QNetworkReply::NoError != reply->error()) {
#if DEBUGPRINTF
            cout << reply->errorString();
#endif
            reply->deleteLater();
            return;
        }

        QByteArray array = reply->readAll();
        reply->deleteLater();

        /**
         * 上传文件：
         * 成功：{"code":"008"}
         * 失败：{"code":"009"}
         */

#if DEBUGPRINTF
        cout << "uploadFile return status = " << array;
#endif
        if ("008" == this->m_common.getStatusCode(array)) {
            this->m_common.writeRecord(login->getAccount(), fileName, "008");
        } else if ("009" == this->m_common.getStatusCode(array)) {
            this->m_common.writeRecord(login->getAccount(), fileName, "009");
        }

        UploadTask* uploadList = UploadTask::getInstance();
        if (nullptr == uploadList) {
#if DEBUGPRINTF
            cout << "UploadTask::getInstance() is nullptr";
#endif
            return;
        }

        uploadList->dealUploadTask();

    });
}

void MyFile::clearFileList() {
    int n = this->m_fileList.size();
    for (int i = 0; i < n; ++i) {
        FileInfo* tmp = this->m_fileList.takeFirst();
        delete tmp;
        tmp = nullptr;
    }
}

void MyFile::clearItems() {
    auto helper = [](QListWidget* listWidget){
        int n = listWidget->count();
        for (int i = 0; i < n; ++i) {
            QListWidgetItem* item = listWidget->takeItem(0);
            delete item;
            item = nullptr;
        }
    };

    helper(ui->total);
    helper(ui->vedio);
    helper(ui->audio);
    helper(ui->picture);
    helper(ui->document);
    helper(ui->book);
}

void MyFile::refreshFileItems() {
    // 清空所有item项目
    clearItems();

    // 如果文件列表不为空，显示文件列表
#if DEBUGPRINTF
    cout << "文件数量: " << this->m_fileList.size();
#endif
    if (!this->m_fileList.isEmpty()) {
        int n = this->m_fileList.size();
        for (int i = 0; i < n; ++i) {
            FileInfo* tmp = this->m_fileList.at(i);
            QString type = tmp->type;
            QListWidgetItem* item = tmp->item;
            ui->total->addItem(item);

            if ("mp3" == type || "wav" == type) {
                ui->audio->addItem(new QListWidgetItem(*item));
            } else if ("mp4" == type || "avi" == type) {
                ui->vedio->addItem(new QListWidgetItem(*item));
            } else if ("epub" == type || "mobi" == type) {
                ui->book->addItem(new QListWidgetItem(*item));
            } else if ("jpg" == type || "png" == type || "jpeg" == type || "gif" == type || "svg" == type) {
                ui->picture->addItem(new QListWidgetItem(*item));
            } else if("doc" == type || "docx" == type || "xls" == type || "xlsx" == type || "ppt" == type || "pdf" == type || "txt" == type) {
                ui->document->addItem(new QListWidgetItem(*item));
            }
        }
    }
}

QStringList MyFile::getCountStatus(QByteArray json) {
    QJsonParseError error;
    QStringList list;

    QJsonDocument doc = QJsonDocument::fromJson(json, &error);
    if (QJsonParseError::NoError == error.error) {
        if (doc.isNull() || doc.isEmpty()) {
#if DEBUGPRINTF
            cout << "QJsonDocument isNull or isEmpty";
#endif
            return list;
        }

        if (doc.isObject()) {
            // 获得整个json对象
            QJsonObject obj = doc.object();
            // 登录token
            list.append(obj.value("token").toString());
            // 文件个数
            list.append(obj.value("num").toString());
        }
    } else {
#if DEBUGPRINTF
        cout << "error: " << error.errorString();
#endif
    }

    return list;
}

void MyFile::refreshFiles(Display cmd) {
    this->m_accountFileCount = 0;

    // 获取登录信息实例
    LoginInfoInstance* login = LoginInfoInstance::getInstance();

    // 请求对象
    QNetworkRequest request;
    QString url = QString("http://%1:%2/myfiles?cmd=count").arg(IP).arg(PORT);
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QByteArray data = setAccountJson(login->getAccount(), login->getToken());
    QNetworkReply* reply = this->m_manager->post(request, data);
    if (nullptr == reply) {
#if DEBUGPRINTF
        cout << "nullpter == reply";
#endif
        return;
    }

    connect(reply, &QNetworkReply::finished, this, [=](){
        if (QNetworkReply::NoError != reply->error()) {
#if DEBUGPRINTF
            cout << reply->errorString();
#endif
            reply->deleteLater();
            return;
        }

        // 服务器返回数据
        QByteArray array = reply->readAll();
#if DEBUGPRINTF
        cout << "array: " << array;
#endif
        reply->deleteLater();

        // 防止没有文件而闪退
        if (array.isEmpty()) {
            return;
        }

        // 得到服务器json文件
        QStringList list = getCountStatus(array);

        if ("111" == list.at(0)) {
            QMessageBox::warning(this, "用户登录", "账户异常，请重新登录!!!");
#if DEBUGPRINTF
            cout << "账户异常";
#endif
            emit loginAgainSignal();
            // 中断
            return;
        }

        this->m_accountFileCount = list.at(1).toLong();
#if DEBUGPRINTF
        cout << "Account File Count" << this->m_accountFileCount;
#endif

        // 清空文件列表信息
        clearFileList();

        if (this->m_accountFileCount > 0) {
            // 说明用户有文件，获取用户文件列表
            // 从0开始
            this->m_start = 0;
            // 每次请求10个
            this->m_count = 10;
            getAccountFileList(cmd);
        } else {
            refreshFileItems();
        }
    });
}

QByteArray MyFile::setAccountJson(QString account, QString token) {
    QMap<QString, QVariant> mmap;
    mmap.insert("account", account);
    mmap.insert("token", token);

    QJsonDocument doc = QJsonDocument::fromVariant(mmap);
    if (doc.isNull()) {
#if DEBUGPRINTF
        cout << "QJsonDocument is NULL";
#endif
        return {};
    }

    return doc.toJson();
}

QByteArray MyFile::setFilesListJson(QString account, QString token, int start, int count) {
    QMap<QString, QVariant> mmap;
    mmap.insert("account", account);
    mmap.insert("token", token);
    mmap.insert("start", start);
    mmap.insert("count", count);

    QJsonDocument doc = QJsonDocument::fromVariant(mmap);
    if (doc.isNull()) {
#if DEBUGPRINTF
        cout << "QJsonDocument is NULL";
#endif
        return {};
    }

    return doc.toJson();

}

void MyFile::getAccountFileList(Display cmd) {
    // 遍历数目，递归结束条件处理
    if (this->m_accountFileCount <= 0) {
#if DEBUGPRINTF
        cout << "获取用户文件列表条件结束";
#endif
        refreshFileItems();
        return;
    } else if (this->m_count > this->m_accountFileCount) {
        // 如果请求的文件数量大于用户的文件数量
        this->m_count = this->m_accountFileCount;
    }

    LoginInfoInstance* login = LoginInfoInstance::getInstance();
    QNetworkRequest request;

    QString tmp;
    if (cmd == Display::Normal) {
        tmp = "normal";
    } else if (cmd == Display::DownloadsAsc) {
        tmp = "downloadsasc";
    } else if (cmd == Display::DownloadsDesc) {
        tmp = "downloadsdesc";
    }

    QString url = QString("http://%1:%2/myfiles?cmd=%3").arg(IP).arg(PORT).arg(tmp);
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    /**
     * {
     *   "user": "milo"
     *   "token": "xxxx"
     *   "start": 0
     *   "count": 10
     * }
     */

    QByteArray data = setFilesListJson(login->getAccount(), login->getToken(), this->m_start, this->m_count);

    // 改变文件起点位置
    this->m_start += this->m_count;
    this->m_accountFileCount -= this->m_count;

    // 发送post请求
    QNetworkReply* reply = this->m_manager->post(request, data);
    if (nullptr == reply) {
#if DEBUGPRINTF
        cout << "get account file list reply is nullptr";
#endif
        return;
    }

    // 获取请求的数据完成时，发送信号finished()
    connect(reply, &QNetworkReply::finished, this, [=](){
        if (QNetworkReply::NoError != reply->error()) {
#if DEBUGPRINTF
            cout << reply->errorString();
#endif
            reply->deleteLater();
            return;
        }

        // 服务器返回给用户的数据
        QByteArray array = reply->readAll();

#if DEBUGPRINTF
        cout << array.length() << "\t" << array.data();
#endif

        reply->deleteLater();

        // 防止没有文件造成闪退
        if (array.isEmpty()) {
            return;
        }

        // token 验证失败
        if ("111" == this->m_common.getStatusCode(array)) {
            QMessageBox::warning(this, "获取文件列表", "账户异常，请重新登录!!!");
#if DEBUGPRINTF
            cout << "账户异常";
#endif
            emit loginAgainSignal();
            return;
        }

        // 不是错误码就处理文件列表JSON信息
        if ("015" != this->m_common.getStatusCode(array)) {
            // 处理文件列表JSON信息, 存放到文件列表中
            getFileJsonInfo(array);
            // 继续获取用户文件
            getAccountFileList(cmd);
        }
    });
}

void MyFile::getFileJsonInfo(QByteArray data) {
    /**
     * "account": "mio",
     * "md5": "xxx",
     * "crete_time": "xxxx-xx-xx xx:xx:xx",
     * "file_name": "test.png",
     * "share_status": 0,
     * "download": 1,
     * "url": "http://IP:PORT/group1/M00/00/00/xxx.png"
     * "size": xxx,
     * "type": "png"
     */

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (QJsonParseError::NoError == error.error) {
        if (doc.isNull() || doc.isEmpty()) {
#if DEBUGPRINTF
            cout << "QJsonDocument is null or is empty";
#endif
            return;
        }

        if (doc.isObject()) {
            // 获取整个JSON对象
            QJsonObject obj = doc.object();

            QJsonArray array = obj.value("files").toArray();

            int size = array.size();
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

                // 添加节点
                this->m_fileList.append(info);
            }
        }
    } else {
#if DEBUGPRINTF
        cout << "QJsonParserError = " << error.errorString();
#endif
    }
}

void MyFile::dealSelectFile(QString cmd) {
    QString currTable = ui->tabWidget->tabText(ui->tabWidget->currentIndex());
    QListWidgetItem* item = nullptr;

    // 获取当前选中的Item
    if ("全部" == currTable) {
        item = ui->total->currentItem();
    } else if ("视频" == currTable) {
        item = ui->vedio->currentItem();
    } else if ("音频" == currTable) {
        item = ui->audio->currentItem();
    } else if ("图片" == currTable) {
        item = ui->picture->currentItem();
    } else if ("图书" == currTable) {
        item = ui->book->currentItem();
    } else if ("文档" == currTable) {
        item = ui->document->currentItem();
    }

    if (nullptr == item) {
        return;
    }

    // 查找文件列表匹配的元素
    int n = this->m_fileList.size();
    for (int i = 0; i < n; ++i) {
        if (item->text() == this->m_fileList.at(i)->item->text()) {
            if ("share" == cmd) {
                    shareFile(this->m_fileList.at(i));
            } else if ("delete" == cmd) {
                    deleteFile(this->m_fileList.at(i));
            } else if ("property" == cmd) {
                    getFileProperty(this->m_fileList.at(i));
            }
            break;
        }
    }
}

QByteArray MyFile::setDealFileJson(QString account, QString token, QString md5, QString fileName) {
    QMap<QString, QVariant> mmap;
    mmap.insert("account", account);
    mmap.insert("token", token);
    mmap.insert("md5", md5);
    mmap.insert("fileName", fileName);

    QJsonDocument doc = QJsonDocument::fromVariant(mmap);
    if (doc.isNull()) {
#if DEBUGPRINTF
        cout << "QJsonDocument is NULL";
#endif
        return {};
    }

    return doc.toJson();
}

void MyFile::shareFile(FileInfo *info) {
    if (1 == info->shareStatus) {
        QMessageBox::warning(this, "分享文件", "此文件已有分享记录!!!");
        return;
    }

    LoginInfoInstance* login = LoginInfoInstance::getInstance();
    QNetworkRequest request;
    QString url = QString("http://%1:%2/dealfile?cmd=share").arg(IP).arg(PORT);
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QByteArray data = setDealFileJson(login->getAccount(), login->getToken(), info->md5, info->fileName);

    // 发送post请求
    QNetworkReply* reply = this->m_manager->post(request, data);
    if (nullptr == reply) {
#if DEBUGPRINTF
        cout << "share file reply is nullptr";
#endif
        return;
    }

    // 获取请求的数据完成时，会发送信号finished()
    connect(reply, &QNetworkReply::finished, this, [=](){
        if (QNetworkReply::NoError != reply->error()) {
#if DEBUGPRINTF
            cout << reply->errorString();
#endif
            reply->deleteLater();
            return;
        }

        // 服务器返回给用户的数据
        QByteArray array = reply->readAll();
        reply->deleteLater();

        /**
         * 分享文件：
         * 成功：{"code":"010"}
         * 失败：{"code":"011"}
         * 别人已经分享此文件：{"code", "012"}
         * token验证失败：{"code":"111"}
         */

        if ("010" == this->m_common.getStatusCode(array)) {
            // 设置此文件已经被分享
            info->shareStatus = 1;
            QMessageBox::information(this, "分享文件", QString("[%1] 分享成功!!!").arg(info->fileName));
        } else if ("011" == this->m_common.getStatusCode(array)) {
            QMessageBox::warning(this, "分享文件", QString("[%1] 分享失败!!!").arg(info->fileName));
        } else if ("012" == this->m_common.getStatusCode(array)) {
            QMessageBox::warning(this, "分享文件", QString("[%1] 别人已分享此文件!!!").arg(info->fileName));
        } else if ("111" == this->m_common.getStatusCode(array)) {
            QMessageBox::warning(this, "分享文件", QString("账户异常，请重新登录!!!").arg(info->fileName));
#if DEBUGPRINTF
            cout << "账户异常";
#endif
            // 发送重新登录信号
            emit loginAgainSignal();
            return;
        }
    });
}

void MyFile::deleteFile(FileInfo *info) {
    /**
     * {
     *   "user": "milo",
     *   "token": "xxxx",
     *   "md5": "xxx",
     *   "filename": "xxx"
     * }
     */

    LoginInfoInstance* login = LoginInfoInstance::getInstance();
    QNetworkRequest request;
    QString url = QString("http://%1:%2/dealfile?cmd=delete").arg(IP).arg(PORT);
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QByteArray data = setDealFileJson(login->getAccount(), login->getToken(), info->md5, info->fileName);

    // 发送post请求
    QNetworkReply* reply = this->m_manager->post(request, data);
    if (nullptr == reply) {
#if DEBUGPRINTF
        cout << "delete file reply is nullptr";
#endif
        return;
    }

    // 获取请求的数据完成时，发送信号finished()
    connect(reply, &QNetworkReply::finished, this, [=](){
        if (QNetworkReply::NoError != reply->error()) {
#if DEBUGPRINTF
            cout << reply->errorString();
#endif
            reply->deleteLater();
            return;
        }

        // 服务器返回给用户的数据
        QByteArray array = reply->readAll();
        reply->deleteLater();

        /**
         * 删除文件：
         * 成功：{"code":"013"}
         * 失败：{"code":"014"}
         */

        if ("013" == this->m_common.getStatusCode(array)) {
            QMessageBox::information(this, "删除文件", QString("[%1] 删除成功!!!").arg(info->fileName));
            int n = this->m_fileList.size();
            // 从文件列表中移除该文件，移除列表视图中的Item
            for (int i = 0; i < n; ++i) {
                if (info == this->m_fileList.at(i)) {
                    QListWidgetItem* item = info->item;
                    QString currTable = ui->tabWidget->tabText(ui->tabWidget->currentIndex());

                    // 获取当前选中的Item
                    if ("视频" == currTable) {
                        ui->vedio->removeItemWidget(item);
                    } else if ("音频" == currTable) {
                        ui->audio->removeItemWidget(item);
                    } else if ("图片" == currTable) {
                        ui->picture->removeItemWidget(item);
                    } else if ("图书" == currTable) {
                        ui->book->removeItemWidget(item);
                    } else if ("文档" == currTable) {
                        ui->document->removeItemWidget(item);
                    }

                    ui->total->removeItemWidget(item);
                    delete item;

                    this->m_fileList.removeAt(i);
                    delete info;
                    break;
                }
            }
        } else if ("014" == this->m_common.getStatusCode(array)) {
            QMessageBox::warning(this, "删除文件", QString("[%1] 文件删除失败!!!").arg(info->fileName));
        } else if ("111" == this->m_common.getStatusCode(array)) {
            QMessageBox::warning(this, "删除文件", "账户异常，请重新登录!!!");
#if DEBUGPRINTF
            cout << "账户异常";
#endif
            emit loginAgainSignal();
            return;
        }
    });
}

void MyFile::getFileProperty(FileInfo *info) {
    // 创建对话框
    FilePropertyInfo dlg;
    dlg.setInfo(info);
    // 模拟方式运行
    dlg.exec();
}

void MyFile::addDownloadFiles() {
    emit gotoTransfer(TransferStatus::Download);

    QString currTable = ui->tabWidget->tabText(ui->tabWidget->currentIndex());
    QListWidgetItem* item = nullptr;

    // 获取当前选中的Item
    if ("全部" == currTable) {
        item = ui->total->currentItem();
    } else if ("视频" == currTable) {
        item = ui->vedio->currentItem();
    } else if ("音频" == currTable) {
        item = ui->audio->currentItem();
    } else if ("图片" == currTable) {
        item = ui->picture->currentItem();
    } else if ("图书" == currTable) {
        item = ui->book->currentItem();
    } else if ("文档" == currTable) {
        item = ui->document->currentItem();
    }

    if (nullptr == item) {
#if DEBUGPRINTF
        cout << "item is nullptr";
#endif
        return;
    }

    DownloadTask* downloadList = DownloadTask::getInstance();
    if (nullptr == downloadList) {
#if DEBUGPRINTF
        cout << "DownloadTask::getInstance() is nullptr";
#endif
        return;
    }

    int n = this->m_fileList.size();
    for (int i = 0; i < n; ++i) {
        if (item == this->m_fileList.at(i)->item) {
            QString fileSavePath = QFileDialog::getSaveFileName(this, "选择保存文件路径", this->m_fileList.at(i)->fileName);
            if (fileSavePath.isEmpty()) {
#if DEBUGPRINTF
                cout << "file save path is empty";
#endif
                return;
            }

            // 0 success; -1 文件已在下载队列中; -2 打开文件失败
            int res = downloadList->appendDownloadList(this->m_fileList.at(i), fileSavePath);
            if (-1 == res) {
                QMessageBox::warning(this, "下载文件", "该文件已在下载队列中!!!");
            } else if (-2 == res) {
                this->m_common.writeRecord(this->m_fileList.at(i)->account, this->m_fileList.at(i)->fileName, "010");
            }
            break;
        }
    }
}

void MyFile::downloadFilesAction() {
    DownloadTask* downloadList = DownloadTask::getInstance();
    if (nullptr == downloadList) {
#if DEBUGPRINTF
        cout << "DownloadTask::getInstance() is nullptr";
#endif
        return;
    }

    if (downloadList->isEmpty()) {
        return;
    }

    if (downloadList->isDownload()) {
        return;
    }

    if (downloadList->isShareTask()) {
        return;
    }

    // 取下载任务
    DownloadInfo* info = downloadList->takeTask();

    QString account = info->account;
    QString md5 = info->md5;
    QString fileName = info->fileName;
    QUrl url = info->url;
    QFile* file = info->file;
    DataProgress* dp = info->dp;

    // 发送get请求
    QNetworkReply* reply = this->m_manager->get(QNetworkRequest(url));
    if (nullptr == reply) {
        // 删除下载任务
        // downloadList->dealDownloadTask();
#if DEBUGPRINTF
        cout << "request get error";
#endif
        return;
    }

    // 获取请求的数据完成时，发送信号finished()
    connect(reply, &QNetworkReply::finished, this, [=](){
#if DEBUGPRINTF
        cout << "下载完成";
#endif
        reply->deleteLater();

        // 删除下载任务
        downloadList->dealDownloadTask();

        // 下载文件成功，记录
        this->m_common.writeRecord(account, fileName, "010");

        // 下在文件成功， downloads字段处理
        dealFileDownloads(md5, fileName);
    });

    // 服务器发送数据， reply将发出readRead()信号
    connect(reply, &QNetworkReply::readyRead, this, [=](){
        // 如果文件存在，则写入文件
        if (nullptr != file) {
            file->write(reply->readAll());
        }
    });

    // 有数据更新时
    connect(reply, &QNetworkReply::downloadProgress, this, [=](qint64 bytesRead, qint64 totalBytes){
        // 设置进度
        dp->setProgressValue(bytesRead, totalBytes);
    });
}

void MyFile::dealFileDownloads(QString md5, QString fileName) {
    /**
     * {
     * 	"account": "mio",
     * 	"token": "xxx",
     * 	"md5": "xxx",
     * 	"filename": "xxx",
     * }
     */

    LoginInfoInstance* login = LoginInfoInstance::getInstance();

    QNetworkRequest request;
    QString url = QString("http://%1:%2/dealfile?cmd=downloads").arg(IP).arg(PORT);
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QByteArray data = setDealFileJson(login->getAccount(), login->getToken(), md5, fileName);

    // 发送post请求
    QNetworkReply* reply = this->m_manager->post(request, data);
    if (nullptr == reply) {
#if DEBUGPRINTF
        cout << "deal file downloads reply == nullptr";
#endif
        return;
    }

    // 获取请求的数据完成时，会发出信号finished()
    connect(reply, &QNetworkReply::finished, this, [=](){
        if (QNetworkReply::NoError != reply->error()) {
#if DEBUGPRINTF
            cout << reply->errorString();
#endif
            reply->deleteLater();
            return;
        }

        // 服务器返回给用户的数据
        QByteArray array = reply->readAll();
        reply->deleteLater();

        /**
         * 下载文件 downloads 字段处理
         * 成功：{"code":"016"}
         * 失败：{"code":"017"}
         */

        if ("016" == this->m_common.getStatusCode(array)) {
            // 该文件downloads字段 +1
            int n = this->m_fileList.size();
            for (int i = 0; i < n; ++i) {
                FileInfo* info = this->m_fileList.at(i);
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

void MyFile::clearAllTask() {
    // 获取上传队列实例
    UploadTask* uploadList = UploadTask::getInstance();
    if (nullptr == uploadList) {
#if DEBUGPRINTF
        cout << "UploadTask::getInstance() is nullptr";
#endif
        return;
    }
    uploadList->clearList();

    // 获取下载队列实例
    DownloadTask* downloadList = DownloadTask::getInstance();
    if (nullptr == downloadList) {
#if DEBUGPRINTF
        cout << "DownloadTask::getInstance() is nullptr";
#endif
        return;
    }
    downloadList->clearList();
}

void MyFile::checkTaskList() {
    // 定时检查上传队列是否有上传任务
    connect(&this->m_uploadFileTimer, &QTimer::timeout, this, [=](){
        uploadFileAction();
    });
    // 启动定时器，500毫秒间隔，每次只能上传一个文件
    this->m_uploadFileTimer.start(500);

    // 定时检查下载队列是否有下载任务
    connect(&this->m_downloadFileTimer, &QTimer::timeout, this, [=](){
        downloadFilesAction();
    });
    // 启动定时器，500毫秒间隔，每次只能下载一个文件
    this->m_downloadFileTimer.start(500);
}

void MyFile::on_uploadFile_clicked() {
    // 上传文件
    this->addUploadFiles();
}

void MyFile::rightMenu(const QPoint &pos) {
    QString currTable = ui->tabWidget->tabText(ui->tabWidget->currentIndex());
    QListWidgetItem* item = nullptr;

    // 获取当前选中的Item
    if ("全部" == currTable) {
        item = ui->total->itemAt(pos);
    } else if ("视频" == currTable) {
        item = ui->vedio->itemAt(pos);
    } else if ("音频" == currTable) {
        item = ui->audio->itemAt(pos);
    } else if ("图片" == currTable) {
        item = ui->picture->itemAt(pos);
    } else if ("图书" == currTable) {
        item = ui->book->itemAt(pos);
    } else if ("文档" == currTable) {
        item = ui->document->itemAt(pos);
    }

    // 没有点击item
    if (nullptr == item) {
        this->m_menuEmpty->exec(QCursor::pos());
    } else {
        if ("全部" == currTable) {
            ui->total->setCurrentItem(item);
        } else if ("视频" == currTable) {
            ui->vedio->setCurrentItem(item);
        } else if ("音频" == currTable) {
            ui->audio->setCurrentItem(item);
        } else if ("图片" == currTable) {
            ui->picture->setCurrentItem(item);
        } else if ("图书" == currTable) {
            ui->book->setCurrentItem(item);
        } else if ("文档" == currTable) {
            ui->document->setCurrentItem(item);
        }
        this->m_menuItem->exec(QCursor::pos());
    }
}

void MyFile::on_comboBox_activated(int index) {
    if (0 == index) {
        setItemViewMode(QListView::ListMode);
    } else if (1 == index) {
        setItemViewMode(QListView::IconMode);
    }
}

