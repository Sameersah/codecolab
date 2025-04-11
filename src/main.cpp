#include <QApplication>
#include <QMessageBox>
#include <QStyleFactory>

#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application information
    QApplication::setApplicationName("CodeColab");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("Code Catalysts");
    QApplication::setOrganizationDomain("codecatalysts.edu");
    
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
    
    return app.exec();
}