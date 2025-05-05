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
                                   "Run in server-only mode");
    parser.addOption(serverOption);
    
    parser.process(app);
    
    // Apply fusion style for a modern look
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    
    // Check if server mode is requested
    if (parser.isSet(serverOption)) {
        qDebug() << "Starting CodeColab server...";
        
        // Create and start the server with port 8080
        CollaborationServer server(8080);
        if (!server.start()) {
            qDebug() << "Failed to start collaboration server";
            return 1;
        }
        
        qDebug() << "Server listening on port 8080";
        
        // Keep the application running
        return app.exec();
    }

    // Normal client mode
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
    
    return app.exec();
}