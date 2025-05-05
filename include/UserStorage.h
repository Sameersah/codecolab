#ifndef USERSTORAGE_H
#define USERSTORAGE_H

#include <QString>
#include <QJsonObject>

namespace UserStorage {
QString getUserFilePath();
QJsonObject readUsersFromFile();
bool writeUsersToFile(const QJsonObject& users);
}

#endif // USERSTORAGE_H
