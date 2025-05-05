#include "UserStorage.h"
#include <QFile>
#include <QJsonDocument>
#include <QDir>

QString UserStorage::getUserFilePath() {
    return QDir::currentPath() + "/users.json";
}

QJsonObject UserStorage::readUsersFromFile() {
    QFile file(getUserFilePath());
    QJsonObject users;

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            users = doc.object();
        }
    }

    return users;
}

bool UserStorage::writeUsersToFile(const QJsonObject& users) {
    QFile file(getUserFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QJsonDocument doc(users);
    file.write(doc.toJson());
    file.close();
    return true;
}
