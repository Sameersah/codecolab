// User.cpp
#include "User.h"
#include "Document.h"
#include <QDateTime>
#include <QDebug>

// User base class implementation
User::User(const QString& id, const QString& name, const QString& mail)
    : QObject(nullptr)
    , userId(id)
    , username(name)
    , email(mail)
    , passwordHash("")
    , sessionToken("")
{
    // Generate a random color for this user's cursor
    // Use the userId to make it deterministic
    int hash = 0;
    for (QChar c : userId) {
        hash = (hash * 31) + c.unicode();
    }

    // Create a hue between 0 and 359 (exclude red which is for errors)
    int hue = (hash % 300) + 30;
    cursorColor = QColor::fromHsv(hue, 255, 255);
}

User::~User() = default;

// RegisteredUser implementation
RegisteredUser::RegisteredUser(const QString& id, const QString& name, const QString& mail)
    : User(id, name, mail)
{
}

RegisteredUser::~RegisteredUser() = default;

bool RegisteredUser::authenticate(const QString& password)
{
    // In a real implementation, this would validate the password hash
    // For the prototype, accept any non-empty password
    if (password.isEmpty()) {
        return false;
    }

    // Store the hashed password (simplified)
    passwordHash = password + "_hashed";
    return true;
}

bool RegisteredUser::manageSession()
{
    // Generate a session token
    QString timestamp = QString::number(QDateTime::currentMSecsSinceEpoch());
    sessionToken = userId + "_" + timestamp;
    emit userStatusChanged();
    return true;
}

std::shared_ptr<Document> RegisteredUser::createDocument(const QString& title)
{
    // Generate a unique document ID
    QString docId = "doc_" + QString::number(QDateTime::currentMSecsSinceEpoch());

    // Create the document
    auto doc = std::make_shared<Document>(docId, title,
        std::dynamic_pointer_cast<User>(shared_from_this()));

    // Add to owned documents
    documentsOwned.append(doc);

    return doc;
}

bool RegisteredUser::shareDocument(std::shared_ptr<Document> doc, std::shared_ptr<User> collaborator, bool editPermission)
{
    // Check if this user owns the document
    auto it = std::find(documentsOwned.begin(), documentsOwned.end(), doc);
    if (it == documentsOwned.end()) {
        return false;
    }

    // Add collaborator to the document
    return doc->addCollaborator(collaborator, editPermission);
}

QVector<std::shared_ptr<Document>> RegisteredUser::getOwnedDocuments() const
{
    return documentsOwned;
}

QVector<std::shared_ptr<Document>> RegisteredUser::getSharedDocuments() const
{
    return documentsShared;
}

// GuestUser implementation
GuestUser::GuestUser(const QString& id, const QString& name)
    : User(id, name, "")
{
}

GuestUser::~GuestUser() = default;

bool GuestUser::manageSession()
{
    // Generate a temporary session token
    QString timestamp = QString::number(QDateTime::currentMSecsSinceEpoch());
    sessionToken = "guest_" + userId + "_" + timestamp;
    emit userStatusChanged();
    return true;
}

bool GuestUser::requestAccess(std::shared_ptr<Document> document)
{
    // In a real implementation, this would send a request to the document owner
    // For the prototype, we'll simulate a simple request
    qDebug() << "Guest " << username << " requests access to document " << document->getTitle();
    return true;
}

bool GuestUser::upgradeAccount(const QString& email, const QString& password)
{
    // In a real implementation, this would convert the guest account to a registered account
    // For the prototype, we'll just update the email and password
    if (email.isEmpty() || password.isEmpty()) {
        return false;
    }

    this->email = email;
    this->passwordHash = password + "_hashed";

    // This would actually create a new RegisteredUser account in a real implementation
    qDebug() << "Guest account upgraded for " << username;
    return true;
}