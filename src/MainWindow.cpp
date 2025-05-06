#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "CodeEditorWidget.h"
#include "LoginDialog.h"
#include "CollaborationClient.h"
#include "DocumentStorage.h"

#include <QSplitter>
#include <QTextEdit>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QTimer>
#include <QKeyEvent>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QDebug>
#include <QColor>
#include <QScrollBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , collaborationManager(std::make_shared<CollaborationManager>())
    , collaborationClient(std::make_unique<CollaborationClient>())
{
    ui->setupUi(this);
    setupUI();
    setupConnections();

    // Show login dialog on startup
    QTimer::singleShot(500, this, &MainWindow::showLoginDialog);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupConnections()
{
    // Connect code editor signals
    connect(codeEditor.get(), &CodeEditorWidget::editorContentChanged,
            this, &MainWindow::onTextChanged);
    connect(codeEditor.get(), &CodeEditorWidget::cursorPositionChanged,
            this, &MainWindow::onCursorPositionChanged);

    // Connect collaboration client signals
    connect(collaborationClient.get(), &CollaborationClient::connected,
            this, [this]() {
                statusLabel->setText("Connected to server");
                if (currentDocument) {
                    collaborationClient->joinDocument(currentDocument->getId());
                }
            });
            
    connect(collaborationClient.get(), &CollaborationClient::disconnected,
            this, [this]() {
                statusLabel->setText("Disconnected from server");
                connectedUsers.clear();
                updateUserList();
            });
            
    connect(collaborationClient.get(), &CollaborationClient::error,
            this, [this](const QString& error) {
                QMessageBox::warning(this, "Collaboration Error", error);
            });
            
    connect(collaborationClient.get(), &CollaborationClient::editReceived,
            this, [this](const EditOperation& op) {
                if (currentDocument && codeEditor) {
                    codeEditor->applyRemoteEdit(op);
                }
            });
            
    connect(collaborationClient.get(), &CollaborationClient::cursorPositionReceived,
            this, [this](const QString& userId, const QString& username, int position) {
                if (codeEditor) {
                    codeEditor->updateRemoteCursor(userId, username, position);
                }
            });
            
    connect(collaborationClient.get(), &CollaborationClient::userConnected,
            this, &MainWindow::onUserConnected);
            
    connect(collaborationClient.get(), &CollaborationClient::userDisconnected,
            this, &MainWindow::onUserDisconnected);
            
    connect(collaborationClient.get(), &CollaborationClient::contentReceived,
            this, [this](const QString& content) {
                if (currentDocument && codeEditor) {
                    qDebug() << "Received latest content, length:" << content.length();
                    codeEditor->setPlainText(content);
                    currentDocument->setContent(content);
                }
            });

    connect(collaborationClient.get(), &CollaborationClient::chatMessageReceived,
            this, &MainWindow::onChatMessageReceived);

    // Connect chat input
    connect(chatInput.get(), &QTextEdit::textChanged, [this]() {
        // Enable/disable send button based on whether there's text
        QPushButton* sendButton = qobject_cast<QPushButton*>(
            chatInput->parentWidget()->layout()->itemAt(
                chatInput->parentWidget()->layout()->count() - 1)->widget());
        if (sendButton) {
            sendButton->setEnabled(!chatInput->toPlainText().trimmed().isEmpty());
        }
    });

    // Install event filter for chat input to handle Enter key
    chatInput->installEventFilter(this);
}

void MainWindow::setupUI()
{
    // Create central widget and layout
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    setCentralWidget(centralWidget);

    // Create main splitter (editor | right panel)
    mainSplitter = std::make_unique<QSplitter>(Qt::Horizontal);
    mainLayout->addWidget(mainSplitter.get());

    // Create code editor
    codeEditor = std::make_unique<CodeEditorWidget>();
    codeEditor->setCollaborationManager(collaborationManager);
    mainSplitter->addWidget(codeEditor.get());

    // Create right panel splitter (user list | chat)
    rightSplitter = std::make_unique<QSplitter>(Qt::Vertical);
    mainSplitter->addWidget(rightSplitter.get());

    // Create user list panel
    QWidget* userListPanel = new QWidget(rightSplitter.get());
    QVBoxLayout* userListLayout = new QVBoxLayout(userListPanel);
    userListLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* collaboratorsLabel = new QLabel("Collaborators:");
    userListLayout->addWidget(collaboratorsLabel);

    QListWidget* userList = new QListWidget();
    userListLayout->addWidget(userList);

    rightSplitter->addWidget(userListPanel);

    // Create chat panel
    QWidget* chatPanel = new QWidget(rightSplitter.get());
    QVBoxLayout* chatLayout = new QVBoxLayout(chatPanel);
    chatLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* chatLabel = new QLabel("Chat:");
    chatLayout->addWidget(chatLabel);

    chatBox = std::make_unique<QTextEdit>();
    chatBox->setReadOnly(true);
    chatLayout->addWidget(chatBox.get());

    chatInput = std::make_unique<QTextEdit>();
    chatInput->setMaximumHeight(60);
    chatLayout->addWidget(chatInput.get());

    QPushButton* sendButton = new QPushButton("Send");
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::onSendChatMessage);
    chatLayout->addWidget(sendButton);

    rightSplitter->addWidget(chatPanel);

    // Set up splitter proportions
    mainSplitter->setSizes({600, 200});
    rightSplitter->setSizes({200, 400});

    // Set up status bar
    statusLabel = std::make_unique<QLabel>("Not logged in");
    statusBar()->addWidget(statusLabel.get());

    // Setup menus with newer Qt 6 syntax
    setupFileMenu();

    QMenu* editMenu = menuBar()->addMenu("&Edit");
    QAction* cutAction = new QAction("Cut", this);
    cutAction->setShortcut(QKeySequence::Cut);
    connect(cutAction, &QAction::triggered, codeEditor.get(), &QPlainTextEdit::cut);
    editMenu->addAction(cutAction);

    QAction* copyAction = new QAction("Copy", this);
    copyAction->setShortcut(QKeySequence::Copy);
    connect(copyAction, &QAction::triggered, codeEditor.get(), &QPlainTextEdit::copy);
    editMenu->addAction(copyAction);

    QAction* pasteAction = new QAction("Paste", this);
    pasteAction->setShortcut(QKeySequence::Paste);
    connect(pasteAction, &QAction::triggered, codeEditor.get(), &QPlainTextEdit::paste);
    editMenu->addAction(pasteAction);

    QMenu* viewMenu = menuBar()->addMenu("&View");
    QAction* zoomInAction = new QAction("Zoom In", this);
    zoomInAction->setShortcut(QKeySequence::ZoomIn);
    connect(zoomInAction, &QAction::triggered, codeEditor.get(), &QPlainTextEdit::zoomIn);
    viewMenu->addAction(zoomInAction);

    QAction* zoomOutAction = new QAction("Zoom Out", this);
    zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    connect(zoomOutAction, &QAction::triggered, codeEditor.get(), &QPlainTextEdit::zoomOut);
    viewMenu->addAction(zoomOutAction);

    QMenu* collaborationMenu = menuBar()->addMenu("&Collaboration");
    QAction* shareAction = new QAction("Share Document", this);
    connect(shareAction, &QAction::triggered, this, &MainWindow::onShareDocument);
    collaborationMenu->addAction(shareAction);

    QMenu* userMenu = menuBar()->addMenu("&User");
    QAction* loginAction = new QAction("Login", this);
    connect(loginAction, &QAction::triggered, this, &MainWindow::onLogin);
    userMenu->addAction(loginAction);

    QAction* logoutAction = new QAction("Logout", this);
    connect(logoutAction, &QAction::triggered, this, &MainWindow::onLogout);
    userMenu->addAction(logoutAction);

    // Set window properties
    resize(1024, 768);
    setWindowTitle("CodeColab - Collaborative Code Editor");
}

