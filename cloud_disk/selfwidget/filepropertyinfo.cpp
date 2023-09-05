#include "filepropertyinfo.h"
#include "ui_filepropertyinfo.h"

FilePropertyInfo::FilePropertyInfo(QWidget *parent) : QDialog(parent), ui(new Ui::FilePropertyInfo) {
    ui->setupUi(this);

    // 设置label内容作为超链接内容
    ui->download_url->setOpenExternalLinks(true);
}

FilePropertyInfo::~FilePropertyInfo() {
    delete ui;
}

void FilePropertyInfo::setInfo(FileInfo* info) {
    ui->file_name->setText(info->fileName);
    ui->account->setText(info->account);
    ui->create_time->setText(info->createTime);

    int size = info->size;
    if (size >= 1024 && size < 1024 * 1024) {
        ui->file_size->setText(QString("%1 KB").arg(size/1024.0));
    } else {
        ui->file_size->setText(QString("%1 MB").arg(size/(1024.0 * 1024.0)));
    }

    ui->downloads->setText(QString("下载次数: %1").arg(info->downloads));

    if (1 == info->shareStatus) {
        ui->share_status->setText("已经分享");
    } else {
        ui->share_status->setText("没有分享");
    }

    QString urlStr = QString("<a href=\"%1\">%2</a>").arg(info->url).arg(info->url);
    ui->download_url->setText(urlStr);
}
