#ifndef UPLOADTASK_H
#define UPLOADTASK_H

#include "common.h"
#include <QVBoxLayout>
#include <QFile>
#include "../selfwidget/dataprogress.h"

// 上传文件信息
struct UploadFileInfo {
    QString md5;        // 文件md5值
    QFile* file;        // 文件指针
    QString fileName;   // 文件名字
    qint64 size;        // 文件大小
    QString path;       // 文件路径
    bool isUpload;      // 是否已经在上传
    DataProgress* dp;   // 上传进度控件
};

// 上传任务列表类，单例模式，一个程序只能有一个上传任务列表
class UploadTask {
private:
    UploadTask() = default;
    ~UploadTask() = default;
    UploadTask(const UploadTask&) = delete;
    UploadTask& operator=(const UploadTask&) = delete;

public:
    static UploadTask* getInstance();

    /**
     * @brief 添加上传文件到上传列表中
     *
     * @param path  (in)    上传文件路径
     *
     * @return 函数执行情况
     *   @retval 0  成功
     *   @retval -1 文件大于100M
     *   @retval -2 上传的文件已经在上传队列中
     *   @retval -3 打开文件失败
     *   @retval -4 获取布局失败
     */
    int appendUploadList(QString path);

    // 判断上传队列是否为空
    bool isEmpty();

    // 是否有文件正在上传
    bool isUpload();

    // 取出第0ge上传任务，如果任务队列没有任务在上传，设置第0个任务上传
    UploadFileInfo* takeTask();

    // 删除上传完成的任务
    void dealUploadTask();

    // 清空上传列表
    void clearList();

private:
    QList<UploadFileInfo*> taskList;
};

#endif // UPLOADTASK_H
