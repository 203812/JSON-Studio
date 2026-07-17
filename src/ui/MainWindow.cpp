#include "ui/MainWindow.h"

#include "core/JsonTools.h"
#include "core/Updater.h"
#include "core/Version.h"
#include "core/YamlTools.h"
#include "editor/CodeEditor.h"
#include "editor/EditorPane.h"
#include "ui/CommandPalette.h"
#include "ui/Theme.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QIcon>
#include <QPushButton>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QGuiApplication>
#include <QScreen>
#include <QScrollBar>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QTabWidget>
#include <QTextBlock>
#include <QTimer>
#include <QTreeView>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("JSON Studio"));

    // Never open larger than the screen: on a 1280x720 panel a hardcoded size
    // pushes the minimap off the right edge and the status bar off the bottom.
    const QRect avail = QGuiApplication::primaryScreen()->availableGeometry();
    resize(qMin(1280, int(avail.width() * 0.92)), qMin(820, int(avail.height() * 0.92)));
    move(avail.center() - QPoint(width() / 2, height() / 2));

    buildUi();
    buildMenus();
    buildCommands();
    setupUpdater();

    newTab();
    refreshStatus();

    installDebugHooks();
}

// Diagnostics for the two things that are painful to reason about from the
// outside: font resolution across DPI, and whether widgets actually got the
// geometry the layout promised.
void MainWindow::installDebugHooks()
{
    if (qEnvironmentVariableIsSet("JSONSTUDIO_DEBUG")) {
        // Layout only settles after show(), so sample it a beat later.
        QTimer::singleShot(1500, this, [this] {
            QScreen *scr = QGuiApplication::primaryScreen();
            EditorPane *pane = currentPane();
            const QFont f = pane->editor()->font();
            const QFontInfo fi(f);
            const QFontMetrics fm(f);
            qInfo() << "font     : requested" << f.family() << f.pointSizeF() << "-> resolved" << fi.family()
                    << fi.pointSizeF() << "pixelSize" << fi.pixelSize();
            qInfo() << "metrics  : height" << fm.height() << "advance" << fm.horizontalAdvance(u'0');
            qInfo() << "screen   : logicalDpi" << scr->logicalDotsPerInch() << "dpr" << scr->devicePixelRatio()
                    << "available" << scr->availableGeometry();
            qInfo() << "window   :" << size() << "editor viewport" << pane->editor()->viewport()->width();
            const QIcon ic = QApplication::windowIcon();
            qInfo() << "windowIcon: null?" << ic.isNull() << "sizes" << ic.availableSizes();
            QWidget *mm = pane->findChild<QWidget *>("Minimap");
            qInfo() << "minimap  :" << (mm ? mm->geometry() : QRect()) << "visible" << (mm && mm->isVisible());
            qInfo() << "statusBar:" << statusBar()->size() << "visible" << statusBar()->isVisible();
        });
    }

    // Grab through Qt: an external screenshot tool runs DPI-virtualised and
    // silently returns a magnified crop, which hides the right and bottom edges.
    const QString shot = qEnvironmentVariable("JSONSTUDIO_SCREENSHOT");
    if (!shot.isEmpty()) {
        QTimer::singleShot(1800, this, [this, shot] {
            const QPixmap pm = grab();
            qInfo() << "screenshot" << (pm.save(shot) ? "written" : "FAILED") << pm.size();
            QCoreApplication::quit();
        });
    }
}

// ---------------------------------------------------------------- UI assembly

