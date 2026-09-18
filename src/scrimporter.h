#pragma once

#include <QString>

class ScrImporter {
public:
    enum class Architecture { Unknown, Ne16, Pe32, Pe64 };

    struct Inspection {
        bool valid = false;
        Architecture architecture = Architecture::Unknown;
        QString description;
        QString error;
    };

    static Inspection inspect(const QString &path);
    static QString importFile(const QString &source, QString *error = nullptr);
    static QString architectureName(Architecture value);
};

