#include "scrimporter.h"

#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

ScrImporter::Inspection ScrImporter::inspect(const QString &path)
{
    Inspection out;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        out.error = "The file could not be opened.";
        return out;
    }
    if (file.size() < 64 || file.read(2) != "MZ") {
        out.error = "This is not a DOS or Windows executable.";
        return out;
    }

    file.seek(0x3c);
    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);
    quint32 headerOffset = 0;
    stream >> headerOffset;
    if (headerOffset + 6 > quint64(file.size()) || !file.seek(headerOffset)) {
        out.error = "The executable header is invalid.";
        return out;
    }

    const QByteArray signature = file.read(4);
    if (signature.left(2) == "NE") {
        out.valid = true;
        out.architecture = Architecture::Ne16;
        out.description = "16 bit Windows NE screensaver";
        return out;
    }
    if (signature != QByteArray("PE\0\0", 4)) {
        out.error = "The executable is neither a 16 bit NE nor a PE program.";
        return out;
    }

    QDataStream pe(&file);
    pe.setByteOrder(QDataStream::LittleEndian);
    quint16 machine = 0;
    pe >> machine;
    out.valid = true;
    if (machine == 0x014c) {
        out.architecture = Architecture::Pe32;
        out.description = "32 bit Windows PE screensaver";
    } else if (machine == 0x8664) {
        out.architecture = Architecture::Pe64;
        out.description = "64 bit Windows PE screensaver";
    } else {
        out.architecture = Architecture::Unknown;
        out.description = "Windows PE screensaver with an unknown CPU architecture";
    }
    return out;
}

QString ScrImporter::importFile(const QString &source, QString *error)
{
    const auto inspection = inspect(source);
    if (!inspection.valid) {
        if (error) *error = inspection.error;
        return {};
    }
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(base);
    if (!dir.mkpath("screensavers")) {
        if (error) *error = "The local screensaver directory could not be created.";
        return {};
    }
    QString name = QFileInfo(source).fileName();
    QString target = dir.filePath("screensavers/" + name);
    for (int n = 2; QFileInfo::exists(target); ++n) {
        target = dir.filePath(QString("screensavers/%1-%2.scr")
                                  .arg(QFileInfo(source).completeBaseName())
                                  .arg(n));
    }
    if (!QFile::copy(source, target)) {
        if (error) *error = "The screensaver could not be copied into local storage.";
        return {};
    }
    QFile::setPermissions(target, QFile::ReadOwner | QFile::WriteOwner);
    return target;
}

QString ScrImporter::architectureName(Architecture value)
{
    switch (value) {
    case Architecture::Ne16: return "16 bit";
    case Architecture::Pe32: return "32 bit";
    case Architecture::Pe64: return "64 bit";
    default: return "unknown";
    }
}

