#include <QApplication>
#include <QMessageBox>
#include <QStyleFactory>
#include <QDebug>
#include <QCommandLineParser>

#include "MainWindow.h"
#include "CollaborationServer.h"
#include "CollaborationClient.h"
#include "Document.h"
#include "User.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application information
    QApplication::setApplicationName("CodeColab");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("Code Catalysts");
    QApplication::setOrganizationDomain("codecatalysts.edu");
    
    // Parse command line arguments
    QCommandLineParser parser;
    parser.setApplicationDescription("CodeColab - Real-time Collaborative Code Editor");
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption serverOption(QStringList() << "s" << "server",
                                   "Start as server instance");
    parser.addOption(serverOption);
    
    parser.process(app);
    
    // Apply fusion style for a modern look
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    
    // Create and show the main window
    MainWindow mainWindow;
    mainWindow.show();
    
    // Display welcome message
    QMessageBox::information(&mainWindow, "Welcome to CodeColab",
                           "Welcome to CodeColab!\n\n"
                           "This is a real-time collaborative code editor designed for distributed teams.\n\n"
                           "For this prototype, you can log in with the following credentials:\n"
                           "Username: user1, Password: pass1\n"
                           "Username: user2, Password: pass2\n\n"
                           "Or you can register a new account or login as a guest.");
    
    // Start collaboration server only if --server option is specified
    std::unique_ptr<CollaborationServer> server;
    if (parser.isSet(serverOption)) {
        server = std::make_unique<CollaborationServer>(8080);
        if (!server->start()) {
            qDebug() << "Failed to start collaboration server";
            return 1;
        }
    }
    
    // Create a test user
    auto user = std::make_shared<RegisteredUser>("user1", "user1", "user1@example.com");
    
    // Create a test document
    auto document = std::make_shared<Document>("test_doc_1", "Test Document", user);
    QString documentId = document->getId();
    
    // Add the document to the main window
    mainWindow.addDocument(document);
    
    // Initialize collaboration client
    CollaborationClient client;
    client.setUser(user);
    client.setDocument(document);
    
    // Connect to server and join document
    if (client.connect("ws://localhost:8080")) {
        client.joinDocument(documentId);
    } else {
        qDebug() << "Failed to connect to collaboration server";
    }
    
    return app.exec();
}