void MainWindow::setupFileMenu()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    
    QAction *newAct = new QAction(tr("&New"), this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip(tr("Create a new document"));
    connect(newAct, &QAction::triggered, this, &MainWindow::onNewDocument);
    fileMenu->addAction(newAct);
    
    QAction *openAct = new QAction(tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing document"));
    connect(openAct, &QAction::triggered, this, &MainWindow::onOpenDocument);
    fileMenu->addAction(openAct);
    
    QAction *shareAct = new QAction(tr("&Share..."), this);
    shareAct->setStatusTip(tr("Share this document with other users"));
    connect(shareAct, &QAction::triggered, this, &MainWindow::onShareDocument);
    fileMenu->addAction(shareAct);
    
    QAction *openSharedAct = new QAction(tr("Open &Shared..."), this);
    openSharedAct->setStatusTip(tr("Open a document shared with you"));
    connect(openSharedAct, &QAction::triggered, this, &MainWindow::onOpenSharedDocument);
    fileMenu->addAction(openSharedAct);

    // Add separator
    fileMenu->addSeparator();

    // Add public access toggle
    QAction *togglePublicAct = new QAction(tr("Make &Public"), this);
    togglePublicAct->setCheckable(true);
    togglePublicAct->setStatusTip(tr("Allow anyone to view this document in read-only mode"));
    connect(togglePublicAct, &QAction::triggered, this, &MainWindow::onTogglePublicAccess);
    fileMenu->addAction(togglePublicAct);
    publicAccessAction.reset(togglePublicAct);  // Store the action for later use
}

void MainWindow::showLoginDialog()
{
    LoginDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        currentUser = dialog.getUser();
        if (currentUser) {
            qDebug() << "User logged in:" << currentUser->getUsername() 
                     << "(ID:" << currentUser->getUserId() << ")";
            
            // Set the user in the collaboration client
            collaborationClient->setUser(currentUser);
            
            updateStatusBar();
            updateTitle();
            
            // Show document selection dialog
            QDialog docDialog(this);
            docDialog.setWindowTitle("Document Selection");
            QVBoxLayout* layout = new QVBoxLayout(&docDialog);

            // Add buttons for different options
            QPushButton* newDocButton = new QPushButton("Create New Document");
            QPushButton* openSharedButton = new QPushButton("Open Shared Document");
            QPushButton* cancelButton = new QPushButton("Cancel");

            layout->addWidget(newDocButton);
            layout->addWidget(openSharedButton);
            layout->addWidget(cancelButton);

            // Connect buttons
            connect(newDocButton, &QPushButton::clicked, [&]() {
                docDialog.accept();
                onNewDocument();
            });
            connect(openSharedButton, &QPushButton::clicked, [&]() {
                docDialog.accept();
                onOpenSharedDocument();
            });
            connect(cancelButton, &QPushButton::clicked, &docDialog, &QDialog::reject);

            if (docDialog.exec() == QDialog::Rejected) {
                qDebug() << "User cancelled document selection";
                close();
                return;
            }

            // Try to connect to the collaboration server
            if (collaborationClient) {
                // Show connecting status
                statusLabel->setText("Connecting to server...");
                
                // Try to connect
                if (!collaborationClient->connect("ws://localhost:8080")) {
                    QMessageBox::warning(this, "Connection Error",
                        "Failed to connect to the collaboration server.\n"
                        "Please make sure the server is running (start with --server flag).\n"
                        "You can continue working offline.");
                    statusLabel->setText("Working offline");
                }
            }
        } else {
            QMessageBox::critical(this, "Login Failed", "Could not authenticate user.");
        }
    } else {
        // User cancelled login, close application if not already logged in
        if (!currentUser) {
            close();
        }
    }
}

