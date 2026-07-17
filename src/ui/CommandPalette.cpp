#include "ui/CommandPalette.h"

#include "ui/Theme.h"

#include <QEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>

#include <algorithm>

CommandPalette::CommandPalette(QWidget *parent)
    : QFrame(parent)
{
    setObjectName("PaletteFrame");
    setFrameShape(QFrame::NoFrame);
    hide();

    m_input = new QLineEdit(this);
    m_input->setObjectName("PaletteInput");
    m_input->setPlaceholderText(QStringLiteral("Type a command…"));

    m_list = new QListWidget(this);
    m_list->setObjectName("PaletteList");
    m_list->setUniformItemSizes(true);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setFocusPolicy(Qt::NoFocus);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(1, 1, 1, 1);
    layout->setSpacing(0);
    layout->addWidget(m_input);
    layout->addWidget(m_list);

    connect(m_input, &QLineEdit::textChanged, this, &CommandPalette::refilter);
    connect(m_input, &QLineEdit::returnPressed, this, &CommandPalette::accept);
    connect(m_list, &QListWidget::itemClicked, this, &CommandPalette::accept);

    // Arrow keys must drive the list while focus stays in the text field.
    m_input->installEventFilter(this);
    if (parent)
        parent->installEventFilter(this);
}

void CommandPalette::setCommands(QList<Command> commands)
{
    m_commands = std::move(commands);
}

int CommandPalette::fuzzyScore(const QString &haystack, const QString &needle)
{
    if (needle.isEmpty())
        return 0;

    int hi = 0;
    int score = 0;
    int lastHit = -1;

    for (int ni = 0; ni < needle.size(); ++ni) {
        const QChar want = needle.at(ni).toLower();
        bool found = false;
        while (hi < haystack.size()) {
            if (haystack.at(hi).toLower() == want) {
                // Prefer hits that are close together and at word starts.
                if (lastHit >= 0)
                    score += (hi - lastHit - 1);
                const bool wordStart = hi == 0 || haystack.at(hi - 1) == u' ' || haystack.at(hi - 1) == u':';
                if (!wordStart)
                    score += 1;
                lastHit = hi;
                ++hi;
                found = true;
                break;
            }
            ++hi;
        }
        if (!found)
            return -1;
    }
    return score;
}

void CommandPalette::refilter(const QString &query)
{
    m_filtered.clear();
    m_list->clear();

    QList<QPair<int, int>> scored; // (score, index)
    for (int i = 0; i < m_commands.size(); ++i) {
        const int s = fuzzyScore(m_commands.at(i).title, query.trimmed());
        if (s >= 0)
            scored.append({s, i});
    }

    std::stable_sort(scored.begin(), scored.end(), [](const auto &a, const auto &b) { return a.first < b.first; });

    for (const auto &[score, idx] : scored) {
        const Command &c = m_commands.at(idx);
        auto *item = new QListWidgetItem(c.title, m_list);
        if (!c.shortcut.isEmpty()) {
            item->setText(c.title);
            item->setToolTip(c.shortcut);
        }
        m_filtered.append(idx);
    }

    if (m_list->count() > 0)
        m_list->setCurrentRow(0);
}

void CommandPalette::reposition()
{
    QWidget *p = parentWidget();
    if (!p)
        return;
    const int w = qMin(560, qMax(320, p->width() / 2));
    const int h = 320;
    move((p->width() - w) / 2, 0);
    resize(w, h);
}

void CommandPalette::showPalette()
{
    refilter(QString());
    m_input->clear();
    reposition();
    show();
    raise();
    m_input->setFocus(Qt::ShortcutFocusReason);
}

void CommandPalette::accept()
{
    const int row = m_list->currentRow();
    if (row < 0 || row >= m_filtered.size())
        return;
    const Command cmd = m_commands.at(m_filtered.at(row));
    hide();
    if (cmd.run)
        cmd.run();
}

bool CommandPalette::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_input && event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent *>(event);
        switch (ke->key()) {
        case Qt::Key_Down:
            if (m_list->count())
                m_list->setCurrentRow(qMin(m_list->currentRow() + 1, m_list->count() - 1));
            return true;
        case Qt::Key_Up:
            if (m_list->count())
                m_list->setCurrentRow(qMax(m_list->currentRow() - 1, 0));
            return true;
        case Qt::Key_Escape:
            hide();
            return true;
        default:
            break;
        }
    }

    // Keep the palette centred when the window resizes underneath it.
    if (watched == parentWidget() && event->type() == QEvent::Resize && isVisible())
        reposition();

    return QFrame::eventFilter(watched, event);
}

void CommandPalette::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        hide();
        return;
    }
    QFrame::keyPressEvent(event);
}
