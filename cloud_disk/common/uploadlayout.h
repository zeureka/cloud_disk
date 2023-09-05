#ifndef UPLOADLAYOUT_H
#define UPLOADLAYOUT_H

#include "common.h"
#include <QVBoxLayout>

// 上传进度布局类，单例模式
class UploadLayout {
private:
    UploadLayout() = default;
    ~UploadLayout() = default;
    UploadLayout(const UploadLayout&) = delete;
    UploadLayout& operator=(const UploadLayout&) = delete;

public:
    static UploadLayout* getInstance();
    // 设置布局
    void setUploadLayout(QWidget* p);
    // 获取布局
    QLayout* getUploadLayout();

private:
    QLayout* m_layout;
    QWidget* m_wg;
};

#endif // UPLOADLAYOUT_H
