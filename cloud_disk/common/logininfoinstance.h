#ifndef LOGININFOINSTANCE_H
#define LOGININFOINSTANCE_H

#include <QString>

class LoginInfoInstance {
private:
    LoginInfoInstance() = default;
    ~LoginInfoInstance() = default;
    LoginInfoInstance(const LoginInfoInstance&) = default;
    LoginInfoInstance& operator=(const LoginInfoInstance&) = default;

public:
    static LoginInfoInstance* getInstance();

public:
    void setLoginInfo(const QString& account, const QString& token);
    QString getAccount() const;
    QString getToken() const;

private:
    QString account;
    QString token;
};

#endif // LOGININFOINSTANCE_H
