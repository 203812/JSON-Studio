#pragma once

#include "editor/Syntax.h"

#include <QMainWindow>

class CommandPalette;
class EditorPane;
class Updater;
class QFileSystemModel;
class QLabel;
class QTabWidget;
class QTreeView;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    // Files named on the command line.
    void openFiles(const QStringList &paths);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void buildUi();
    void buildMenus();
    void buildCommands();
    void installDebugHooks();

    EditorPane *currentPane() const;
    EditorPane *newTab();
    bool requireJson(EditorPane *pane, const QString &action);
    void openPath(const QString &path);
    bool savePane(EditorPane *pane, bool saveAs);
    bool maybeSave(EditorPane *pane);
    void updateTabTitle(EditorPane *pane);
    void refreshStatus();
    void setStatusMessage(const QString &text, bool error);

    // Actions
    void actNew();
    void actOpen();
    void actOpenFolder();
    void actSave();
    void actSaveAs();
    void actClose(int index);
    void actFormat();
    void actMinify();
    void actValidate();
    void actToYaml();
    void actToJson();
    void actToggleSidebar();
    void actCheckForUpdates(); // manual: always reports the outcome
    void actAbout();

    // Conversions never overwrite the source; they open the result in a new tab.
    void openConverted(const QString &text, Language lang, const QString &sourceName);

    void setupUpdater();
    void onUpdateAvailable(const QString &version, const QString &pageUrl, const QString &notes);

    QTabWidget *m_tabs = nullptr;
    QTreeView *m_tree = nullptr;
    QFileSystemModel *m_fsModel = nullptr;
    QWidget *m_sidebar = nullptr;
    QLabel *m_sidebarEmpty = nullptr;
    CommandPalette *m_palette = nullptr;

    QLabel *m_statusPos = nullptr;
    QLabel *m_statusIndent = nullptr;
    QLabel *m_statusSyntax = nullptr;
    QLabel *m_statusMessage = nullptr;

    Updater *m_updater = nullptr;
};
