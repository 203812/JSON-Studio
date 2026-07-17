#pragma once

#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

// Checks the public GitHub repo's latest release and reports whether it is newer
// than this build. It never downloads or installs on its own — it hands back the
// release page URL so the UI can offer the download. That keeps a portable app
// from trying to overwrite its own running executable.
class Updater : public QObject
{
    Q_OBJECT

public:
    explicit Updater(QObject *parent = nullptr);

    // silent == true suppresses the "you are up to date" / error signals, so a
    // startup check only ever speaks up when there is genuinely an update.
    void checkForUpdates(bool silent);

    bool isChecking() const { return m_inFlight != nullptr; }

signals:
    void updateAvailable(const QString &version, const QString &pageUrl, const QString &notes);
    void upToDate(const QString &version); // only when !silent
    void checkFailed(const QString &error); // only when !silent

private:
    void onReply(QNetworkReply *reply, bool silent);

    QNetworkAccessManager *m_net = nullptr;
    QNetworkReply *m_inFlight = nullptr;

    static constexpr const char *kLatestReleaseUrl =
        "https://api.github.com/repos/203812/JSON-Studio/releases/latest";
};