void MainWindow::buildUi()
{
    auto *root = new QWidget(this);
    root->setObjectName("Root");
    auto *rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto *splitter = new QSplitter(Qt::Horizontal, root);
    splitter->setHandleWidth(1);
    splitter->setChildrenCollapsible(false);

    // Sidebar
    m_sidebar = new QWidget(splitter);
    m_sidebar->setObjectName("Sidebar");
    auto *sideLayout = new QVBoxLayout(m_sidebar);
    sideLayout->setContentsMargins(0, 0, 0, 0);
    sideLayout->setSpacing(0);

    auto *header = new QLabel(QStringLiteral("FOLDERS"), m_sidebar);
    header->setObjectName("SidebarHeader");
    sideLayout->addWidget(header);

    m_fsModel = new QFileSystemModel(this);
    m_fsModel->setNameFilters({QStringLiteral("*.json"), QStringLiteral("*.jsonc"), QStringLiteral("*.yaml"),
                               QStringLiteral("*.yml")});
    m_fsModel->setNameFilterDisables(false); // grey out non-matches instead of hiding dirs

    m_tree = new QTreeView(m_sidebar);
    m_tree->setObjectName("FileTree");
    m_tree->setModel(m_fsModel);
    m_tree->setHeaderHidden(true);
    m_tree->setAnimated(false);
    m_tree->setIndentation(14);
    m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    for (int c = 1; c < m_fsModel->columnCount(); ++c)
        m_tree->hideColumn(c);
    // A QFileSystemModel with no root path lists "This PC". Keep the tree
    // hidden until a folder is actually opened, the way Sublime does.
    m_tree->hide();
    sideLayout->addWidget(m_tree, 1);

    m_sidebarEmpty = new QLabel(QStringLiteral("Geen map geopend.\n\nFile › Open Folder…"), m_sidebar);
    m_sidebarEmpty->setObjectName("SidebarEmpty");
    m_sidebarEmpty->setWordWrap(true);
    m_sidebarEmpty->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    sideLayout->addWidget(m_sidebarEmpty, 1);

    connect(m_tree, &QTreeView::doubleClicked, this, [this](const QModelIndex &idx) {
        if (!m_fsModel->isDir(idx))
            openPath(m_fsModel->filePath(idx));
    });

    // Tabs
    m_tabs = new QTabWidget(splitter);
    m_tabs->setDocumentMode(true);
    m_tabs->setTabsClosable(true);
    m_tabs->setMovable(true);
    connect(m_tabs, &QTabWidget::tabCloseRequested, this, &MainWindow::actClose);
    connect(m_tabs, &QTabWidget::currentChanged, this, [this](int) { refreshStatus(); });

    splitter->addWidget(m_sidebar);
    splitter->addWidget(m_tabs);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({230, 1050});

    rootLayout->addWidget(splitter);
    setCentralWidget(root);

    // Status bar
    m_statusMessage = new QLabel(this);
    m_statusMessage->setObjectName("StatusMessage");
    m_statusPos = new QLabel(this);
    m_statusIndent = new QLabel(QStringLiteral("Spaces: 2"), this);
    m_statusSyntax = new QLabel(QStringLiteral("JSON"), this);

    statusBar()->addWidget(m_statusMessage, 1);
    statusBar()->addPermanentWidget(m_statusPos);
    statusBar()->addPermanentWidget(m_statusIndent);
    statusBar()->addPermanentWidget(m_statusSyntax);
    statusBar()->setSizeGripEnabled(false);

    m_palette = new CommandPalette(root);
}

