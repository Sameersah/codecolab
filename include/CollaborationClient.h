#ifndef COLLABORATIONCLIENT_H
#define COLLABORATIONCLIENT_H

#include <QObject>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QMap>
#include <memory>

class User;
class Document;

// Edit operation structure (similar to CollaborationManager::EditOperation)
struct EditOperation {
    QString userId;
    QString documentId;
    int position;
    QString insertion;
    int deletionLength;
    
    QJsonObject toJson() const;
    static EditOperation fromJson(const QJsonObject& json);
};

class CollaborationClient : public QObject
{
    Q_OBJECT

public:
    explicit CollaborationClient(QObject *parent = nullptr);
    ~CollaborationClient();

    bool connect(const QString& serverUrl);
    void disconnect();
    bool isConnected() const;
    
    void setUser(std::shared_ptr<User> user);
    void setDocument(std::shared_ptr<Document> document);
    
    void joinDocument(const QString& documentId);
    void leaveDocument();
    
    void sendEdit(const EditOperation& operation);
    void sendCursorPosition(int position);
    void sendChatMessage(const QString& message);

signals:
    void connected();
    void disconnected();
    void error(const QString& errorMessage);
    
    void documentJoined(const QString& documentId);
    void documentLeft();
    
    void editReceived(const EditOperation& operation);
    void cursorPositionReceived(const QString& userId, const QString& username, int position);
    void chatMessageReceived(const QString& userId, const QString& username, const QString& message);
    
    void userConnected(const QString& userId, const QString& username);
    void userDisconnected(const QString& userId);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString& message);
    void onError(QAbstractSocket::SocketError error);

private:
    void sendMessage(const QString& type, const QJsonObject& payload);
    void handleMessage(const QJsonObject& message);
    
    QWebSocket webSocket;
    QString serverUrl;
    std::shared_ptr<User> currentUser;
    std::shared_ptr<Document> currentDocument;
    
    bool isDocumentJoined;
    QMap<QString, QString> connectedUsers; // userId -> username
};

#endif // COLLABORATIONCLIENT_H