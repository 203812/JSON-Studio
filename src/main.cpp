#include "ui/MainWindow.h"
#include "ui/Theme.h"

#include <QApplication>
#include <QIcon>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("JSON Studio"));
    app.setOrganizationName(QStringLiteral("JSON Studio"));
    app.setWindowIcon(QIcon(QStringLiteral(":/app/app-icon.png")));

    // Fusion ignores the Windows native theme, which is what lets our QSS
    // control every pixel of the chrome.
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    app.setStyleSheet(theme::styleSheet());

    MainWindow w;
    w.show();

    QStringList files = app.arguments();
    files.removeFirst(); // argv[0]
    if (!files.isEmpty())
        w.openFiles(files);

    return app.exec();
}
