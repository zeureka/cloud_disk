#include "filepreview.h"
#include "ui_filepreview.h"
#include <QNetworkReply>
#include <QUrl>
#include <QLabel>
#include <QMovie>

FilePreview::FilePreview(QWidget *parent) : QDialog(parent), ui(new Ui::FilePreview) {
    ui->setupUi(this);
    this->m_manager = Common::getNetManager();
}

FilePreview::~FilePreview() {
    delete ui;
}

void FilePreview::setInfo(FileInfo *info) {
    QString fileName = info->fileName;
    QString type = info->type;
    QString url = info->url;

    QNetworkReply* reply = this->m_manager->get(QNetworkRequest(QUrl(url)));
    if (nullptr == reply) {
#if DEBUGPRINTF
        cout << "preview file reply is nullptr";
#endif
        return;
    }

    connect(reply, &QNetworkReply::finished, this, [=](){
        if (QNetworkReply::NoError != reply->error()) {
#if DEBUGPRINTF
            cout << "preview file failed, error = " << reply->errorString();
#endif
            reply->deleteLater();
            return;
        }

        QByteArray data = reply->readAll();
        reply->deleteLater();

        if ("jpg" == type || "jpeg" == type || "png" == type || "webp" == type) {
            // QPixmap pixmap("../fileType/gif.png");
            QPixmap pixmap;
            pixmap.loadFromData(data);
            ui->label->setPixmap(pixmap.scaled(ui->label->size()));
        } else if ("cpp" == type || "cc" == type || "h" == type || "c" == type ||
                "py" == type || "java" == type || "hpp" == type || "js" == type ||
                "html" == type || "css" == type || "md" == type || "txt" == type){
            ui->label->setWordWrap(true);
            ui->label->setText(QString::fromUtf8(data));
        } else if ("gif" == type) {
            QMovie* movie = new QMovie(this);
            movie->setFormat(data);
            ui->label->setMovie(movie);
            movie->start();
        } else {
            ui->label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            ui->label->setText("抱歉, 该文件格式无法预览, 请关闭弹窗!");
        }

        setWindowTitle(fileName);
    });
}
