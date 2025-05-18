# CodeColab - Real-time Collaborative Code Editor

## Overview
CodeColab is a powerful real-time collaborative code editor that enables multiple developers to work on the same code document simultaneously. Built with Qt 6, it provides a seamless coding experience with features like real-time synchronization, syntax highlighting, and integrated chat functionality.

## Features

### Real-time Collaboration
- **Live Synchronization**: Multiple users can edit the same document in real-time
- **Cursor Tracking**: See other users' cursor positions with distinct colors
- **User Presence**: Real-time display of connected collaborators
- **Conflict Resolution**: Smart handling of simultaneous edits

### Code Editor
- **Syntax Highlighting**: Support for multiple programming languages
  - C++
  - Python
  - JavaScript
  - Java
- **Line Numbers**: Automatic line numbering
- **Auto-indentation**: Smart indentation based on code structure
- **Tab Handling**: Configurable tab width (4 spaces)
- **Monospace Font**: Courier New for better code readability

### Document Management
- **Document Creation**: Create new documents with language-specific templates
- **Document Sharing**: Share documents with other users
- **Access Control**: Different access levels (Read-only, Edit)
- **Version History**: Track document changes

### User Interface
- **Split View**: Main editor with collapsible side panels
- **User List**: Real-time display of connected collaborators
- **Chat System**: Integrated chat for communication
- **Status Bar**: Connection status and cursor position

## Technical Details

### Architecture
- **Client-Server Model**: WebSocket-based real-time communication
- **Qt 6 Framework**: Modern C++ GUI framework
- **Object-Oriented Design**: Clean separation of concerns
- **Signal-Slot Mechanism**: Loose coupling between components

### Key Components
1. **MainWindow**: Main application window and UI management
2. **CodeEditorWidget**: Custom code editor implementation
3. **CollaborationClient**: Real-time communication handler
4. **Document**: Document data model
5. **User**: User data model

## Getting Started

### Prerequisites
- Qt 6.0 or higher
- C++17 compatible compiler
- CMake 3.16 or higher

### Building from Source
```bash

# Install QT/qmake/related libraries
brew install qt6
brew install qt6-websockets
/opt/homebrew/opt/qt6/bin/qmake -v

#Note
configure in preferences->qt versions->6.9.0 and tick websockets in maintenance tool

# Clone the repository
git clone https://github.com/yourusername/codecolab.git
cd codecolab

# Create build directory
mkdir build

# Generate Makefile using qmake and build the project
make clean && qmake && make

# Run the application server (websocket server)
./codecolab.app/Contents/MacOS/codecolab --server

# Start multiple instances of code editor
./codecolab.app/Contents/MacOS/codecolab

./codecolab.app/Contents/MacOS/codecolab

# Next steps
1. Register/Login as a Registered User
2. Create new Document
3. Fetch the document id from Collaboration --> Share Document option from menu bar.
4. Register/Login as different user
5. Open shared document using the document id fetched from previous step.
6. Edit and collaborate on the shared document.
```

## Usage

### Creating a New Document
1. Click "File" → "New"
2. Enter document title
3. Select programming language
4. Start coding!

### Sharing a Document
1. Open the document you want to share
2. Click "Collaboration" → "Share Document"
3. Enter user ID
4. Select access level (Read-only/Edit)

### Collaborating
1. Connect to the server
2. Open a shared document
3. See other users' cursors in real-time
4. Use the chat system for communication

## Design Principles

### Code Organization
- **Cohesive Classes**: Each class has a single, well-defined responsibility
- **Loose Coupling**: Components communicate through well-defined interfaces
- **Interface-Based Design**: Dependencies on interfaces, not implementations
- **Delegation**: Clear separation of responsibilities

### Architecture Patterns
- **MVC Pattern**: Clear separation of Model, View, and Controller
- **Observer Pattern**: Real-time updates through signal-slot mechanism
- **Singleton Pattern**: Document storage management
- **Factory Pattern**: Document and syntax highlighter creation