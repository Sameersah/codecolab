// CollaborationManager.cpp
#include "CollaborationManager.h"
#include "Document.h"
#include "CollaborationClient.h"

#include <QDebug>

CollaborationManager::CollaborationManager(QObject *parent)
    : QObject(parent)
{
}

CollaborationManager::~CollaborationManager() = default;

bool CollaborationManager::joinDocument(std::shared_ptr<Document> document, const QString& userId)
{
    if (!document) {
        return false;
    }

    QString documentId = document->getId();

    // Create document session if it doesn't exist
    if (!activeDocuments.contains(documentId)) {
        DocumentSession session;
        session.document = document;
        activeDocuments[documentId] = session;
    }

    // Add user to active users
    activeDocuments[documentId].activeUsers[userId] = true;

    // Emit signal
    emit documentJoined(documentId, userId);

    return true;
}

bool CollaborationManager::leaveDocument(std::shared_ptr<Document> document, const QString& userId)
{
    if (!document) {
        return false;
    }

    QString documentId = document->getId();

    // Check if document session exists
    if (!activeDocuments.contains(documentId)) {
        return false;
    }

    // Remove user from active users
    activeDocuments[documentId].activeUsers.remove(userId);

    // Remove document session if no more active users
    if (activeDocuments[documentId].activeUsers.isEmpty()) {
        activeDocuments.remove(documentId);
    }

    // Emit signal
    emit documentLeft(documentId, userId);

    return true;
}

void CollaborationManager::synchronizeChanges(const EditOperation& operation)
{
    // Add operation to queue
    editQueue.push(operation);

    // Process queue
    processEditQueue();
}

void CollaborationManager::broadcastCursorPosition(const QString& documentId, const QString& userId, int position)
{
    // Emit signal directly (in a real implementation, this might go through a more complex process)
    emit cursorUpdated(documentId, userId, position);
}

void CollaborationManager::broadcastChatMessage(const QString& documentId, const QString& userId, const QString& message)
{
    // Emit signal directly (in a real implementation, this might go through a more complex process)
    emit chatMessageReceived(documentId, userId, message);
}

void CollaborationManager::processEditQueue()
{
    while (!editQueue.empty()) {
        // Get next operation
        EditOperation operation = editQueue.front();
        editQueue.pop();

        // Check if document exists
        if (!activeDocuments.contains(operation.documentId)) {
            continue;
        }

        // Get document
        std::shared_ptr<Document> document = activeDocuments[operation.documentId].document;

        // In a real implementation, we would transform operations for conflict resolution
        // For the prototype, we'll just emit the operation as is
        emit changesSynchronized(operation);
    }
}