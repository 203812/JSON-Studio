#include "editor/Syntax.h"

#include <QFileInfo>

namespace syntax {

Language fromFileName(const QString &path)
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    if (suffix == QStringLiteral("yaml") || suffix == QStringLiteral("yml"))
        return Language::Yaml;
    return Language::Json;
}

QString displayName(Language lang)
{
    switch (lang) {
    case Language::Yaml:
        return QStringLiteral("YAML");
    case Language::Json:
        break;
    }
    return QStringLiteral("JSON");
}

QString defaultExtension(Language lang)
{
    switch (lang) {
    case Language::Yaml:
        return QStringLiteral("yaml");
    case Language::Json:
        break;
    }
    return QStringLiteral("json");
}

} // namespace syntax