void MainWindow::updateTitle()
{
    QString title = "CodeColab";
    if (currentDocument) {
        title += " - " + currentDocument->getTitle();
    }
    if (currentUser) {
        title += " - " + currentUser->getUsername();
    }
    setWindowTitle(title);
}

void MainWindow::updateStatusBar()
{
    if (currentUser) {
        statusLabel->setText("Logged in as: " + currentUser->getUsername());
    } else {
        statusLabel->setText("Not logged in");
    }
}

void MainWindow::updateUserList()
{
    QListWidget* userList = findChild<QListWidget*>();
    if (!userList) return;

    userList->clear();

    if (currentUser) {
        QListWidgetItem* ownerItem = new QListWidgetItem(currentUser->getUsername() + " (You)");
        ownerItem->setForeground(currentUser->getCursorColor());
        userList->addItem(ownerItem);
    }

    for (const auto& [userId, username] : connectedUsers.toStdMap()) {
        QListWidgetItem* userItem = new QListWidgetItem(username);
        userList->addItem(userItem);
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == chatInput.get() && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Return && !(keyEvent->modifiers() & Qt::ShiftModifier)) {
            onSendChatMessage();
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

// Slot implementations
void MainWindow::onLogin()
{
    // If already logged in, ask if user wants to logout first
    if (currentUser) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Already Logged In",
                                     "You are already logged in. Do you want to logout first?",
                                     QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            onLogout();
        } else {
            return;
        }
    }

    showLoginDialog();
}

