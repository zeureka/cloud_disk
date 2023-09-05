#include "transferlist.h"
#include "ui_transferlist.h"
#include "common/logininfoinstance.h"
#include "common/uploadlayout.h"
#include "common/downloadlayout.h"
#include <QFile>

TransferList::TransferList(QWidget *parent) : QWidget(parent), ui(new Ui::TransferList) {
    ui->setupUi(this);

    // 设置上传布局
    UploadLayout* uploadLayout = UploadLayout::getInstance();
    uploadLayout->setUploadLayout(ui->upload_scroll);

    // 设置下载布局
    DownloadLayout* downloadLayout = DownloadLayout::getInstance();
    downloadLayout->setDownloadLayout(ui->download_scroll);

    // 设置default页面
    ui->tabWidget->setCurrentIndex(0);

    // 切换tab页
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, [=](int index){
        if (0 == index) {
            emit currentTabSignal("upload");
        } else if (1 == index) {
            emit currentTabSignal("download");
        } else if (2 == index) {
            emit currentTabSignal("record");
            displayDataRecord();
        }
    });
}

TransferList::~TransferList() {
    delete ui;
}

void TransferList::displayDataRecord(QString path) {
    LoginInfoInstance* login = LoginInfoInstance::getInstance();

    QString fileName = path + login->getAccount();
    QFile file(fileName);

    if (false == file.open(QIODevice::ReadOnly)) {
#if DEBUGPRINTF
        cout << "file.open(QIODevice::ReadOnly) failed";
#endif
        return;
    }

    QByteArray array = file.readAll();
    ui->record_msg->setText(array);
    file.close();
}

void TransferList::showUpload() {
    ui->tabWidget->setCurrentWidget(ui->upload);
}

void TransferList::showDownload() {
    ui->tabWidget->setCurrentWidget(ui->download);
}

void TransferList::on_clear_btn_clicked() {
    LoginInfoInstance* login = LoginInfoInstance::getInstance();
    QString fileName = Common::m_recordPath + login->getAccount();

    // 如果文件存在，则删除文件
    if (QFile::exists(fileName)) {
        QFile::remove(fileName);
        ui->record_msg->clear();
    }
}

