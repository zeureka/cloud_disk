#include "uploadlayout.h"

UploadLayout* UploadLayout::getInstance() {
    static UploadLayout instance;
    return &instance;
}

// 设置布局
void UploadLayout::setUploadLayout(QWidget* p) {
    this->m_wg = new QWidget(p);
    QLayout* layout = p->layout();
    layout->addWidget(m_wg);
    layout->setContentsMargins(0, 0, 0, 0);
    QVBoxLayout* vlayout = new QVBoxLayout;

    // 布局设置给窗口
    this->m_wg->setLayout(vlayout);

    // 边界间隔
    vlayout->setContentsMargins(0, 0, 0, 0);
    this->m_layout = vlayout;

    // 添加弹簧
    vlayout->addStretch();
}

// 获取布局
QLayout* UploadLayout::getUploadLayout() {
    return this->m_layout;
}