void MainWindow::onLogout()
{
    if (!currentUser) return;

    // Check if we need to save the document
    if (currentDocument) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Unsaved Changes",
                                     "Do you want to save changes before logging out?",
                                     QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);
        if (reply == QMessageBox::Cancel) {
            return;
        } else if (reply == QMessageBox::Yes) {
            onSaveDocument();
        }
    }

    // Clear current state
    currentUser = nullptr;
    currentDocument = nullptr;
    connectedUsers.clear();

    // Clear UI
    codeEditor->setDocument(nullptr);
    codeEditor->setPlainText("");
    chatBox->clear();
    chatInput->clear();

    // Update UI
    updateStatusBar();
    updateTitle();
    updateUserList();

    // Show login dialog
    QTimer::singleShot(100, this, &MainWindow::showLoginDialog);
}

void MainWindow::onNewDocument()
{
    if (!currentUser) {
        QMessageBox::warning(this, "Not Logged In", "You must be logged in to create a document.");
        return;
    }

    bool ok;
    QString title = QInputDialog::getText(this, "New Document",
                                         "Document title:", QLineEdit::Normal,
                                         "Untitled.cpp", &ok);
    if (ok && !title.isEmpty()) {
        // Create a new document
        QString documentId = "doc_" + QString::number(QDateTime::currentSecsSinceEpoch());
        currentDocument = std::make_shared<Document>(documentId, title, currentUser);
        
        qDebug() << "Created new document:"
                 << "\n  Title:" << title
                 << "\n  ID:" << documentId
                 << "\n  Owner:" << currentUser->getUsername()
                 << "\n  Owner ID:" << currentUser->getUserId();

        // Set initial content
        QString initialContent = "// " + title + "\n// Created by " + currentUser->getUsername() + "\n\n";
        if (title.endsWith(".cpp") || title.endsWith(".h")) {
            initialContent += "#include <iostream>\n\nint main() {\n    std::cout << \"Hello, World!\" << std::endl;\n    return 0;\n}\n";
            currentDocument->setLanguage("C++");
        } else if (title.endsWith(".py")) {
            initialContent += "def main():\n    print(\"Hello, World!\")\n\nif __name__ == \"__main__\":\n    main()\n";
            currentDocument->setLanguage("Python");
        } else if (title.endsWith(".js")) {
            initialContent += "console.log(\"Hello, World!\");\n";
            currentDocument->setLanguage("JavaScript");
        } else if (title.endsWith(".java")) {
            initialContent += "public class " + title.left(title.lastIndexOf('.')) + " {\n    public static void main(String[] args) {\n        System.out.println(\"Hello, World!\");\n    }\n}\n";
            currentDocument->setLanguage("Java");
        }

        currentDocument->setContent(initialContent);
        qDebug() << "Set initial content and language:" << currentDocument->getLanguage();

        // Save document to storage
        if (!DocumentStorage::getInstance().saveDocument(currentDocument)) {
            QMessageBox::warning(this, "Save Error", "Failed to save document to storage.");
            return;
        }

        // Set up editor
        codeEditor->setDocument(currentDocument);
        codeEditor->setPlainText(initialContent);
        codeEditor->setLanguage(currentDocument->getLanguage());

        // Update UI
        updateTitle();
        updateUserList();

        // Clear chat
        chatBox->clear();
        chatInput->clear();

        QMessageBox::information(this, "Document Created", "New document '" + title + "' has been created.");
    }
}

