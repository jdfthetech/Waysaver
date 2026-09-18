#pragma once

#include <QStringList>

class MediaGuard {
public:
    struct Result {
        bool playing = false;
        QStringList players;
    };

    static Result query();
};

