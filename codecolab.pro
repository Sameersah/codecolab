QT       += core gui websockets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = codecolab
TEMPLATE = app

# Set C++17 standard
CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/main.cpp \
    src/MainWindow.cpp \
    src/CodeEditorWidget.cpp \
    src/SyntaxHighlighter.cpp \
    src/LoginDialog.cpp \
    src/Document.cpp \
    src/User.cpp \
    src/CollaborationClient.cpp \
    src/CollaborationManager.cpp \
    src/EditOperation.cpp     # Add this line

HEADERS += \
    include/MainWindow.h \
    include/CodeEditorWidget.h \
    include/SyntaxHighlighter.h \
    include/LoginDialog.h \
    include/Document.h \
    include/User.h \
    include/CollaborationClient.h \
    include/CollaborationManager.h \
    include/EditOperation.h   # Add this line

FORMS += \
    forms/MainWindow.ui \
    forms/LoginDialog.ui

INCLUDEPATH += include

RESOURCES += \
    resources/CodeColab.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target