void MainWindow::onOpenDocument()
{
    if (!currentUser) {
        QMessageBox::warning(this, "Not Logged In", "You must be logged in to open a document.");
        return;
    }

    QString fileName = QFileDialog::getOpenFileName(this, "Open Document", "",
                                                  "C++ Files (*.cpp *.h);;Python Files (*.py);;JavaScript Files (*.js);;Java Files (*.java);;All Files (*)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString content = in.readAll();
            file.close();

            // Create document
            QFileInfo fileInfo(fileName);
            currentDocument = std::make_shared<Document>(
                "doc_" + QString::number(QDateTime::currentSecsSinceEpoch()),
                fileInfo.fileName(),
                currentUser);

            // Set content
            currentDocument->setContent(content);

            // Determine language based on file extension
            QString extension = fileInfo.suffix().toLower();
            if (extension == "cpp" || extension == "h") {
                currentDocument->setLanguage("C++");
            } else if (extension == "py") {
                currentDocument->setLanguage("Python");
            } else if (extension == "js") {
                currentDocument->setLanguage("JavaScript");
            } else if (extension == "java") {
                currentDocument->setLanguage("Java");
            } else {
                currentDocument->setLanguage("Plain");
            }

            // Set up editor
            codeEditor->setDocument(currentDocument);
            codeEditor->setPlainText(content);
            codeEditor->setLanguage(currentDocument->getLanguage());

            // Update UI
            updateTitle();
            updateUserList();

            // Clear chat
            chatBox->clear();
            chatInput->clear();
        } else {
            QMessageBox::critical(this, "Error", "Could not open file: " + fileName);
        }
    }
}

void MainWindow::onSaveDocument()
{
    if (!currentUser || !currentDocument) {
        QMessageBox::warning(this, "No Document", "No document is currently open.");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, "Save Document",
                                                  currentDocument->getTitle(),
                                                  "C++ Files (*.cpp *.h);;Python Files (*.py);;JavaScript Files (*.js);;Java Files (*.java);;All Files (*)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << codeEditor->toPlainText();
            file.close();

            QMessageBox::information(this, "Saved", "Document saved successfully.");
        } else {
            QMessageBox::critical(this, "Error", "Could not save file: " + fileName);
        }
    }
}

