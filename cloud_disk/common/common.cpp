#include <QScreen>
#include <QApplication>
#include <QDir>
#include <QTime>
#include <QRandomGenerator>

#include "common.h"
// QString Common::m_typePath = QDir::currentPath() + "/../client/cloud_disk/conf/fileType";
// QString Common::m_recordPath = QDir::currentPath() + "/../client/cloud_disk/conf/record/";
QString Common::m_recordPath = QDir::currentPath() + "/record/";
QString Common::m_typePath = ":/fileType";
QStringList Common::m_typeList = QStringList();

Common::Common(QObject *parent) : QObject{parent} {}

Common::~Common() {}

void Common::WindowMoveToCenter(QWidget *win) {
    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    int x = (screenGeometry.width() - win->width()) / 2;
    int y = (screenGeometry.height() - win->height()) / 2;
    win->move(x, y);

    // 显示窗口
    win->show();

}

QString Common::getFileMd5(const QString &filePath) {
    QFile file = QFile(filePath);
    // 文件打开失败
    if (!file.open(QIODevice::ReadOnly)) {

#if DEBUGPRINTF
        cout << "file open error";
#endif

        return NULL;
    }

    QCryptographicHash hash(QCryptographicHash::Md5);
    quint64 totalBytes = 0;
    quint64 bytesWritten = 0;
    quint64 bytesToWrite = 0;
    quint64 loadSize = 1024 * 4;
    QByteArray buf;

    totalBytes = file.size();
    bytesToWrite = totalBytes;

    while(true) {
        if (bytesToWrite > 0) {
            buf = file.read(qMin(loadSize, bytesToWrite));
            hash.addData(buf);
            bytesWritten += buf.length();
            bytesToWrite -= buf.length();
            buf.resize(0);
        } else {
            break;
        }

        if (bytesWritten == totalBytes) {
            break;
        }
    }

    file.close();
    QByteArray md5 = hash.result();
    return md5.toHex();
}

QString Common::getStrMd5(QString str) {
    QByteArray array;
    // 将 string 对象转换为本地8位编码的字节数组，方便在不支持 unicode 编码环境下使用
    array = QCryptographicHash::hash(str.toLocal8Bit(), QCryptographicHash::Md5);
    return array.toHex();
}

QString Common::getStatusCode(QByteArray json) {
    QJsonParseError error;

    // 将 JSON 数据转化为 QJsonDocument
    QJsonDocument doc = QJsonDocument::fromJson(json, &error);
    if (QJsonParseError::NoError == error.error) {
        if (doc.isNull() || doc.isEmpty()) {

#if DEBUGPRINTF
            cout << "doc.isNull() or doc.isEmpty()";
#endif
            return "";
        }

        if (doc.isObject()) {
            // 返回最外层的 object 的 key(code) 对应的 value
            return doc.object().value("code").toString();
        }
    } else {
#if DEBUGPRINTF
        cout << "error: " << error.errorString();
#endif
    }

    return "";
}

void Common::getFileTypeList() {
    // 使用相对路径或绝对路径，指向一个文件/目录
    QDir dir(m_typePath);
    if (!dir.exists()) {
        dir.mkpath(m_typePath);

#if DEBUGPRINTF
        cout << m_typePath << "创建成功！！！";
#endif
    }
    /*
        QDir::Dirs      		列出目录；
        QDir::AllDirs   		列出所有目录，不对目录名进行过滤；
        QDir::Files     		列出文件；
        QDir::Drives    		列出逻辑驱动器名称，该枚举变量在Linux/Unix中将被忽略；
        QDir::NoSymLinks        不列出符号链接；
        QDir::NoDotAndDotDot    不列出文件系统中的特殊文件.及..；
        QDir::NoDot             不列出.文件，即指向当前目录的软链接
        QDir::NoDotDot          不列出..文件；
        QDir::AllEntries        其值为Dirs | Files | Drives，列出目录、文件、驱动器及软链接等所有文件；
        QDir::Readable      	列出当前应用有读权限的文件或目录；
        QDir::Writable      	列出当前应用有写权限的文件或目录；
        QDir::Executable    	列出当前应用有执行权限的文件或目录；
        QDir::Modified      	列出已被修改的文件，该值在Linux/Unix系统中将被忽略；
        QDir::Hidden        	列出隐藏文件；
        QDir::System        	列出系统文件；
        QDir::CaseSensitive 	设定过滤器为大小写敏感。
        Readable、Writable及Executable均需要和Dirs或Files枚举值联合使用；

        QDir::Size              通过文件大小排序
        QDir::Reversed          翻转文件排序顺序
    */

    // 设置过滤条件
    dir.setFilter(QDir::Files | QDir::NoDot | QDir::NoDotDot | QDir::NoSymLinks);
    // 排序
    dir.setSorting(QDir::Size | QDir::Reversed);

    // dir 路径下的所有文件和目录
    QFileInfoList list = dir.entryInfoList();

#if DEBUGPRINTF
    cout << "file type size = " << list.size();
#endif

    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);
        m_typeList.append(fileInfo.fileName());
    }
}