void MainWindow::buildMenus()
{
    QMenu *file = menuBar()->addMenu(QStringLiteral("&File"));
    file->addAction(QStringLiteral("New File"), QKeySequence::New, this, &MainWindow::actNew);
    file->addAction(QStringLiteral("Open File…"), QKeySequence::Open, this, &MainWindow::actOpen);
    file->addAction(QStringLiteral("Open Folder…"), QKeySequence(QStringLiteral("Ctrl+K, Ctrl+O")), this,
                    &MainWindow::actOpenFolder);
    file->addSeparator();
    file->addAction(QStringLiteral("Save"), QKeySequence::Save, this, &MainWindow::actSave);
    file->addAction(QStringLiteral("Save As…"), QKeySequence::SaveAs, this, &MainWindow::actSaveAs);
    file->addSeparator();
    file->addAction(QStringLiteral("Close File"), QKeySequence::Close, this,
                    [this] { actClose(m_tabs->currentIndex()); });
    file->addAction(QStringLiteral("Exit"), QKeySequence::Quit, this, &QWidget::close);

    QMenu *edit = menuBar()->addMenu(QStringLiteral("&Edit"));
    edit->addAction(QStringLiteral("Undo"), QKeySequence::Undo, this, [this] {
        if (auto *p = currentPane())
            p->editor()->undo();
    });
    edit->addAction(QStringLiteral("Redo"), QKeySequence::Redo, this, [this] {
        if (auto *p = currentPane())
            p->editor()->redo();
    });
    edit->addSeparator();
    edit->addAction(QStringLiteral("Cut"), QKeySequence::Cut, this, [this] {
        if (auto *p = currentPane())
            p->editor()->cut();
    });
    edit->addAction(QStringLiteral("Copy"), QKeySequence::Copy, this, [this] {
        if (auto *p = currentPane())
            p->editor()->copy();
    });
    edit->addAction(QStringLiteral("Paste"), QKeySequence::Paste, this, [this] {
        if (auto *p = currentPane())
            p->editor()->paste();
    });

    QMenu *tools = menuBar()->addMenu(QStringLiteral("&Tools"));
    tools->addAction(QStringLiteral("Format Document"), QKeySequence(QStringLiteral("Ctrl+Shift+F")), this,
                     &MainWindow::actFormat);
    tools->addAction(QStringLiteral("Minify Document"), QKeySequence(QStringLiteral("Ctrl+Shift+M")), this,
                     &MainWindow::actMinify);
    tools->addAction(QStringLiteral("Validate"), QKeySequence(QStringLiteral("Ctrl+Shift+V")), this,
                     &MainWindow::actValidate);
    tools->addSeparator();
    tools->addAction(QStringLiteral("Convert to YAML"), this, &MainWindow::actToYaml);
    tools->addAction(QStringLiteral("Convert to JSON"), this, &MainWindow::actToJson);
    tools->addSeparator();
    tools->addAction(QStringLiteral("Command Palette…"), QKeySequence(QStringLiteral("Ctrl+Shift+P")), this,
                     [this] { m_palette->showPalette(); });

    QMenu *view = menuBar()->addMenu(QStringLiteral("&View"));
    view->addAction(QStringLiteral("Toggle Sidebar"), QKeySequence(QStringLiteral("Ctrl+K, Ctrl+B")), this,
                    &MainWindow::actToggleSidebar);

    QMenu *help = menuBar()->addMenu(QStringLiteral("&Help"));
    help->addAction(QStringLiteral("Check for Updates…"), this, &MainWindow::actCheckForUpdates);
    help->addAction(QStringLiteral("About JSON Studio"), this, &MainWindow::actAbout);
}

void MainWindow::buildCommands()
{
    QList<CommandPalette::Command> cmds{
        {QStringLiteral("JSON: Format Document"), QStringLiteral("Ctrl+Shift+F"), [this] { actFormat(); }},
        {QStringLiteral("JSON: Minify Document"), QStringLiteral("Ctrl+Shift+M"), [this] { actMinify(); }},
        {QStringLiteral("JSON: Validate"), QStringLiteral("Ctrl+Shift+V"), [this] { actValidate(); }},
        {QStringLiteral("Convert: JSON to YAML"), QString(), [this] { actToYaml(); }},
        {QStringLiteral("Convert: YAML to JSON"), QString(), [this] { actToJson(); }},
        {QStringLiteral("Help: Check for Updates…"), QString(), [this] { actCheckForUpdates(); }},
        {QStringLiteral("File: New"), QStringLiteral("Ctrl+N"), [this] { actNew(); }},
        {QStringLiteral("File: Open…"), QStringLiteral("Ctrl+O"), [this] { actOpen(); }},
        {QStringLiteral("File: Open Folder…"), QString(), [this] { actOpenFolder(); }},
        {QStringLiteral("File: Save"), QStringLiteral("Ctrl+S"), [this] { actSave(); }},
        {QStringLiteral("View: Toggle Sidebar"), QString(), [this] { actToggleSidebar(); }},
    };
    m_palette->setCommands(std::move(cmds));
}

// ------------------------------------------------------------------- Tab plumbing

void MainWindow::openFiles(const QStringList &paths)
{
    for (const QString &p : paths)
        openPath(p);
}

EditorPane *MainWindow::currentPane() const
{
    return qobject_cast<EditorPane *>(m_tabs->currentWidget());
}

EditorPane *MainWindow::newTab()
{
    auto *pane = new EditorPane(m_tabs);
    const int idx = m_tabs->addTab(pane, pane->displayName());
    m_tabs->setCurrentIndex(idx);

    connect(pane, &EditorPane::modificationChanged, this, [this, pane](bool) { updateTabTitle(pane); });
    connect(pane->editor(), &QPlainTextEdit::cursorPositionChanged, this, &MainWindow::refreshStatus);
    connect(pane->editor(), &QPlainTextEdit::textChanged, this, [this, pane] {
        // Any edit invalidates a previously flagged error line.
        pane->editor()->setErrorLine(-1);
    });

    pane->editor()->setFocus();
    return pane;
}

