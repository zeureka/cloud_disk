#ifndef MYFILE_H
#define MYFILE_H

#include <QWidget>
#include <QTimer>
#include "selfwidget/mymenu.h"
#include "common/uploadtask.h"

namespace Ui {
class MyFile;
}

class MyFile : public QWidget
{
    Q_OBJECT

public:
    explicit MyFile(QWidget *parent = nullptr);
    ~MyFile();

    // 初始化listWidget文件列表
    void initListWidget();
    // 添加右键菜单
    void addActionMenu();
    // 设置Item显示模式ListMode or IconMode
    void setItemViewMode(QListView::ViewMode mode = QListView::ListMode);

    // =======> 上传文件处理 <======
    // 添加文件到上传列表中
    void addUploadFiles();
    // 设置md5值的JSON包
    QByteArray setMd5Json(QString account, QString token, QString md5, QString fileName);
    // 上传文件处理，取出上传任务队列的队首任务，上传完成后，再取出下一个任务
    void uploadFileAction();
    // 在不能秒传的前提下，上传文件内容
    void uploadFile(UploadFileInfo* info);

    // =======> 文件Item展示 <======
    // 清空文件列表
    void clearFileList();
    // 清空所有Item项目
    void clearItems();
    // 文件Item展示
    void refreshFileItems();

    // =======> 显示用户的文件列表 <========
    // desc 是 descend 降序意思
    // asc 是 ascend 升序意思
    // Normal：普通用户列表，DownloadsAsc：按下载量升序， DownloadsDesc：按下载量降序
    enum class Display : char {Normal, DownloadsAsc, DownloadsDesc};
    // current tabWidget
    enum class CurrTable : char {Total, Vedio, Audio, Picture, Document, Book};

    // 得到服务器的JSON文件
    QStringList getCountStatus(QByteArray json);
    // 显示用户的文件列表
    void refreshFiles(Display cmd = Display::Normal);
    // 设置JSON包
    QByteArray setAccountJson(QString account, QString token);
    // 设置JSON包
    QByteArray setFilesListJson(QString account, QString token, int start, int count);
    // 获取用户文件列表
    void getAccountFileList(Display cmd = Display::Normal);
    // 解析文件列表JSON包，存放在文件列表中
    void getFileJsonInfo(QByteArray data);

    // ========> 分享、删除、处理文件 <=========
    // 处理选中的文件
    void dealSelectFile(QString cmd = "share");
    QByteArray setDealFileJson(QString account, QString token, QString md5, QString fileName);
    // 分享文件
    void shareFile(FileInfo* info);
    // 删除文件
    void deleteFile(FileInfo* info);
    // 获取文件属性
    void getFileProperty(FileInfo* info);
    // 添加文件到下载任务列表
    void addDownloadFiles();
    // 下载文件处理，取出下载任务队列的队首任务，下载完成后，再取下一个任务
    void downloadFilesAction();
    // 下载文件downloads字段处理
    void dealFileDownloads(QString md5, QString fileName);

    // 清除上传，下载任务
    void clearAllTask();
    // 定时检查任务队列中的任务
    void checkTaskList();

signals:
    void loginAgainSignal();
    void gotoTransfer(TransferStatus status);

private slots:
    void on_uploadFile_clicked();

    // 右键菜单信号的槽函数
    void rightMenu(const QPoint& pos);

    void on_comboBox_activated(int index);

private:
    Ui::MyFile *ui;

    Common m_common;
    QNetworkAccessManager* m_manager;

    MyMenu* m_menuItem;					// 右键点击文件弹出菜单
    MyMenu* m_menuEmpty;			// 有点点击空白处弹出菜单

    long m_accountFileCount;		// 用户文件数量
    int m_start;					// 文件位置起点
    int m_count;					// 每次请求文件个数

    QTimer m_uploadFileTimer;		// 定时检查上传队列是否有任务
    QTimer m_downloadFileTimer;		// 定时检查下载队列是否有任务

    QList<FileInfo*> m_fileList;	// 文件列表
};

#endif // MYFILE_H
