// EditOperation.cpp
#include "EditOperation.h"

QJsonObject EditOperation::toJson() const {
    QJsonObject json;
    json["userId"] = userId;
    json["documentId"] = documentId;
    json["position"] = position;
    json["insertion"] = insertion;
    json["deletionLength"] = deletionLength;
    return json;
}

EditOperation EditOperation::fromJson(const QJsonObject& json) {
    EditOperation op;
    op.userId = json["userId"].toString();
    op.documentId = json["documentId"].toString();
    op.position = json["position"].toInt();
    op.insertion = json["insertion"].toString();
    op.deletionLength = json["deletionLength"].toInt();
    return op;
}