void MainWindow::updateTabTitle(EditorPane *pane)
{
    const int idx = m_tabs->indexOf(pane);
    if (idx < 0)
        return;
    const QString name = pane->displayName();
    m_tabs->setTabText(idx, pane->isModified() ? name + QStringLiteral(" •") : name);
    m_tabs->setTabToolTip(idx, pane->filePath());
}

void MainWindow::openPath(const QString &path)
{
    // Already open? Just focus it.
    for (int i = 0; i < m_tabs->count(); ++i) {
        auto *p = qobject_cast<EditorPane *>(m_tabs->widget(i));
        if (p && p->filePath() == path) {
            m_tabs->setCurrentIndex(i);
            return;
        }
    }

    // Reuse a pristine untitled tab rather than stacking an empty one.
    EditorPane *pane = currentPane();
    if (!pane || !pane->filePath().isEmpty() || pane->isModified()
        || !pane->editor()->toPlainText().isEmpty()) {
        pane = newTab();
    }

    QString err;
    if (!pane->loadFile(path, &err)) {
        setStatusMessage(QStringLiteral("Could not open %1: %2").arg(QFileInfo(path).fileName(), err), true);
        return;
    }
    updateTabTitle(pane);
    setStatusMessage(QString(), false);
    refreshStatus();
}

bool MainWindow::savePane(EditorPane *pane, bool saveAs)
{
    if (!pane)
        return false;

    QString path = pane->filePath();
    if (saveAs || path.isEmpty()) {
        path = QFileDialog::getSaveFileName(this, QStringLiteral("Save As"),
                                            path.isEmpty() ? pane->displayName() + QStringLiteral(".json") : path,
                                            QStringLiteral("JSON (*.json *.jsonc);;All files (*)"));
        if (path.isEmpty())
            return false;
    }

    QString err;
    if (!pane->saveFile(path, &err)) {
        setStatusMessage(QStringLiteral("Could not save: %1").arg(err), true);
        return false;
    }
    updateTabTitle(pane);
    setStatusMessage(QStringLiteral("Saved %1").arg(QFileInfo(path).fileName()), false);
    return true;
}

bool MainWindow::maybeSave(EditorPane *pane)
{
    if (!pane || !pane->isModified())
        return true;

    const auto choice = QMessageBox::warning(
        this, QStringLiteral("JSON Studio"),
        QStringLiteral("%1 has unsaved changes.").arg(pane->displayName()),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (choice == QMessageBox::Save)
        return savePane(pane, false);
    return choice == QMessageBox::Discard;
}

// ---------------------------------------------------------------------- Status

void MainWindow::setStatusMessage(const QString &text, bool error)
{
    m_statusMessage->setText(text);
    m_statusMessage->setObjectName(error ? "StatusError" : "StatusMessage");
    // Re-polish so the objectName-based QSS rule actually takes effect.
    m_statusMessage->style()->unpolish(m_statusMessage);
    m_statusMessage->style()->polish(m_statusMessage);
}

void MainWindow::refreshStatus()
{
    EditorPane *pane = currentPane();
    if (!pane) {
        m_statusPos->setText(QString());
        return;
    }
    const QTextCursor c = pane->editor()->textCursor();
    m_statusPos->setText(
        QStringLiteral("Line %1, Column %2").arg(c.blockNumber() + 1).arg(c.positionInBlock() + 1));
    m_statusIndent->setText(QStringLiteral("Spaces: %1").arg(pane->editor()->indentWidth()));
    m_statusSyntax->setText(syntax::displayName(pane->editor()->language()));
}

// --------------------------------------------------------------------- Actions

void MainWindow::actNew()
{
    newTab();
}

void MainWindow::actOpen()
{
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Open File"), QString(),
                                                      QStringLiteral("JSON (*.json *.jsonc);;All files (*)"));
    if (!path.isEmpty())
        openPath(path);
}

