// EditOperation.h
#ifndef EDIT_OPERATION_H
#define EDIT_OPERATION_H

#include <QString>
#include <QJsonObject>

struct EditOperation {
    QString userId;
    QString documentId;
    int position;
    QString insertion;
    int deletionLength;

    QJsonObject toJson() const;
    static EditOperation fromJson(const QJsonObject& json);
};

#endif // EDIT_OPERATION_H