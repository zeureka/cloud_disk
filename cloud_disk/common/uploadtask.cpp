#include "uploadtask.h"
#include "../selfwidget/dataprogress.h"
#include "common.h"
#include "uploadlayout.h"
#include <QFileInfo>

UploadTask* UploadTask::getInstance() {
    static UploadTask instance;
    return &instance;
}

int UploadTask::appendUploadList(QString path) {
    qint64 size = QFileInfo(path).size();
    if (size > 100 * 1024 * 1024) {
#if DEBUGPRINTF
        cout << "File is to big! (> 100M)";
#endif
        return -1;
    }

    for (const auto& info : this->taskList) {
        if (path == info->path) {
#if DEBUGPRINTF
            cout << QFileInfo(path).fileName() << "已经在上传队列中";
#endif
            return -2;
        }
    }

    QFile* file = new QFile(path);
    if (!file->open(QIODevice::ReadOnly)) {
#if DEBUGPRINTF
        cout << "File open failed";
#endif
        delete file;
        file = nullptr;
        return -3;
    }

    // 获取文件属性
    QFileInfo fileInfo(path);

    // 动态创建节点
    Common common;
    UploadFileInfo* uploadInfo = new UploadFileInfo;

    // 初始化信息, 赋值
    uploadInfo->md5 = common.getFileMd5(path);
    uploadInfo->file = file;
    uploadInfo->fileName = fileInfo.fileName();
    uploadInfo->size = size;
    uploadInfo->path = path;
    uploadInfo->isUpload = false;

    // 设置进度条
    DataProgress* dataP = new DataProgress;
    dataP->setFileName(uploadInfo->fileName);
    uploadInfo->dp = dataP;

    // 获取布局信息
    UploadLayout* pUpload = UploadLayout::getInstance();
    if (nullptr == pUpload) {
#if DEBUGPRINTF
        cout << "UploadTask::getInstance() == nullptr";
#endif
        return -4;
    }

    QVBoxLayout* layout = (QVBoxLayout*)pUpload->getUploadLayout();
    // 添加到布局，最后一个是弹簧，插入到弹簧上面，所以这里要-1
    layout->insertWidget(layout->count() - 1, dataP);

#if DEBUGPRINTF
    cout << uploadInfo->fileName.toUtf8().data() << "已经加入上传列表";
#endif

    // 添加节点
    this->taskList.append(uploadInfo);

    return 0;

}

// 判断上传队列是否为空
bool UploadTask::isEmpty() {
    return taskList.isEmpty();
}

// 是否有文件正在上传
bool UploadTask::isUpload() {
    for (const auto& info : this->taskList) {
        if (true == info->isUpload) {
            return true;
        }
    }

    return false;
}

// 取出第0ge上传任务，如果任务队列没有任务在上传，设置第0个任务上传
UploadFileInfo* UploadTask::takeTask() {
    UploadFileInfo* info = this->taskList.at(0);
    this->taskList.at(0)->isUpload = true;

    return info;
}

// 删除上传完成的任务
void UploadTask::dealUploadTask() {
    int n = this->taskList.size();
    for (int i = 0; i < n; ++i) {
        if (true == this->taskList.at(i)->isUpload) {
            // 说明有下载任务
            // 移除此文件，因为已经上传完成了
            UploadFileInfo* info = this->taskList.takeAt(i);

            // 获取布局
            UploadLayout* pUpload = UploadLayout::getInstance();
            QLayout* layout = pUpload->getUploadLayout();

            // 从布局中移除控件
            layout->removeWidget(info->dp);

            // 关闭打开的文件指针
            QFile* file = info->file;
            file->close();
            delete file;

            // 释放资源
            delete info->dp;
            delete info;
        }
    }
}

// 清空上传列表
void UploadTask::clearList() {
    int n = this->taskList.size();
    for (int i = 0; i < n; ++i) {
        UploadFileInfo* info = this->taskList.takeFirst();
        delete info;
    }
}
