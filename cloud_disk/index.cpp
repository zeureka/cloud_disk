#include "index.h"
#include "./ui_index.h"
#include "./common/iconhelper.h"


#include <QDebug>

QList<QColor> Index::colors = QList<QColor>();

Index::Index(QWidget *parent) : QWidget(parent), ui(new Ui::Index) {
    ui->setupUi(this);
    setWindowTitle("我的云盘");

    this->my_file_page = new MyFile(this);
    this->share_list_page = new ShareList(this);
    this->transfer_list_page = new TransferList(this);
    this->my_info_page = new MyInfo(this);

    this->initForm();
    this->initWidget();
    this->initNav();
    this->initIcon();

    // 处理信号
    this->managerSignals();
}

Index::~Index() {
    delete ui;
}

void Index::showMainWindow(){
    // 显示页面
    this->m_common.WindowMoveToCenter(this);
}

void Index::managerSignals() {
    connect(this->my_file_page, &MyFile::loginAgainSignal, this, &Index::loginAgain);
    connect(this->my_info_page, &MyInfo::loginAgainSignal, this, &Index::loginAgain);
}

void Index::loginAgain() {
#if DEBUGPRINTF
    cout << "Bye Bye ~";
#endif

    // 发送信号告诉登录窗口，切换用户
    emit changeAccount();
    // 清空上一个用户上传或下载的任务
    this->my_file_page->clearAllTask();
    // 清空上一个用户的显示信息
    this->my_file_page->clearFileList();
    this->my_file_page->clearItems();
    this->my_info_page->clearInfo();
}


// ======================> 侧边导航栏 <=====================

void Index::initForm() {
    // 颜色集合供其他界面使用
    colors << QColor(211, 78, 78) << QColor(29, 185, 242) << QColor(170, 162, 119) << QColor(255, 192, 1) << QColor(192, 168, 172);

    ui->navigate_sa->setFixedWidth(170);
    ui->widgetLeft->setProperty("flag", "left");
}

void Index::initWidget() {
    ui->stackedWidget->addWidget(this->my_file_page);
    ui->stackedWidget->addWidget(this->share_list_page);
    ui->stackedWidget->addWidget(this->transfer_list_page);
    ui->stackedWidget->addWidget(this->my_info_page);
}

void Index::initNav() {
    //按钮文字集合
    QStringList names;
    names << "个人文件" << "共享列表" << "传输列表" << "个人中心";

    //自动生成按钮
    for (int i = 0; i < names.count(); i++) {
        QToolButton *btn = new QToolButton(this);
        //设置按钮固定高度
        btn->setFixedHeight(35);
        //设置按钮的文字
        btn->setText(QString("%1").arg(names.at(i)));
        //设置按钮可选中按下类似复选框的功能
        btn->setCheckable(true);
        //设置按钮图标在左侧文字在右侧
        btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        //设置按钮拉伸策略为横向填充
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        //关联按钮单击事件
        connect(btn, SIGNAL(clicked(bool)), this, SLOT(buttonClicked()));
        //将按钮加入到布局
        ui->widgetLeft->layout()->addWidget(btn);
        //可以指定不同的图标
        icons << 0xf061;
        btns << btn;
    }

    // 底部加个弹簧
    QSpacerItem *verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
    ui->widgetLeft->layout()->addItem(verticalSpacer);

    // 设置默认首页
    btns.at(0)->click();
}

void Index::initIcon() {
    // 左侧导航样式,可以设置各种牛逼的参数
    IconHelper::StyleColor styleColor;
    styleColor.defaultBorder = true;
    IconHelper::setStyle(ui->widgetLeft, btns, icons, styleColor);

    ui->widgetLeft->setStyleSheet("background-color: rgb(220, 220, 220);");
}

void Index::buttonClicked() {
    QAbstractButton *btn = (QAbstractButton *)sender();
    int count = btns.count();
    int index = btns.indexOf(btn);
    ui->stackedWidget->setCurrentIndex(index);

    for (int i = 0; i < count; i++) {
        QAbstractButton *tmp = btns.at(i);
        tmp->setChecked(tmp == btn);
    }
}
