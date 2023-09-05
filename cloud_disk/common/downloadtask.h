#ifndef DOWNLOADTASK_H
#define DOWNLOADTASK_H

#include "common.h"
#include "../selfwidget/dataprogress.h"

#include <QVBoxLayout>
#include <QUrl>
#include <QFile>
#include <QList>

// 下载文件信息
struct DownloadInfo {
    QFile* file;        // 文件指针
    QString account;    // 下载用户
    QString fileName;   // 文件名
    QString md5;        // 文件md5值
    QUrl url;           // 下载网址
    DataProgress* dp;   // 下载进度控件
    bool isDownload;    // 是否已经在下载
    bool isShare;       // 是否为共享文件下载
};

/**
 * @brief 下载任务队列类，单例模式
 */
class DownloadTask {
private:
    DownloadTask() = default;
    ~DownloadTask() = default;
    DownloadTask(const DownloadTask&) = delete;
    DownloadTask* operator=(const DownloadTask&) = delete;

public:
    static DownloadTask* getInstance();

    // 清空下载队列
    void clearList();
    // 删除下载完成的任务
    void dealDownloadTask();
    // 判断下载队列是否为空
    bool isEmpty();
    // 判断是否有文件正在下载
    bool isDownload();
    // 第一个任务是否为共享文件的任务
    bool isShareTask();
    // 取出第0个下载任务，如果任务队列没有任务正在下载，设置第0个任务
    DownloadInfo* takeTask();

    /**
     * @brief 添加任务到下载队列中
     *
     * @param info          (in)    下载文件信息
     * @param fileSavePath  (in)    文件保存路径
     * @param isShare       (in)    是否为共享文件下载(default false)
     *
     * @return 函数执行情况
     *   @retval 0  success
     *   @retval -1 下载的文件已经在下载队列中
     *   @retval -2 打开文件失败
     */
    int appendDownloadList(FileInfo* info, QString fileSavePath, bool isShare = false);

private:
    QList<DownloadInfo*> taskList;
};

#endif // DOWNLOADTASK_H
