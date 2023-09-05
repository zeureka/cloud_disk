#ifndef COMMON_H
#define COMMON_H

#include <QObject>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QDebug>
#include <QListWidgetItem>

#define cout qDebug() << "[" << __FILE__ << ":" << __LINE__ << "]: "

#define ACCOUNT_REG "^[a-zA-Z\\d@_#]\{3,16\}$" // '^' 以中间那些字符开头; '$' 以中间那些字符结尾
#define PASSWORD_REG "^[a-zA-Z\\d@_#]\{6,16\}$"
#define PHONE_REG "1\\d\{10\}"
#define EMAIL_REG "^[a-zA-Z\\d\._-]\+@[a-zA-Z\\d_\.-]\+(\.[a-zA-Z\\d_-]\+)+$"

#define IP "47.109.133.176"
#define PORT 8888

#define DEBUGPRINTF 0

// 文件信息
struct FileInfo
{
    QString md5;            // 文件md5码
    QString fileName;       // 文件名字
    QString account;        // 用户
    QString createTime;     // 上传时间
    QString url;            // url
    QString path;			// 文件父级路径（文件目录路径）
    QString type;           // 文件类型
    qint64 size;            // 文件大小
    int shareStatus;        // 是否共享, 1共享， 0不共享
    int downloads;                 // 下载量
    QListWidgetItem *item;  // list widget 的item
};

// 传输状态
enum class TransferStatus : char {Download, Uplaod, Recod};



class Common : public QObject {
    Q_OBJECT
public:
    explicit Common(QObject *parent = nullptr);
    ~Common();

    // 窗口移动到屏幕中央
    void WindowMoveToCenter(QWidget* win);

    // 获取某个文件的 md5 编码
    QString getFileMd5(const QString& filePath);

    // 将字符串加密，生成 md5 编码
    QString getStrMd5(QString str = "");

    // 获取服务器回复的状态码
    QString getStatusCode(QByteArray json);

    // 通过读取文件，得到文件类型，存放在typelist中
    void getFileTypeList();

    // 得到文件后缀，参数为文件类型，函数内部判断是否有此类型，如果有，使用此类型，没有，使用other.png
    QString getFileType(QString type);

    // 传输数据记录到本地文件，user：操作用户，name：操作的文件, code: 操作码， path: 文件保存的路径
    void writeRecord(QString account, QString name, QString code);

    // 产生分隔线
    QString getBoundary();


public:
    // 单例模式构建一个 http 通信类对象
    static QNetworkAccessManager* getNetManager();
    // 存放文件类型的列表
    static QStringList m_typeList;
    // 文件类型路径
    static QString m_typePath;
    // recore路径
    static QString m_recordPath;

};

#endif // COMMON_H
