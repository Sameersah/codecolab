#include "LoginDialog.h"
#include "ui_LoginDialog.h"
#include "User.h"
#include "UserStorage.h"
#include <QDateTime>
#include <QPushButton>
#include <QMessageBox>
#include <QDebug>
#include <QJsonObject>


LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    
    // Set up connections
    connect(ui->loginButton, &QPushButton::clicked, this, &LoginDialog::onLoginButtonClicked);
    connect(ui->registerButton, &QPushButton::clicked, this, &LoginDialog::onRegisterButtonClicked);
    connect(ui->guestCheckBox, &QCheckBox::toggled, this, &LoginDialog::onGuestLoginToggled);
    
    // Connect text fields for validation
    connect(ui->usernameEdit, &QLineEdit::textChanged, this, &LoginDialog::validateInput);
    connect(ui->passwordEdit, &QLineEdit::textChanged, this, &LoginDialog::validateInput);
    connect(ui->emailEdit, &QLineEdit::textChanged, this, &LoginDialog::validateInput);
    
    // Set window properties
    setWindowTitle("CodeColab - Login");
    setMinimumWidth(350);
    
    // Initial validation
    validateInput();
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

std::shared_ptr<User> LoginDialog::getUser() const
{
    return user;
}

void LoginDialog::onLoginButtonClicked()
{
    QString username = ui->usernameEdit->text();
    QString password = ui->passwordEdit->text();
    
    if (ui->guestCheckBox->isChecked()) {
        if (loginAsGuest(username)) {
            accept();
        } else {
            QMessageBox::warning(const_cast<LoginDialog*>(this), 
                                "Login Failed", 
                                "Failed to login as guest.");
        }
    } else {
        if (login(username, password)) {
            accept();
        } else {
            QMessageBox::warning(const_cast<LoginDialog*>(this), 
                                "Login Failed", 
                                "Invalid username or password.");
        }
    }
}

void LoginDialog::onRegisterButtonClicked()
{
    QString username = ui->usernameEdit->text();
    QString email = ui->emailEdit->text();
    QString password = ui->passwordEdit->text();
    
    if (registerUser(username, email, password)) {
        QMessageBox::information(this, 
                               "Registration Successful", 
                               "Your account has been created. You can now log in.");
    } else {
        QMessageBox::warning(this, 
                           "Registration Failed", 
                           "Failed to register user. The username might already exist.");
    }
}

void LoginDialog::onGuestLoginToggled(bool checked)
{
    // Enable/disable fields based on guest mode
    ui->emailLabel->setVisible(!checked);
    ui->emailEdit->setVisible(!checked);
    ui->passwordLabel->setEnabled(!checked);
    ui->passwordEdit->setEnabled(!checked);
    ui->registerButton->setEnabled(!checked);
    
    // Adjust username label
    if (checked) {
        ui->usernameLabel->setText("Guest Name:");
    } else {
        ui->usernameLabel->setText("Username:");
    }
    
    // Re-validate
    validateInput();
}

void LoginDialog::validateInput()
{
    bool valid = false;
    
    if (ui->guestCheckBox->isChecked()) {
        // For guest login, only need a non-empty name
        valid = !ui->usernameEdit->text().isEmpty();
    } else {
        // For regular login, need username and password
        valid = !ui->usernameEdit->text().isEmpty() && 
                !ui->passwordEdit->text().isEmpty();
        
        // For registration, also need email
        bool registerValid = valid && !ui->emailEdit->text().isEmpty() && 
                           ui->emailEdit->text().contains('@');
        
        ui->registerButton->setEnabled(registerValid);
    }
    
    ui->loginButton->setEnabled(valid);
}

bool LoginDialog::login(const QString& username, const QString& password)
{
    QJsonObject users = UserStorage::readUsersFromFile();

    if (users.contains(username)) {
        QJsonObject userInfo = users[username].toObject();
        if (userInfo["password"].toString() == password) {
            QString email = userInfo["email"].toString();
            user = std::make_shared<RegisteredUser>(username, username, email);
            return user->manageSession(); // Simulated session
        }
    }

    return false;
}


bool LoginDialog::registerUser(const QString& username, const QString& email, const QString& password)
{
    QJsonObject users = UserStorage::readUsersFromFile();

    if (users.contains(username)) {
        return false; // User already exists
    }

    QJsonObject newUser;
    newUser["email"] = email;
    newUser["password"] = password;

    users[username] = newUser;

    if (!UserStorage::writeUsersToFile(users)) {
        return false;
    }

    // Set the user so MainWindow gets the new user
    user = std::make_shared<RegisteredUser>(username, username, email);
    return true;
}


bool LoginDialog::loginAsGuest(const QString& name)
{
    if (name.isEmpty()) {
        return false;
    }
    
    // Create a guest user object
    QString guestId = "guest_" + QString::number(QDateTime::currentMSecsSinceEpoch());
    user = std::make_shared<GuestUser>(guestId, name);
    
    // Create session
    return user->manageSession();
}
