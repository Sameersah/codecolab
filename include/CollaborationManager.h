// CollaborationManager.h
#ifndef COLLABORATIONMANAGER_H
#define COLLABORATIONMANAGER_H

#include <QObject>
#include <QMap>
#include <QVector>
#include <queue>
#include <memory>
#include "EditOperation.h" // Include instead of forward declaration

class Document;

class CollaborationManager : public QObject
{
    Q_OBJECT

public:
    explicit CollaborationManager(QObject *parent = nullptr);
    ~CollaborationManager();

    bool joinDocument(std::shared_ptr<Document> document, const QString& userId);
    bool leaveDocument(std::shared_ptr<Document> document, const QString& userId);

    void synchronizeChanges(const EditOperation& operation);
    void broadcastCursorPosition(const QString& documentId, const QString& userId, int position);
    void broadcastChatMessage(const QString& documentId, const QString& userId, const QString& message);

    signals:
        void documentJoined(const QString& documentId, const QString& userId);
    void documentLeft(const QString& documentId, const QString& userId);
    void changesSynchronized(const EditOperation& operation);
    void cursorUpdated(const QString& documentId, const QString& userId, int position);
    void chatMessageReceived(const QString& documentId, const QString& userId, const QString& message);

private:
    struct DocumentSession {
        std::shared_ptr<Document> document;
        QMap<QString, bool> activeUsers; // userId -> isActive
    };

    QMap<QString, DocumentSession> activeDocuments; // documentId -> DocumentSession
    std::queue<EditOperation> editQueue;

    void processEditQueue();
};

#endif // COLLABORATIONMANAGER_H