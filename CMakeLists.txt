cmake_minimum_required(VERSION 3.10)
project(CodeColab VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include Qt
find_package(Qt5 COMPONENTS Core Widgets WebSockets REQUIRED)

# Set automoc for Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Source files
set(SOURCES
        main.cpp
        MainWindow.cpp
        CodeEditorWidget.cpp
        SyntaxHighlighter.cpp
        LoginDialog.cpp
        Document.cpp
        User.cpp
        CollaborationClient.cpp
        CollaborationManager.cpp
)

# Header files
set(HEADERS
        MainWindow.h
        CodeEditorWidget.h
        SyntaxHighlighter.h
        LoginDialog.h
        Document.h
        User.h
        CollaborationClient.h
        CollaborationManager.h
)

# UI files
set(UI_FILES
        MainWindow.ui
        LoginDialog.ui
)

# Add resources
set(RESOURCE_FILES
        resources/CodeColab.qrc
)

# Create executable
add_executable(codecolab ${SOURCES} ${HEADERS} ${UI_FILES} ${RESOURCE_FILES})

# Link Qt libraries
target_link_libraries(codecolab PRIVATE
        Qt5::Core
        Qt5::Widgets
        Qt5::WebSockets
)

# Install
install(TARGETS codecolab
        RUNTIME DESTINATION bin
)

# Create a resource file if it doesn't exist yet
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/CodeColab.qrc")
    file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/resources/CodeColab.qrc" "<!DOCTYPE RCC>
<RCC version=\"1.0\">
    <qresource prefix=\"/icons\">
        <file>icons/new.png</file>
        <file>icons/open.png</file>
        <file>icons/save.png</file>
        <file>icons/cut.png</file>
        <file>icons/copy.png</file>
        <file>icons/paste.png</file>
        <file>icons/user.png</file>
        <file>icons/share.png</file>
        <file>icons/logo.png</file>
    </qresource>
    <qresource prefix=\"/styles\">
        <file>styles/default.qss</file>
    </qresource>
</RCC>
")
endif()

# Create directories for resources
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources/styles")