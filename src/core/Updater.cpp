#include "core/Updater.h"

#include "core/Version.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

Updater::Updater(QObject *parent)
    : QObject(parent)
    , m_net(new QNetworkAccessManager(this))
{
}

void Updater::checkForUpdates(bool silent)
{
    if (m_inFlight)
        return; // one check at a time

    QNetworkRequest req{QUrl(QString::fromUtf8(kLatestReleaseUrl))};
    req.setRawHeader("Accept", "application/vnd.github+json");
    req.setRawHeader("User-Agent", "JSON-Studio-Updater");
    // A slow network shouldn't hang the app; the reply errors out instead.
    req.setTransferTimeout(8000);

    m_inFlight = m_net->get(req);
    connect(m_inFlight, &QNetworkReply::finished, this, [this, silent] {
        QNetworkReply *reply = m_inFlight;
        m_inFlight = nullptr;
        onReply(reply, silent);
        reply->deleteLater();
    });
}

void Updater::onReply(QNetworkReply *reply, bool silent)
{
    if (reply->error() != QNetworkReply::NoError) {
        if (!silent)
            emit checkFailed(reply->errorString());
        return;
    }

    const QByteArray body = reply->readAll();
    QJsonParseError perr;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &perr);
    if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
        if (!silent)
            emit checkFailed(QStringLiteral("Unexpected response from GitHub"));
        return;
    }

    const QJsonObject obj = doc.object();
    // A repo with no published (non-draft) release returns 404, handled above.
    const QString tag = obj.value(QStringLiteral("tag_name")).toString();
    if (tag.isEmpty()) {
        if (!silent)
            emit checkFailed(QStringLiteral("No release information found"));
        return;
    }

    const QString cur = version::current();
    if (qEnvironmentVariableIsSet("JSONSTUDIO_DEBUG"))
        qInfo() << "updater: latest tag" << tag << "current" << cur << "-> newer?"
                << version::isNewer(tag, cur);

    if (version::isNewer(tag, cur)) {
        const QString pageUrl = obj.value(QStringLiteral("html_url")).toString();
        const QString notes = obj.value(QStringLiteral("body")).toString();
        emit updateAvailable(tag, pageUrl, notes);
    } else if (!silent) {
        emit upToDate(cur);
    }
}
