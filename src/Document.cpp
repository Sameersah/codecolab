// Document.cpp
#include "Document.h"
#include "User.h"
#include <QDateTime>

Document::Document(const QString& id, const QString& title, std::shared_ptr<User> owner)
    : QObject(nullptr)
    , documentId(id)
    , title(title)
    , content("")
    , language("Plain")
    , owner(owner)
{
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
    if (!hasEditPermission(editor)) {
        return false;
    }

    setContent(newContent);

    // Add to version history periodically (simplified)
    static int editCount = 0;
    if (++editCount % 10 == 0) {
        addToVersionHistory(editor, "Auto-saved");
    }

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