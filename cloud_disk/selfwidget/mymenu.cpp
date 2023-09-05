#include "mymenu.h"

MyMenu::MyMenu(QWidget *parent) : QMenu{parent} {
    this->setStyleSheet(
        "QMenu { background: rgba(255,255,255,1); border: none; }"
        "QMenu::item { padding: 10px 32px; color: rgba(51,51,51,1); font-size: 14px; }"
        "QMenu::item:hover { background-color: #409CE1; }"
        " QMenu::item:selected { background-color: #409CE1; }"
    );
}
