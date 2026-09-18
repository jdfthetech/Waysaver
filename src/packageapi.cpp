#include "packageapi.h"

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>

static bool validId(const QString &id)
{
    return QRegularExpression("^[a-z0-9][a-z0-9._-]{2,127}$").match(id).hasMatch();
}

PackageApi::Result PackageApi::create(const Definition &d, const QString &outputPath)
{
    if (d.name.trimmed().isEmpty()) return {false, "name is required"};
    if (!validId(d.id)) return {false, "id must be lowercase and may contain dots, dashes and underscores"};
    if (d.type != "images" && d.type != "executable")
        return {false, "type must be images or executable"};
    if (d.source.trimmed().isEmpty()) return {false, "source is required"};

    QJsonObject saver{{"type", d.type}, {"source", d.source}};
    if (d.type == "images") saver.insert("slideSeconds", qMax(2, d.slideSeconds));
    QJsonObject root{
        {"schema", "org.waysaver.package/v1"},
        {"id", d.id},
        {"name", d.name},
        {"version", qMax(1, d.version)},
        {"saver", saver}
    };

    QSaveFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly)) return {false, file.errorString()};
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!file.commit()) return {false, file.errorString()};
    return {true, {}};
}

PackageApi::Result PackageApi::validate(const QString &path, Definition *definition)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {false, file.errorString()};
    QJsonParseError parse;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parse);
    if (parse.error != QJsonParseError::NoError || !doc.isObject())
        return {false, "invalid JSON: " + parse.errorString()};
    const QJsonObject root = doc.object();
    if (root.value("schema").toString() != "org.waysaver.package/v1")
        return {false, "unsupported package schema"};
    Definition d;
    d.name = root.value("name").toString();
    d.id = root.value("id").toString();
    d.version = root.value("version").toInt(1);
    const QJsonObject saver = root.value("saver").toObject();
    d.type = saver.value("type").toString();
    d.source = saver.value("source").toString();
    d.slideSeconds = saver.value("slideSeconds").toInt(10);
    if (d.name.isEmpty() || !validId(d.id) || (d.type != "images" && d.type != "executable") || d.source.isEmpty())
        return {false, "package has missing or invalid required fields"};
    if (definition) *definition = d;
    return {true, {}};
}

