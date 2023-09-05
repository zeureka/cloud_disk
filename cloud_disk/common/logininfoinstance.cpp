#include "logininfoinstance.h"

LoginInfoInstance* LoginInfoInstance::getInstance() {
    static LoginInfoInstance instance;
    return &instance;
}

void LoginInfoInstance::setLoginInfo(const QString &account, const QString &token) {
    this->account = account;
    this->token = token;
}

QString LoginInfoInstance::getAccount() const {
    return this->account;
}

QString LoginInfoInstance::getToken() const {
    return this->token;
}
