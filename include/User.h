// User.h
#ifndef USER_H
#define USER_H

#include <QString>
#include <QVector>
#include <QColor>
#include <QObject>  // Add proper include for QObject
#include <memory>
#include <QSharedFromThis> // Add for shared_from_this functionality

class Document;

// Base User interface
class User : public QObject, public std::enable_shared_from_this<User>
{
    Q_OBJECT

public:
    User(const QString& id, const QString& name, const QString& mail);
    virtual ~User();
    
    QString getUserId() const { return userId; }
    QString getUsername() const { return username; }
    QString getEmail() const { return email; }
    QColor getCursorColor() const { return cursorColor; }
    
    void setCursorColor(const QColor& color) { cursorColor = color; }
    
    virtual bool authenticate(const QString& password) = 0;
    virtual bool manageSession() = 0;
    
    virtual bool canCreateDocuments() const = 0;
    virtual bool canShareDocuments() const = 0;

signals:
    void userStatusChanged();

protected:
    QString userId;
    QString username;
    QString email;
    QString passwordHash;
    QString sessionToken;
    QColor cursorColor;
};

// Registered User implementation
class RegisteredUser : public User
{
    Q_OBJECT

public:
    RegisteredUser(const QString& id, const QString& name, const QString& mail);
    ~RegisteredUser() override;
    
    bool authenticate(const QString& password) override;
    bool manageSession() override;
    
    bool canCreateDocuments() const override { return true; }
    bool canShareDocuments() const override { return true; }
    
    std::shared_ptr<Document> createDocument(const QString& title);
    bool shareDocument(std::shared_ptr<Document> doc, std::shared_ptr<User> collaborator, bool editPermission = false);
    
    QVector<std::shared_ptr<Document>> getOwnedDocuments() const;
    QVector<std::shared_ptr<Document>> getSharedDocuments() const;

private:
    QVector<std::shared_ptr<Document>> documentsOwned;
    QVector<std::shared_ptr<Document>> documentsShared;
};

// Guest User implementation
class GuestUser : public User
{
    Q_OBJECT

public:
    GuestUser(const QString& id, const QString& name);
    ~GuestUser() override;
    
    bool authenticate(const QString& /* password */) override { return true; }
    bool manageSession() override;
    
    bool canCreateDocuments() const override { return false; }
    bool canShareDocuments() const override { return false; }
    
    bool requestAccess(std::shared_ptr<Document> document);
    bool upgradeAccount(const QString& email, const QString& password);
};

#endif // USER_H