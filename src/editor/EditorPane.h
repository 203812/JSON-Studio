#pragma once

#include "editor/Syntax.h"

#include <QWidget>

class CodeEditor;
class Minimap;

// One open document: editor plus its minimap, and the file-backing state that
// MainWindow needs for tab titles, dirty markers and save behaviour.
class EditorPane : public QWidget
{
    Q_OBJECT

public:
    explicit EditorPane(QWidget *parent = nullptr);

    CodeEditor *editor() const { return m_editor; }

    QString filePath() const { return m_filePath; }
    void setFilePath(const QString &path);

    // File name, or "untitled N" for a never-saved buffer.
    QString displayName() const;
    bool isModified() const;

    bool loadFile(const QString &path, QString *error);
    bool saveFile(const QString &path, QString *error);

    // Fills a never-saved buffer with converted content, e.g. the YAML produced
    // from a JSON document.
    void setContent(const QString &text, Language lang);

signals:
    void modificationChanged(bool modified);

private:
    CodeEditor *m_editor = nullptr;
    Minimap *m_minimap = nullptr;
    QString m_filePath;
    int m_untitledIndex = 0;

    static int s_untitledCounter;
};
