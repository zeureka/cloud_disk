#include "downloadlayout.h"

DownloadLayout* DownloadLayout::getInstance() {
    static DownloadLayout instance;
    return &instance;
}

void DownloadLayout::setDownloadLayout(QWidget* p) {
    this->m_wg = new QWidget(p);
    QLayout* layout = p->layout();
    layout->addWidget(m_wg);
    layout->setContentsMargins(0, 0, 0, 0);
    QVBoxLayout* vlayout = new QVBoxLayout;

    // 布局设置给窗口
    m_wg->setLayout(vlayout);

    // 边界间隔
    vlayout->setContentsMargins(0, 0, 0, 0);
    m_layout = vlayout;

    vlayout->addStretch();
}

// 获取布局
QLayout* DownloadLayout::getDownloadLayout() {
    return this->m_layout;
}
