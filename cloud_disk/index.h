#ifndef INDEX_H
#define INDEX_H

#include <QWidget>
#include <QAbstractButton>
#include "common/common.h"

#include "myfile.h"
#include "sharelist.h"
#include "transferlist.h"
#include "frmtrash.h"
#include "myinfo.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Index; }
QT_END_NAMESPACE

class Index : public QWidget {
    Q_OBJECT

public:
    Index(QWidget *parent = nullptr);
    ~Index();

    // 显示主窗口
    void showMainWindow();

    // 管理信号
    void managerSignals();


private:
    Ui::Index *ui;
    Common m_common;
    static QList<QColor> colors;

    // 左侧导航栏图标 + 按钮集合
    QList<int> icons;
    QList<QAbstractButton*> btns;

    // 右侧widget
    MyFile* my_file_page;
    ShareList* share_list_page;
    TransferList* transfer_list_page;
    MyInfo* my_info_page;

signals:
    // 切换用户按钮信号
    void changeAccount();

private slots:
    void initForm();        //初始化界面数据
    void initWidget();      //初始化子窗体
    void initNav();         //初始化导航按钮
    void initIcon();        //初始化导航按钮图标
    void buttonClicked();   //导航按钮单击事件
    void loginAgain();		// 重新登录
};
#endif // INDEX_H
