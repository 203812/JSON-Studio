#pragma once

#include <QString>

// Languages the editor can display. Codegen targets (C#, Java, TypeScript)
// will land here too, which is why the editor is language-driven rather than
// hardcoded to JSON.
enum class Language {
    Json,
    Yaml,
};

namespace syntax {

Language fromFileName(const QString &path);

// Shown in the status bar.
QString displayName(Language lang);

// Used when naming a converted, never-saved buffer.
QString defaultExtension(Language lang);

} // namespace syntax
