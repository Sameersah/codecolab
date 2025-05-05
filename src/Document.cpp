// Document.cpp
#include "Document.h"
#include "User.h"
#include <QDateTime>
#include <QDebug>

Document::Document(const QString& id, const QString& title, std::shared_ptr<User> owner)
    : QObject(nullptr)
    , documentId(id)
    , title(title)
    , content("")
    , language("Plain")
    , owner(owner)
    , lastModified(QDateTime::currentDateTime())
    , isPublic(false)
{
    if (owner) {
        qDebug() << "Created document" << id << "owned by" << owner->getUserId();
        // Owner gets edit access by default
        sharedUsers[owner->getUserId()] = AccessLevel::Edit;
    }
    // Add first version to history
    addToVersionHistory(owner, "Initial document creation");
}

Document::~Document() = default;

void Document::setTitle(const QString& newTitle)
{
    if (title != newTitle) {
        title = newTitle;
        emit titleChanged(title);
    }
}

void Document::setContent(const QString& newContent)
{
    if (content != newContent) {
        content = newContent;
        lastModified = QDateTime::currentDateTime();
        emit contentChanged(content);
    }
}

void Document::setLanguage(const QString& newLanguage)
{
    if (language != newLanguage) {
        language = newLanguage;
        emit languageChanged(language);
    }
}

bool Document::editContent(const QString& newContent, std::shared_ptr<User> editor)
{
    if (!editor) return false;
    
    // Check if user has edit access
    if (!canEdit(editor->getUserId())) {
        qDebug() << "User" << editor->getUserId() << "does not have edit access to document" << documentId;
        return false;
    }
    
    content = newContent;
    lastModified = QDateTime::currentDateTime();
    emit contentChanged(newContent);
    return true;
}

bool Document::updateContent(const QString& delta, int position, std::shared_ptr<User> editor)
{
    if (!hasEditPermission(editor)) {
        return false;
    }

    // Apply the delta at the specified position
    if (position >= 0 && position <= content.length()) {
        QString newContent = content;
        newContent.insert(position, delta);
        setContent(newContent);
        return true;
    }

    return false;
}

bool Document::addCollaborator(std::shared_ptr<User> user, bool canEdit)
{
    if (!user) {
        return false;
    }

    collaborators[user->getUserId()] = canEdit;
    emit collaboratorAdded(user);
    return true;
}

bool Document::removeCollaborator(std::shared_ptr<User> user)
{
    if (!user) {
        return false;
    }

    if (collaborators.remove(user->getUserId()) > 0) {
        emit collaboratorRemoved(user);
        return true;
    }

    return false;
}

bool Document::hasEditPermission(std::shared_ptr<User> user) const
{
    if (!user) {
        return false;
    }

    // The owner always has edit permission
    if (owner && user->getUserId() == owner->getUserId()) {
        return true;
    }

    // Check if user is a collaborator with edit permission
    auto it = collaborators.find(user->getUserId());
    if (it != collaborators.end()) {
        return it.value();
    }

    return false;
}

QVector<std::shared_ptr<User>> Document::getCollaborators() const
{
    QVector<std::shared_ptr<User>> result;
    // In a real implementation, we would convert the map of IDs to User objects
    // For the prototype, this is a placeholder
    return result;
}

bool Document::saveVersion(const QString& description, std::shared_ptr<User> user)
{
    if (!hasEditPermission(user)) {
        return false;
    }

    addToVersionHistory(user, description);
    return true;
}

QVector<DocumentVersion> Document::getVersionHistory() const
{
    return versionHistory;
}

bool Document::restoreVersion(int versionIndex)
{
    if (versionIndex < 0 || versionIndex >= versionHistory.size()) {
        return false;
    }

    content = versionHistory[versionIndex].content;
    emit contentChanged(content);

    return true;
}

void Document::addToVersionHistory(std::shared_ptr<User> user, const QString& description)
{
    DocumentVersion version;
    version.content = content;
    version.userId = user ? user->getUserId() : "";
    version.timestamp = QDateTime::currentDateTime();
    version.description = description;

    versionHistory.append(version);
    emit versionSaved(version);
}

void Document::setPublicAccess(bool publicAccess)
{
    if (isPublic != publicAccess) {
        isPublic = publicAccess;
        emit publicAccessChanged(isPublic);
    }
}

Document::AccessLevel Document::getAccessLevel(const QString& userId) const
{
    // Owner always has edit access
    if (owner && userId == owner->getUserId()) {
        qDebug() << "User is owner, granting edit access";
        return AccessLevel::Edit;
    }

    // Check if document is public
    if (isPublic) {
        qDebug() << "Document is public, granting read-only access";
        return AccessLevel::ReadOnly;
    }

    // Check if user has explicit access
    auto it = sharedUsers.find(userId);
    if (it != sharedUsers.end()) {
        qDebug() << "User has explicit access level:" << (it.value() == AccessLevel::Edit ? "Edit" : "ReadOnly");
        return it.value();
    }

    qDebug() << "User has no access to document";
    return AccessLevel::None;
}

bool Document::shareWith(const QString& userId, AccessLevel level)
{
    qDebug() << "Attempting to share document" << documentId << "with user" << userId 
             << "at level" << (level == AccessLevel::Edit ? "edit" : "read-only");
    
    // Can't share with owner (they already have access)
    if (owner && owner->getUserId() == userId) {
        qDebug() << "Cannot share with owner";
        return false;
    }
    
    sharedUsers[userId] = level;
    qDebug() << "Successfully shared document with user";
    return true;
}

bool Document::revokeAccess(const QString& userId)
{
    qDebug() << "Attempting to revoke access for user" << userId << "from document" << documentId;
    
    // Can't revoke owner's access
    if (owner && owner->getUserId() == userId) {
        qDebug() << "Cannot revoke owner's access";
        return false;
    }
    
    if (sharedUsers.remove(userId) > 0) {
        qDebug() << "Successfully revoked access";
        return true;
    }
    
    qDebug() << "User did not have access to revoke";
    return false;
}

bool Document::canRead(const QString& userId) const
{
    return getAccessLevel(userId) != AccessLevel::None;
}

bool Document::canEdit(const QString& userId) const
{
    return getAccessLevel(userId) == AccessLevel::Edit;
}