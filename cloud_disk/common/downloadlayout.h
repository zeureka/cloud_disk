#ifndef DOWNLOADLAYOUT_H
#define DOWNLOADLAYOUT_H

#include "common.h"
#include <QVBoxLayout>

/**
 * @brief 下载进度布局类，单例模式
 */
class DownloadLayout {
private:
    DownloadLayout() = default;
    ~DownloadLayout() = default;
    DownloadLayout(const DownloadLayout&) = delete;
    DownloadLayout& operator=(const DownloadLayout&) = delete;

public:
    static DownloadLayout* getInstance();
    // 设置布局
    void setDownloadLayout(QWidget* p);
    // 获取布局
    QLayout* getDownloadLayout();

private:
    QLayout* m_layout;
    QWidget* m_wg;

};

#endif // DOWNLOADLAYOUT_H

