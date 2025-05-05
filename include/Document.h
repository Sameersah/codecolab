#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QString>
#include <QMap>
#include <QVector>
#include <QDateTime>
#include <memory>
#include "User.h"

class User;

// Version history tracking
struct DocumentVersion {
    QString content;
    QString userId;
    QDateTime timestamp;
    QString description;
};

class Document : public QObject
{
    Q_OBJECT

public:
    enum class AccessLevel {
        None,
        ReadOnly,
        Edit
    };

    Document(const QString& id, const QString& title, std::shared_ptr<User> owner);
    ~Document();

    QString getId() const { return documentId; }
    QString getTitle() const { return title; }
    QString getContent() const { return content; }
    QString getLanguage() const { return language; }
    std::shared_ptr<User> getOwner() const { return owner; }
    QDateTime getLastModified() const { return lastModified; }
    QMap<QString, AccessLevel> getSharedWith() const { return sharedUsers; }
    bool isPubliclyAccessible() const { return isPublic; }
    
    void setTitle(const QString& newTitle);
    void setContent(const QString& newContent);
    void setLanguage(const QString& newLanguage);
    void setPublicAccess(bool isPublic);
    
    bool editContent(const QString& newContent, std::shared_ptr<User> editor);
    bool updateContent(const QString& delta, int position, std::shared_ptr<User> editor);
    
    bool addCollaborator(std::shared_ptr<User> user, bool canEdit);
    bool removeCollaborator(std::shared_ptr<User> user);
    bool hasEditPermission(std::shared_ptr<User> user) const;
    
    QVector<std::shared_ptr<User>> getCollaborators() const;
    
    bool saveVersion(const QString& description, std::shared_ptr<User> user);
    QVector<DocumentVersion> getVersionHistory() const;
    bool restoreVersion(int versionIndex);

    // Access control methods
    AccessLevel getAccessLevel(const QString& userId) const;
    bool shareWith(const QString& userId, AccessLevel level);
    bool revokeAccess(const QString& userId);
    bool canEdit(const QString& userId) const;
    bool canRead(const QString& userId) const;

signals:
    void contentChanged(const QString& newContent);
    void titleChanged(const QString& newTitle);
    void languageChanged(const QString& newLanguage);
    void collaboratorAdded(std::shared_ptr<User> user);
    void collaboratorRemoved(std::shared_ptr<User> user);
    void versionSaved(const DocumentVersion& version);
    void publicAccessChanged(bool isPublic);

private:
    QString documentId;
    QString title;
    QString content;
    QString language;
    std::shared_ptr<User> owner;
    QDateTime lastModified;
    QMap<QString, AccessLevel> sharedUsers; // userId -> AccessLevel
    bool isPublic; // Flag for public read-only access
    
    QMap<QString, bool> collaborators; // userId -> canEdit
    QVector<DocumentVersion> versionHistory;
    
    void addToVersionHistory(std::shared_ptr<User> user, const QString& description);
};

#endif // DOCUMENT_H