void MainWindow::onShareDocument()
{
    if (!currentUser || !currentDocument) {
        QMessageBox::warning(this, "No Document", "No document is currently open.");
        return;
    }

    // Create dialog for sharing
    QDialog dialog(this);
    dialog.setWindowTitle("Share Document");
    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    // Add document ID display
    QLabel* idLabel = new QLabel("Document ID: " + currentDocument->getId());
    layout->addWidget(idLabel);

    // Add user input
    QLineEdit* userInput = new QLineEdit();
    userInput->setPlaceholderText("Enter user ID or email");
    layout->addWidget(userInput);

    // Add access level selection
    QComboBox* accessLevel = new QComboBox();
    accessLevel->addItem("Read Only", static_cast<int>(Document::AccessLevel::ReadOnly));
    accessLevel->addItem("Edit", static_cast<int>(Document::AccessLevel::Edit));
    layout->addWidget(accessLevel);

    // Add buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        QString userId = userInput->text().trimmed();
        Document::AccessLevel level = static_cast<Document::AccessLevel>(
            accessLevel->currentData().toInt());

        qDebug() << "Attempting to share document" << currentDocument->getId() 
                 << "with user" << userId << "at level" 
                 << (level == Document::AccessLevel::Edit ? "edit" : "read-only");

        if (currentDocument->shareWith(userId, level)) {
            // Save the document after sharing
            if (DocumentStorage::getInstance().saveDocument(currentDocument)) {
                qDebug() << "Successfully shared document with user";
                QMessageBox::information(this, "Document Shared",
                    "Document shared successfully with " + userId);
            } else {
                qDebug() << "Failed to save document after sharing";
                QMessageBox::warning(this, "Share Error",
                    "Document was shared but failed to save changes.");
            }
        } else {
            qDebug() << "Failed to share document with user";
            QMessageBox::warning(this, "Share Failed",
                "Failed to share document with " + userId);
        }
    }
}

void MainWindow::onOpenSharedDocument()
{
    if (!currentUser) {
        QMessageBox::warning(this, "Not Logged In", "You must be logged in to open shared documents.");
        return;
    }

    bool ok;
    QString documentId = QInputDialog::getText(this, "Open Shared Document",
        "Enter Document ID:", QLineEdit::Normal, "", &ok);
    
    if (ok && !documentId.isEmpty()) {
        qDebug() << "Attempting to open shared document:"
                 << "\n  Document ID:" << documentId
                 << "\n  User:" << currentUser->getUsername()
                 << "\n  User ID:" << currentUser->getUserId();
        
        // Check if we already have this document open
        if (currentDocument && currentDocument->getId() == documentId) {
            qDebug() << "Document already open";
            QMessageBox::information(this, "Document Already Open",
                "This document is already open.");
            return;
        }
        
        // Try to load the document from storage
        std::shared_ptr<Document> doc = DocumentStorage::getInstance().loadDocument(documentId);
        
        if (!doc) {
            qDebug() << "Document not found in storage";
            QMessageBox::warning(this, "Document Not Found",
                "Could not find the document. Please make sure the document ID is correct.");
            return;
        }
        
        qDebug() << "Found document in storage:"
                 << "\n  Title:" << doc->getTitle()
                 << "\n  Owner:" << (doc->getOwner() ? doc->getOwner()->getUsername() : "Unknown")
                 << "\n  Content length:" << doc->getContent().length()
                 << "\n  Content:" << doc->getContent();
        
        // Check access level
        Document::AccessLevel accessLevel = doc->getAccessLevel(currentUser->getUserId());
        qDebug() << "Access check:"
                 << "\n  User:" << currentUser->getUsername()
                 << "\n  Access Level:" << (accessLevel == Document::AccessLevel::Edit ? "Edit" :
                                          accessLevel == Document::AccessLevel::ReadOnly ? "ReadOnly" : "None");
        
        if (accessLevel == Document::AccessLevel::None) {
            qDebug() << "Access denied for user" << currentUser->getUsername();
            QMessageBox::warning(this, "Access Denied",
                "You don't have access to this document.\n"
                "Please ask the document owner to share it with you.");
            return;
        }
        
        // Set read-only if user only has read access
        bool isReadOnly = (accessLevel == Document::AccessLevel::ReadOnly);
        qDebug() << "Setting document read-only:" << isReadOnly;
        
        // First set up the editor with the document
        codeEditor->setDocument(doc);
        codeEditor->setReadOnly(isReadOnly);
        codeEditor->setLanguage(doc->getLanguage());
        
        // Get the content from the document
        QString content = doc->getContent();
        qDebug() << "Setting editor content, length:" << content.length();
        codeEditor->setPlainText(content);
        
        // Then set up the collaboration client
        if (collaborationClient) {
            collaborationClient->setDocument(doc);
            if (collaborationClient->isConnected()) {
                collaborationClient->joinDocument(documentId);
                // Request latest content from collaboration server
                collaborationClient->requestLatestContent(documentId);
            }
        }
        
        // Update UI
        currentDocument = doc;
        updateTitle();
        updateUserList();
        
        // Clear chat
        chatBox->clear();
        chatInput->clear();
        
        // Show access level in status bar
        QString accessText = isReadOnly ? "Read-only access" : "Edit access";
        statusLabel->setText(accessText);
        
        qDebug() << "Successfully opened shared document:"
                 << "\n  Document ID:" << documentId
                 << "\n  Access Level:" << accessText
                 << "\n  Owner:" << (doc->getOwner() ? doc->getOwner()->getUsername() : "Unknown")
                 << "\n  Content length:" << content.length();
    }
}

