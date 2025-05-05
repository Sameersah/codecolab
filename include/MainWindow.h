// MainWindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QString>
#include <memory>
#include <QElapsedTimer>    // Add for Qt 6
#include <QRandomGenerator> // Add for Qt 6

#include "User.h"
#include "Document.h"
#include "CollaborationManager.h"
#include "CollaborationClient.h"

class CodeEditorWidget;
class LoginDialog;
class QTextEdit;
class QSplitter;
class QPlainTextEdit;
class QLineEdit;
class QListWidget;
class QAction;

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Add document to the editor
    void addDocument(std::shared_ptr<Document> document);

private slots:
    void onLogin();
    void onLogout();
    void onNewDocument();
    void onOpenDocument();
    void onOpenSharedDocument();
    void onSaveDocument();
    void onShareDocument();
    void onTextChanged();
    void onCursorPositionChanged();
    void onSendChatMessage();
    void onUserConnected(const QString& userId, const QString& username);
    void onUserDisconnected(const QString& userId);
    void onChatMessageReceived(const QString& userId, const QString& username, const QString& message);
    void onTogglePublicAccess();
    void showLoginDialog();
    void onUserLoggedIn(std::shared_ptr<User> user);
    void onDocumentOpened(std::shared_ptr<Document> document);
    void onDocumentShared(const QString& userId, bool canEdit);
    void onDocumentAccessChanged(const QString& userId, Document::AccessLevel level);
    void onPublicAccessChanged(bool isPublic);

private:
    void setupUI();
    void setupConnections();
    void setupFileMenu();
    void setupEditMenu();
    void setupViewMenu();
    void updateTitle();
    void updateStatusBar();
    void updateUserList();
    void removeDocument(const QString& documentId);
    void updateDocumentAccess(const QString& documentId, const QString& userId, Document::AccessLevel level);
    bool eventFilter(QObject *obj, QEvent *event) override;

    Ui::MainWindow *ui;
    std::unique_ptr<CodeEditorWidget> codeEditor;
    std::unique_ptr<QTextEdit> chatBox;
    std::unique_ptr<QTextEdit> chatInput;
    std::unique_ptr<QLabel> statusLabel;
    std::unique_ptr<QSplitter> mainSplitter;
    std::unique_ptr<QSplitter> rightSplitter;
    std::unique_ptr<QPlainTextEdit> codeEditorPlain;
    std::unique_ptr<QLineEdit> chatInputLine;
    std::unique_ptr<QListWidget> userList;
    std::unique_ptr<QAction> publicAccessAction;

    // Core objects
    std::shared_ptr<User> currentUser;
    std::shared_ptr<Document> currentDocument;
    std::shared_ptr<CollaborationManager> collaborationManager;
    std::unique_ptr<CollaborationClient> collaborationClient;

    // Track collaboration state
    QMap<QString, QString> connectedUsers; // userId -> username
    bool isCollaborating = false;
};

#endif // MAINWINDOW_H