#ifndef DOCUMENTSTORAGE_H
#define DOCUMENTSTORAGE_H

#include <QString>
#include <QMap>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <memory>
#include "Document.h"

class DocumentStorage {
public:
    static DocumentStorage& getInstance() {
        static DocumentStorage instance;
        return instance;
    }

    bool saveDocument(const std::shared_ptr<Document>& document) {
        QJsonObject docObj;
        docObj["id"] = document->getId();
        docObj["title"] = document->getTitle();
        docObj["content"] = document->getContent();
        docObj["language"] = document->getLanguage();
        docObj["ownerId"] = document->getOwner()->getUserId();
        docObj["ownerName"] = document->getOwner()->getUsername();
        docObj["isPublic"] = document->isPubliclyAccessible();
        
        // Save access control information
        QJsonObject accessObj;
        QMap<QString, Document::AccessLevel> sharedUsers = document->getSharedWith();
        for (auto it = sharedUsers.begin(); it != sharedUsers.end(); ++it) {
            accessObj[it.key()] = static_cast<int>(it.value());
        }
        docObj["access"] = accessObj;

        // Create documents directory if it doesn't exist
        QDir dir("documents");
        if (!dir.exists()) {
            dir.mkpath(".");
        }

        // Save to file
        QFile file("documents/" + document->getId() + ".json");
        if (file.open(QIODevice::WriteOnly)) {
            QJsonDocument doc(docObj);
            file.write(doc.toJson());
            file.close();
            return true;
        }
        return false;
    }

    std::shared_ptr<Document> loadDocument(const QString& documentId) {
        QFile file("documents/" + documentId + ".json");
        if (!file.exists()) {
            return nullptr;
        }

        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            file.close();

            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                
                // Create owner user
                auto owner = std::make_shared<RegisteredUser>(
                    obj["ownerId"].toString(),
                    obj["ownerName"].toString(),
                    obj["ownerId"].toString() + "@example.com"
                );

                // Create document
                auto document = std::make_shared<Document>(
                    obj["id"].toString(),
                    obj["title"].toString(),
                    owner
                );

                // Set content and language
                document->setContent(obj["content"].toString());
                document->setLanguage(obj["language"].toString());

                // Set public access flag
                if (obj.contains("isPublic")) {
                    document->setPublicAccess(obj["isPublic"].toBool());
                }

                // Restore access control
                QJsonObject accessObj = obj["access"].toObject();
                for (auto it = accessObj.begin(); it != accessObj.end(); ++it) {
                    document->shareWith(it.key(), 
                        static_cast<Document::AccessLevel>(it.value().toInt()));
                }

                return document;
            }
        }
        return nullptr;
    }

    bool documentExists(const QString& documentId) {
        return QFile::exists("documents/" + documentId + ".json");
    }

private:
    DocumentStorage() {} // Private constructor for singleton
    ~DocumentStorage() {}
    DocumentStorage(const DocumentStorage&) = delete;
    DocumentStorage& operator=(const DocumentStorage&) = delete;
};

#endif // DOCUMENTSTORAGE_H 