void MainWindow::onUserConnected(const QString& userId, const QString& username)
{
    if (!connectedUsers.contains(userId)) {
        connectedUsers[userId] = username;
        updateUserList();

        // Add message to chat
        QString message = "<b>" + username + " has joined the document.</b>";
        chatBox->append(message);
    }
}

void MainWindow::onUserDisconnected(const QString& userId)
{
    if (connectedUsers.contains(userId)) {
        QString username = connectedUsers[userId];
        connectedUsers.remove(userId);
        updateUserList();

        // Add message to chat
        QString message = "<b>" + username + " has left the document.</b>";
        chatBox->append(message);
    }
}

void MainWindow::onCursorPositionChanged()
{
    if (!currentUser || !currentDocument) return;

    // Get current position
    int position = codeEditor->textCursor().position();

    // Update status bar with position information
    int line = codeEditor->textCursor().blockNumber() + 1;
    int column = codeEditor->textCursor().columnNumber() + 1;
    statusLabel->setText(QString("Line: %1 Column: %2 | %3").arg(line).arg(column).arg(currentUser->getUsername()));

    // Send cursor position to other users
    if (collaborationClient && collaborationClient->isConnected()) {
        qDebug() << "Sending cursor position:" << position << "for user" << currentUser->getUsername();
        collaborationClient->sendCursorPosition(position);
    }
}

void MainWindow::onTextChanged()
{
    if (!currentUser || !currentDocument) return;

    // Update document content
    QString newContent = codeEditor->toPlainText();
    if (newContent != currentDocument->getContent()) {
        qDebug() << "Document content changed:"
                 << "\n  Document ID:" << currentDocument->getId()
                 << "\n  User:" << currentUser->getUsername()
                 << "\n  Content length:" << newContent.length();
        
        currentDocument->setContent(newContent);

        // Save changes to storage
        if (!DocumentStorage::getInstance().saveDocument(currentDocument)) {
            qDebug() << "Failed to save document changes";
            QMessageBox::warning(this, "Save Error", "Failed to save document changes.");
        }

        // Send changes through collaboration client
        if (collaborationClient && collaborationClient->isConnected()) {
            EditOperation op;
            op.userId = currentUser->getUserId();
            op.documentId = currentDocument->getId();
            op.position = 0;
            op.insertion = newContent;
            op.deletionLength = currentDocument->getContent().length();
            collaborationClient->sendEdit(op);
        }

        // Mark the document as modified
        setWindowModified(true);
    }
}

