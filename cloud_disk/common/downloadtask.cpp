#include "downloadtask.h"
#include "common.h"
#include "downloadlayout.h"
#include <cstdio>

DownloadTask* DownloadTask::getInstance() {
    static DownloadTask instance;
    return &instance;
}

// 清空下载队列
void DownloadTask::clearList() {
    int n = this->taskList.size();
    for (int i = 0; i < n; ++i) {
        DownloadInfo* tmp = this->taskList.takeFirst();
        delete tmp;
    }
}

// 删除下载完成的任务
void DownloadTask::dealDownloadTask() {
    int n = this->taskList.size();
    for (int i = 0; i < n; ++i) {
        if (true == this->taskList.at(i)->isDownload) {
            // 说明有下载任务，且下载任务已经完成了
            DownloadInfo* tmp = this->taskList.takeAt(i);

            // 获取布局
            DownloadLayout* downloadLayout = DownloadLayout::getInstance();
            QLayout* layout = downloadLayout->getDownloadLayout();

            // 从布局中移除控件
            layout->removeWidget(tmp->dp);

            // 关闭文件
            QFile* file = tmp->file;
            file->close();

            // 释放资源
            delete file;
            delete tmp;

            return;
        }
    }
}

// 判断下载队列是否为空
bool DownloadTask::isEmpty() {
    return this->taskList.isEmpty();
}

// 判断是否有文件正在下载
bool DownloadTask::isDownload() {
    int n = this->taskList.size();
    for (int i = 0; i < n; ++i) {
        if (true == this->taskList.at(i)->isDownload) {
            // 说明有下载任务，不能添加新任务
            return true;
        }
    }

    return false;
}

// 第一个任务是否为共享文件的任务
bool DownloadTask::isShareTask() {
    return this->taskList.at(0)->isShare;
}

// 取出第0个下载任务，如果任务队列没有任务正在下载，设置第0个任务
DownloadInfo* DownloadTask::takeTask() {
    if ( isEmpty() ) {
        return nullptr;
    }

    this->taskList.at(0)->isDownload = true;
    return this->taskList.at(0);
}

// 添加任务到下载队列中
int DownloadTask::appendDownloadList(FileInfo* info, QString fileSavePath, bool isShare) {
    int n = this->taskList.size();

    // 查看下载的文件是否在队列
    for (int i = 0; i < n; ++i) {
        if (this->taskList.at(i)->account == info->account && this->taskList.at(i)->fileName == info->fileName) {
#if DEBUGPRINTF
            cout << info->fileName << " 已经在下载队列中";
#endif
            return -1;
        }
    }

    QFile* file = new QFile(fileSavePath);
    if (!file->open(QIODevice::WriteOnly)) {
        // 打开文件失败
#if DEBUGPRINTF
        cout << "file open failed";
#endif
        delete file;
        file = nullptr;
        return -2;
    }

    // 动态创建节点
    DownloadInfo* downloadInfo = new DownloadInfo;
    downloadInfo->account = info->account;
    downloadInfo->file = file;
    downloadInfo->fileName = info->fileName;
    downloadInfo->md5 = info->md5;
    downloadInfo->url = info->url;
    downloadInfo->isDownload = false;
    downloadInfo->isShare = isShare;

    DataProgress* dataP = new DataProgress;
    dataP->setFileName(downloadInfo->fileName);

    // 获取布局
    DownloadLayout* downloadLayout = DownloadLayout::getInstance();
    QVBoxLayout* layout = (QVBoxLayout*)downloadLayout->getDownloadLayout();

    downloadInfo->dp = dataP;

    // 添加到布局中，最后一个是弹簧，插入到弹簧上面
    layout->insertWidget(layout->count() - 1, dataP);

    // 插入节点
    this->taskList.append(downloadInfo);

#if DEBUGPRINTF
    cout << info->url << "已经添加到下载列表";
#endif

    return 0;
}