void MainWindow::actOpenFolder()
{
    const QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("Open Folder"));
    if (dir.isEmpty())
        return;
    m_fsModel->setRootPath(dir);
    m_tree->setRootIndex(m_fsModel->index(dir));
    m_sidebarEmpty->hide();
    m_tree->show();
    m_sidebar->show();
}

void MainWindow::actSave()
{
    savePane(currentPane(), false);
}

void MainWindow::actSaveAs()
{
    savePane(currentPane(), true);
}

void MainWindow::actClose(int index)
{
    auto *pane = qobject_cast<EditorPane *>(m_tabs->widget(index));
    if (!pane || !maybeSave(pane))
        return;

    m_tabs->removeTab(index);
    pane->deleteLater();

    if (m_tabs->count() == 0)
        newTab();
}

// The JSON tools are meaningless on a YAML buffer; say so rather than emit
// nonsense.
bool MainWindow::requireJson(EditorPane *pane, const QString &action)
{
    if (!pane)
        return false;
    if (pane->editor()->language() == Language::Json)
        return true;
    setStatusMessage(QStringLiteral("%1 works on JSON documents; this one is %2")
                         .arg(action, syntax::displayName(pane->editor()->language())),
                     true);
    return false;
}

void MainWindow::actFormat()
{
    EditorPane *pane = currentPane();
    if (!requireJson(pane, QStringLiteral("Format")))
        return;

    CodeEditor *ed = pane->editor();
    const jsontools::Result r = jsontools::format(ed->toPlainText(), ed->indentWidth());
    if (!r.ok) {
        ed->setErrorLine(r.line);
        setStatusMessage(QStringLiteral("Line %1, Column %2: %3").arg(r.line).arg(r.column).arg(r.message), true);
        return;
    }

    // Replace via cursor so the edit lands on the undo stack as one step.
    QTextCursor c = ed->textCursor();
    const int scroll = ed->verticalScrollBar()->value();
    c.beginEditBlock();
    c.select(QTextCursor::Document);
    c.insertText(r.text);
    c.endEditBlock();
    ed->verticalScrollBar()->setValue(scroll);

    ed->setErrorLine(-1);
    setStatusMessage(QStringLiteral("Formatted"), false);
}

void MainWindow::actMinify()
{
    EditorPane *pane = currentPane();
    if (!requireJson(pane, QStringLiteral("Minify")))
        return;

    CodeEditor *ed = pane->editor();
    const jsontools::Result r = jsontools::minify(ed->toPlainText());
    if (!r.ok) {
        ed->setErrorLine(r.line);
        setStatusMessage(QStringLiteral("Line %1, Column %2: %3").arg(r.line).arg(r.column).arg(r.message), true);
        return;
    }

    QTextCursor c = ed->textCursor();
    c.beginEditBlock();
    c.select(QTextCursor::Document);
    c.insertText(r.text);
    c.endEditBlock();

    ed->setErrorLine(-1);
    setStatusMessage(QStringLiteral("Minified"), false);
}

void MainWindow::actValidate()
{
    EditorPane *pane = currentPane();
    if (!requireJson(pane, QStringLiteral("Validate")))
        return;

    CodeEditor *ed = pane->editor();
    const jsontools::Result r = jsontools::validate(ed->toPlainText());
    if (r.ok) {
        ed->setErrorLine(-1);
        setStatusMessage(QStringLiteral("Valid JSON"), false);
        m_statusMessage->setObjectName("StatusOk");
        m_statusMessage->style()->unpolish(m_statusMessage);
        m_statusMessage->style()->polish(m_statusMessage);
        return;
    }

    ed->setErrorLine(r.line);
    setStatusMessage(QStringLiteral("Line %1, Column %2: %3").arg(r.line).arg(r.column).arg(r.message), true);

    // Park the caret on the offending line.
    if (r.line > 0) {
        QTextCursor c(ed->document()->findBlockByNumber(r.line - 1));
        c.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, qMax(0, r.column - 1));
        ed->setTextCursor(c);
        ed->centerCursor();
    }
}

void MainWindow::openConverted(const QString &text, Language lang, const QString &sourceName)
{
    EditorPane *pane = newTab();
    pane->setContent(text, lang);
    updateTabTitle(pane);
    refreshStatus();
    setStatusMessage(QStringLiteral("%1 → %2").arg(sourceName, syntax::displayName(lang)), false);
}

