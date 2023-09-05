#ifndef SHARELIST_H
#define SHARELIST_H

#include <QWidget>
#include <QTimer>
#include "common/common.h"
#include "selfwidget/mymenu.h"

namespace Ui {
class ShareList;
}

class ShareList : public QWidget {
    Q_OBJECT

public:
    explicit ShareList(QWidget *parent = nullptr);
    ~ShareList();
    enum class CMD:char {Property, Cancel, Save};

    // 初始化listWidget属性
    void initListWidget();
    // 添加菜单项
    void addActionMenu();

    // 清空分享文件列表
    void clearShareFileList();
    // 清空所有item项目
    void clearItems();
    // item文件展示
    void refreshFileItems();
    // 显示共享的文件列表
    void refreshFiles();
    // 设置JSON包
    QByteArray setFilesListJson(int start, int count);
    // 获取共享文件列表
    void getAccountFilesList();
    // 解析文件列表JSON信息，存放在文件列表中
    void getFileJsonInfo(QByteArray data);
    // 添加需要下载的文件到下载任务队列中
    void addDownloadFiles();
    // 下载任务处理，取出下载队列的队首任务，下载完后，再取下一个任务
    void downloadFilesAction();
    // 设置JSON包
    QByteArray setShareFileJson(QString account, QString md5, QString fileName);
    // 处理下载downloads字段
    void dealFileDownloads(QString md5, QString fileName);
    // 处理选中的文件
    void dealSelectedFile(CMD cmd = CMD::Property);
    // 获取文件属性
    void getFileProperty(FileInfo* info);
    // 取消已经分享的文件
    void CancelShareFile(FileInfo* info);
    // 转存文件
    void SaveFileToMyList(FileInfo* info);

signals:
    void gotoTransfer(TransferStatus status);

private:
    Ui::ShareList *ui;

    Common m_common;
    MyMenu* m_menuItem;     // 点击Item时弹出的菜单
    MyMenu* m_menuEmpty;    // 点击Empty时弹出的菜单

    QNetworkAccessManager* m_manager;

    int m_start;    // 文件位置起点
    int m_count;    // 每次请求文件的个数
    long m_accountFilesCount;           // 用户文件数目
    QList<FileInfo*> m_shareFileList;   // 分享文件列表
    QTimer m_downloadTimer; // 定时检查下载队列中是否有任务需要下载
};

#endif // SHARELIST_H
