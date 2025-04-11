#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <memory>

namespace Ui {
    class LoginDialog;
}

class User;

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

    std::shared_ptr<User> getUser() const;

    private slots:
        void onLoginButtonClicked();
    void onRegisterButtonClicked();
    void onGuestLoginToggled(bool checked);
    void validateInput();

private:
    Ui::LoginDialog *ui;
    std::shared_ptr<User> user;

    bool login(const QString& username, const QString& password);
    bool registerUser(const QString& username, const QString& email, const QString& password);
    bool loginAsGuest(const QString& name);
};

#endif // LOGINDIALOG_H