void MainWindow::onSendChatMessage()
{
    if (!currentUser || !currentDocument) return;

    QString message = chatInput->toPlainText().trimmed();
    if (!message.isEmpty()) {
        // Clear input
        chatInput->clear();

        // Send message through collaboration client
        if (collaborationClient && collaborationClient->isConnected()) {
            collaborationClient->sendChatMessage(message);
        } else {
            // If not connected, just display locally
            onChatMessageReceived(currentUser->getUserId(), currentUser->getUsername(), message);
        }
    }
}

void MainWindow::onChatMessageReceived(const QString& userId, const QString& username, const QString& message)
{
    // Skip if this is our own message and we're connected (it will come back through the server)
    if (currentUser && userId == currentUser->getUserId() && 
        collaborationClient && collaborationClient->isConnected()) {
        return;
    }

    QString formattedMessage;
    if (currentUser && userId == currentUser->getUserId()) {
        formattedMessage = "<span style='color: blue'><b>You:</b> " + message + "</span>";
    } else {
        formattedMessage = "<span style='color: green'><b>" + username + ":</b> " + message + "</span>";
    }
    chatBox->append(formattedMessage);
    
    // Scroll to bottom to show new message
    QScrollBar* scrollBar = chatBox->verticalScrollBar();
    if (scrollBar) {
        scrollBar->setValue(scrollBar->maximum());
    }
}

void MainWindow::onTogglePublicAccess()
{
    if (!currentDocument || !currentUser) {
        QMessageBox::warning(this, "No Document", "Please open a document first.");
        return;
    }

    // Only document owner can toggle public access
    if (currentDocument->getOwner()->getUserId() != currentUser->getUserId()) {
        QMessageBox::warning(this, "Access Denied", "Only the document owner can change public access settings.");
        return;
    }

    bool newState = publicAccessAction->isChecked();
    currentDocument->setPublicAccess(newState);

    // Save the document to persist the change
    if (!DocumentStorage::getInstance().saveDocument(currentDocument)) {
        QMessageBox::warning(this, "Save Error", "Failed to save document settings.");
        return;
    }

    // Update status
    statusBar()->showMessage(newState ? 
        "Document is now publicly accessible" : 
        "Document is now private", 3000);
}

void MainWindow::addDocument(std::shared_ptr<Document> document)
{
    if (!document) return;
    
    currentDocument = document;
    codeEditor->setDocument(document);
    codeEditor->setPlainText(document->getContent());
    
    // Set up collaboration
    if (collaborationClient) {
        // Set the document in the collaboration client
        collaborationClient->setDocument(document);
        
        // If connected, join the document
        if (collaborationClient->isConnected()) {
            collaborationClient->joinDocument(document->getId());
        } else {
            // Try to connect to the collaboration server
            collaborationClient->connect("ws://localhost:8080");
        }
    }
    
    updateTitle();
    updateStatusBar();
}

void MainWindow::onUserLoggedIn(std::shared_ptr<User> user)
{
    if (user) {
        currentUser = user;
        updateStatusBar();
        updateTitle();
    }
}

void MainWindow::onDocumentOpened(std::shared_ptr<Document> document)
{
    if (document) {
        currentDocument = document;
        codeEditor->setDocument(document);
        codeEditor->setPlainText(document->getContent());
        codeEditor->setLanguage(document->getLanguage());
        updateTitle();
        updateUserList();
    }
}

void MainWindow::onDocumentShared(const QString& userId, bool canEdit)
{
    if (currentDocument) {
        Document::AccessLevel level = canEdit ? Document::AccessLevel::Edit : Document::AccessLevel::ReadOnly;
        currentDocument->shareWith(userId, level);
        updateUserList();
    }
}

void MainWindow::onPublicAccessChanged(bool isPublic)
{
    if (currentDocument) {
        currentDocument->setPublicAccess(isPublic);
        updateStatusBar();
    }
}

void MainWindow::onDocumentAccessChanged(const QString& userId, Document::AccessLevel level)
{
    if (currentDocument) {
        currentDocument->shareWith(userId, level);
        updateUserList();
    }
}