void MainWindow::actToYaml()
{
    EditorPane *pane = currentPane();
    if (!pane)
        return;

    CodeEditor *ed = pane->editor();
    if (ed->language() == Language::Yaml) {
        setStatusMessage(QStringLiteral("Document is already YAML"), true);
        return;
    }

    const yamltools::Result r = yamltools::jsonToYaml(ed->toPlainText());
    if (!r.ok) {
        setStatusMessage(QStringLiteral("Convert to YAML: %1").arg(r.message), true);
        return;
    }
    openConverted(r.text, Language::Yaml, pane->displayName());
}

void MainWindow::actToJson()
{
    EditorPane *pane = currentPane();
    if (!pane)
        return;

    CodeEditor *ed = pane->editor();
    if (ed->language() == Language::Json) {
        setStatusMessage(QStringLiteral("Document is already JSON"), true);
        return;
    }

    const yamltools::Result r = yamltools::yamlToJson(ed->toPlainText(), ed->indentWidth());
    if (!r.ok) {
        if (r.line > 0) {
            ed->setErrorLine(r.line);
            setStatusMessage(
                QStringLiteral("Line %1, Column %2: %3").arg(r.line).arg(r.column).arg(r.message), true);
        } else {
            setStatusMessage(QStringLiteral("Convert to JSON: %1").arg(r.message), true);
        }
        return;
    }
    openConverted(r.text, Language::Json, pane->displayName());
}

void MainWindow::actToggleSidebar()
{
    m_sidebar->setVisible(!m_sidebar->isVisible());
}

// ----------------------------------------------------------------- Updates

void MainWindow::setupUpdater()
{
    m_updater = new Updater(this);
    connect(m_updater, &Updater::updateAvailable, this, &MainWindow::onUpdateAvailable);
    connect(m_updater, &Updater::upToDate, this, [this](const QString &v) {
        setStatusMessage(QStringLiteral("Up to date (version %1)").arg(v), false);
    });
    connect(m_updater, &Updater::checkFailed, this, [this](const QString &err) {
        setStatusMessage(QStringLiteral("Update check failed: %1").arg(err), true);
    });

    // A quiet check shortly after launch: it only speaks up if there is an
    // update, so it never nags a user who is current.
    const int delay = qEnvironmentVariableIsSet("JSONSTUDIO_DEBUG") ? 300 : 2500;
    QTimer::singleShot(delay, this, [this] { m_updater->checkForUpdates(/*silent=*/true); });
}

void MainWindow::actCheckForUpdates()
{
    setStatusMessage(QStringLiteral("Checking for updates…"), false);
    m_updater->checkForUpdates(/*silent=*/false);
}

void MainWindow::onUpdateAvailable(const QString &ver, const QString &pageUrl, const QString &notes)
{
    QMessageBox box(this);
    box.setWindowTitle(QStringLiteral("Update available"));
    box.setIcon(QMessageBox::Information);
    box.setText(QStringLiteral("JSON Studio %1 is available.\nYou have %2.")
                    .arg(ver, version::current()));
    if (!notes.trimmed().isEmpty())
        box.setDetailedText(notes.trimmed());
    QPushButton *download = box.addButton(QStringLiteral("Download"), QMessageBox::AcceptRole);
    box.addButton(QStringLiteral("Later"), QMessageBox::RejectRole);
    box.exec();

    if (box.clickedButton() == download && !pageUrl.isEmpty())
        QDesktopServices::openUrl(QUrl(pageUrl));
}

void MainWindow::actAbout()
{
    QMessageBox::about(
        this, QStringLiteral("About JSON Studio"),
        QStringLiteral("<h3>JSON Studio %1</h3>"
                       "<p>Meer dan een formatter — een JSON-werkbank met het uiterlijk van Sublime Text.</p>"
                       "<p><a href=\"https://github.com/203812/JSON-Studio\">github.com/203812/JSON-Studio</a></p>")
            .arg(version::current()));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    for (int i = 0; i < m_tabs->count(); ++i) {
        auto *pane = qobject_cast<EditorPane *>(m_tabs->widget(i));
        if (!maybeSave(pane)) {
            event->ignore();
            return;
        }
    }
    event->accept();
}
