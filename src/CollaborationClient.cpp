// CollaborationClient.cpp
#include "CollaborationClient.h"
#include "User.h"
#include "Document.h"
#include "EditOperation.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QTimer>
#include <QDateTime>
#include <QEventLoop>

CollaborationClient::CollaborationClient(QObject *parent)
    : QObject(parent)
    , isDocumentJoined(false)
{
    // Connect WebSocket signals
    QObject::connect(&webSocket, &QWebSocket::connected, this, &CollaborationClient::onConnected);
    QObject::connect(&webSocket, &QWebSocket::disconnected, this, &CollaborationClient::onDisconnected);
    QObject::connect(&webSocket, &QWebSocket::textMessageReceived, this, &CollaborationClient::onTextMessageReceived);
    QObject::connect(&webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::errorOccurred),
                     this, &CollaborationClient::onError);
}

CollaborationClient::~CollaborationClient()
{
    if (webSocket.isValid()) {
        webSocket.close();
    }
}

bool CollaborationClient::connect(const QString& url)
{
    if (webSocket.isValid()) {
        webSocket.close();
    }

    serverUrl = url;
    webSocket.open(QUrl(serverUrl));

    // Wait for connection with timeout
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    
    QObject::connect(&webSocket, &QWebSocket::connected, &loop, &QEventLoop::quit);
    QObject::connect(&webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::errorOccurred),
            &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    
    timer.start(5000); // 5 second timeout
    loop.exec();
    
    if (timer.isActive()) {
        timer.stop();
        return webSocket.state() == QAbstractSocket::ConnectedState;
    } else {
        emit error("Connection timeout");
        return false;
    }
}

void CollaborationClient::disconnect()
{
    if (webSocket.isValid()) {
        webSocket.close();
    }
}

bool CollaborationClient::isConnected() const
{
    return webSocket.state() == QAbstractSocket::ConnectedState;
}

void CollaborationClient::setUser(std::shared_ptr<User> user)
{
    currentUser = user;
}

void CollaborationClient::setDocument(std::shared_ptr<Document> document)
{
    currentDocument = document;
}

void CollaborationClient::joinDocument(const QString& documentId)
{
    if (!currentUser) {
        emit error("No user is set");
        return;
    }

    // Construct a join message
    QJsonObject payload;
    payload["documentId"] = documentId;
    payload["userId"] = currentUser->getUserId();
    payload["username"] = currentUser->getUsername();

    sendMessage("join", payload);
    isDocumentJoined = true;
    emit documentJoined(documentId);
}

void CollaborationClient::leaveDocument()
{
    if (!isDocumentJoined) {
        return;
    }
    
    // Construct a leave message
    QJsonObject payload;
    if (currentDocument) {
        payload["documentId"] = currentDocument->getId();
    }
    if (currentUser) {
        payload["userId"] = currentUser->getUserId();
    }
    
    // Send the message
    sendMessage("leave", payload);
    
    // For the prototype, simulate successful leave
    QTimer::singleShot(300, [this]() {
        isDocumentJoined = false;
        emit documentLeft();
    });
}

void CollaborationClient::sendEdit(const EditOperation& operation)
{
    if (!isDocumentJoined) {
        emit error("Not joined to a document");
        return;
    }
    
    // Construct an edit message
    QJsonObject payload = operation.toJson();
    
    // Send the message
    sendMessage("edit", payload);
}

void CollaborationClient::sendCursorPosition(int position)
{
    if (!isDocumentJoined || !currentUser || !currentDocument) {
        return;
    }
    
    // Construct a cursor message
    QJsonObject payload;
    payload["documentId"] = currentDocument->getId();
    payload["userId"] = currentUser->getUserId();
    payload["username"] = currentUser->getUsername();
    payload["position"] = position;
    
    // Send the message
    sendMessage("cursor", payload);
}

void CollaborationClient::sendChatMessage(const QString& message)
{
    if (!isDocumentJoined || !currentUser || !currentDocument) {
        return;
    }
    
    // Construct a chat message
    QJsonObject payload;
    payload["documentId"] = currentDocument->getId();
    payload["userId"] = currentUser->getUserId();
    payload["username"] = currentUser->getUsername();
    payload["message"] = message;
    
    // Send the message
    sendMessage("chat", payload);
}

void CollaborationClient::requestLatestContent(const QString& documentId)
{
    if (!isDocumentJoined) {
        emit error("Not joined to a document");
        return;
    }
    
    // Construct a request message
    QJsonObject payload;
    payload["documentId"] = documentId;
    
    // Send the message
    sendMessage("request_content", payload);
}

void CollaborationClient::onConnected()
{
    emit connected();
    
    // For the prototype, simulate some users being available
    connectedUsers.clear();
}

void CollaborationClient::onDisconnected()
{
    isDocumentJoined = false;
    connectedUsers.clear();
    emit disconnected();
}

void CollaborationClient::onTextMessageReceived(const QString& message)
{
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) return;

    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();
    QJsonObject payload = obj["payload"].toObject();

    if (type == "edit") {
        EditOperation operation = EditOperation::fromJson(payload);
        emit editReceived(operation);
    } else if (type == "cursor") {
        QString userId = payload["userId"].toString();
        QString username = payload["username"].toString();
        int position = payload["position"].toInt();
        emit cursorPositionReceived(userId, username, position);
    } else if (type == "chat") {
        QString userId = payload["userId"].toString();
        QString username = payload["username"].toString();
        QString chatMessage = payload["message"].toString();
        emit chatMessageReceived(userId, username, chatMessage);
    } else if (type == "user_joined") {
        QString userId = payload["userId"].toString();
        QString username = payload["username"].toString();
        emit userConnected(userId, username);
    } else if (type == "user_left") {
        QString userId = payload["userId"].toString();
        emit userDisconnected(userId);
    } else if (type == "content") {
        QString content = payload["content"].toString();
        emit contentReceived(content);
    }
}

void CollaborationClient::onError(QAbstractSocket::SocketError error)
{
    QString errorMessage = "WebSocket error: " + webSocket.errorString();
    emit this->error(errorMessage);
}

void CollaborationClient::sendMessage(const QString& type, const QJsonObject& payload)
{
    // Construct the message
    QJsonObject message;
    message["type"] = type;
    message["payload"] = payload;
    
    // Convert to JSON string
    QJsonDocument doc(message);
    QString jsonString = doc.toJson(QJsonDocument::Compact);
    
    // Send the message over WebSocket
    if (webSocket.isValid()) {
        webSocket.sendTextMessage(jsonString);
    } else {
        emit error("WebSocket not connected");
    }
}