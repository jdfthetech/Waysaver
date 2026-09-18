#pragma once

#include <QString>

class PackageApi {
public:
    struct Definition {
        QString name;
        QString id;
        QString type;
        QString source;
        int version = 1;
        int slideSeconds = 10;
    };

    struct Result {
        bool ok = false;
        QString error;
    };

    static Result create(const Definition &definition, const QString &outputPath);
    static Result validate(const QString &path, Definition *definition = nullptr);
};