QString Common::getFileType(QString type) {
    if (m_typeList.contains(type)) {
        return m_typePath + "/" + type;
    }

    return m_typePath + "/other.png";
}

void Common::writeRecord(QString account, QString name, QString code) {
    QString path = m_recordPath;

    // 文件名字，命名为登录的用户名
    QString fileName = m_recordPath + account;

    QDir dir(m_recordPath);
    if (!dir.exists()) {
        // 目录不存在，则创建
        if (dir.mkpath(m_recordPath)) {
            cout << m_recordPath << "创建成功！！！";
        } else {
            cout << m_recordPath << "创建失败！！！";
        }
    }

#if DEBUGPRINTF
    cout << "fileName = " << fileName.toUtf8().data();
#endif

    QByteArray fileData;
    QFile file(fileName);

    // 如果文件存在，先读取原来的内容
    if (file.exists()) {
        if(false == file.open(QIODevice::ReadOnly)) {
#if DEBUGPRINTF
            cout << "file.open(QIODevice::ReadOnly) error";
#endif
            return;
        }

        fileData = file.readAll();
        file.close();
    }

    if (false == file.open(QIODevice::WriteOnly)) {
#if DEBUGPRINTF
        cout << "file.open(QIODevice::WriteOnly) error";
#endif
        return;
    }

    // record package operation
    /*
       秒传文件：
            文件已存在：{"code":"005"}
            秒传成功：  {"code":"006"}
            秒传失败：  {"code":"007"}
        上传文件：
            成功：{"code":"008"}
            失败：{"code":"009"}
        下载文件：
            成功：{"code":"010"}
            失败：{"code":"011"}
    */
    QDateTime curTime = QDateTime::currentDateTime();
    QString timeStr = curTime.toString("yyyy-MM-dd hh:mm:ss ddd");

    QString actionStr;
    if(code == "005") {
        actionStr = "上传失败，文件已存在";
    } else if(code == "006") {
        actionStr = "秒传成功";
    } else if(code == "008") {
        actionStr = "上传成功";
    } else if(code == "009") {
        actionStr = "上传失败";
    } else if(code == "010") {
        actionStr = "下载成功";
    } else if(code == "011") {
        actionStr = "下载失败";
    }

    QString record = QString("[%1]\t%2\t[%3]\r\n").arg(name).arg(timeStr).arg(actionStr);

#if DEBUGPRINTF
    cout << record.toUtf8().data();
#endif

    file.write(record.toLocal8Bit());
    if (!fileData.isEmpty()) {
        file.write(fileData);
    }

    file.close();
}

QString Common::getBoundary() {
    QString tmp = "";

    // 48 ～ 122 '0'~'A'~'z'
    for (int i = 0; i < 16; ++i) {
        tmp += QChar(QRandomGenerator::global()->bounded(48, 123));
    }

    return QString("------WebKitFormBoundary%1").arg(tmp);
}

QNetworkAccessManager *Common::getNetManager() {
    static QNetworkAccessManager instance;
    return &instance;
}

// end
