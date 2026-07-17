#include "editor/EditorPane.h"

#include "editor/CodeEditor.h"
#include "editor/Minimap.h"

#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QSaveFile>
#include <QTextStream>

int EditorPane::s_untitledCounter = 0;

EditorPane::EditorPane(QWidget *parent)
    : QWidget(parent)
    , m_untitledIndex(++s_untitledCounter)
{
    m_editor = new CodeEditor(this);
    m_minimap = new Minimap(m_editor, this);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_editor, 1);
    layout->addWidget(m_minimap, 0);

    connect(m_editor->document(), &QTextDocument::modificationChanged, this, &EditorPane::modificationChanged);
}

void EditorPane::setFilePath(const QString &path)
{
    m_filePath = path;
    m_editor->setLanguage(syntax::fromFileName(path));
}

QString EditorPane::displayName() const
{
    if (m_filePath.isEmpty())
        return QStringLiteral("untitled %1.%2")
            .arg(m_untitledIndex)
            .arg(syntax::defaultExtension(m_editor->language()));
    return QFileInfo(m_filePath).fileName();
}

bool EditorPane::isModified() const
{
    return m_editor->document()->isModified();
}

void EditorPane::setContent(const QString &text, Language lang)
{
    m_editor->setLanguage(lang);
    m_editor->setPlainText(text);
    // A conversion result is unsaved work, not a pristine buffer.
    m_editor->document()->setModified(true);
}

bool EditorPane::loadFile(const QString &path, QString *error)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error)
            *error = f.errorString();
        return false;
    }

    QTextStream in(&f);
    in.setEncoding(QStringConverter::Utf8);
    m_editor->setLanguage(syntax::fromFileName(path));
    m_editor->setPlainText(in.readAll());
    m_editor->document()->setModified(false);
    m_filePath = path;
    return true;
}

bool EditorPane::saveFile(const QString &path, QString *error)
{
    // QSaveFile writes to a temp file and renames, so a failed write can't
    // truncate the user's existing document.
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error)
            *error = f.errorString();
        return false;
    }

    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    out << m_editor->toPlainText();

    if (!f.commit()) {
        if (error)
            *error = f.errorString();
        return false;
    }

    m_editor->document()->setModified(false);
    m_filePath = path;
    m_editor->setLanguage(syntax::fromFileName(path));
    return true;
}
