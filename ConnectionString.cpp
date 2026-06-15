#include "ConnectionString.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QHostAddress>

namespace {

const QString kPrefix = QStringLiteral("beatoven://share/");
constexpr int kMaxEncodedLength = 6144;   // hard cap before any decoding work
constexpr int kMaxAddresses = 8;
constexpr int kMaxAddressLength = 64;
constexpr int kMaxProjectNameLength = 128;
constexpr int kFingerprintLength = 64;    // SHA-256 as lowercase hex

bool validFingerprint(const QString& fp) {
    if (fp.size() != kFingerprintLength)
        return false;
    for (const QChar c : fp) {
        const char16_t u = c.unicode();
        const bool isHex = (u >= u'0' && u <= u'9') || (u >= u'a' && u <= u'f');
        if (!isHex)
            return false; // uppercase deliberately rejected: one canonical form
    }
    return true;
}

} // namespace

bool validProjectName(const QString& name) {
    if (name.isEmpty() || name.size() > kMaxProjectNameLength)
        return false;
    if (name == QStringLiteral("."))
        return false;
    if (name.contains(QLatin1Char('/')) || name.contains(QLatin1Char('\\')))
        return false;
    if (name.contains(QStringLiteral("..")))
        return false;
    for (const QChar c : name) {
        if (c.unicode() < 0x20 || c.unicode() == 0x7f)
            return false; // no control characters
    }
    return true;
}

bool parseAddress(const QString& address, QString& hostOut, quint16& portOut) {
    if (address.isEmpty() || address.size() > kMaxAddressLength)
        return false;

    QString hostPart;
    QString portPart;

    if (address.startsWith(QLatin1Char('['))) {
        // "[ipv6]:port"
        const int close = address.indexOf(QLatin1Char(']'));
        if (close < 0 || close + 1 >= address.size()
            || address.at(close + 1) != QLatin1Char(':'))
            return false;
        hostPart = address.mid(1, close - 1);
        portPart = address.mid(close + 2);
        QHostAddress host;
        if (!host.setAddress(hostPart)
            || host.protocol() != QAbstractSocket::IPv6Protocol)
            return false;
    } else {
        // "ipv4:port"
        const int colon = address.lastIndexOf(QLatin1Char(':'));
        if (colon <= 0 || colon + 1 >= address.size())
            return false;
        hostPart = address.left(colon);
        portPart = address.mid(colon + 1);
        QHostAddress host;
        if (!host.setAddress(hostPart)
            || host.protocol() != QAbstractSocket::IPv4Protocol)
            return false;
    }

    bool numberOk = false;
    const uint port = portPart.toUInt(&numberOk);
    if (!numberOk || port < 1 || port > 65535)
        return false;

    hostOut = hostPart;
    portOut = static_cast<quint16>(port);
    return true;
}

QString generateConnectionString(const ShareInfo& info) {
    QJsonObject obj;
    obj.insert(QStringLiteral("v"), 1);
    obj.insert(QStringLiteral("key"), info.fingerprint);
    QJsonArray addrs;
    for (const QString& a : info.addresses)
        addrs.append(a);
    obj.insert(QStringLiteral("addrs"), addrs);
    obj.insert(QStringLiteral("project"), info.project);

    const QByteArray json = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    const QByteArray encoded = json.toBase64(
        QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    return kPrefix + QString::fromLatin1(encoded);
}

// SECURITY-SENSITIVE: this is the app's parser for fully untrusted input
// (anyone can send a link that ends up launching the client). Every field is
// validated; `out` is written only after everything passed. Error messages
// never echo attacker-controlled content back.
bool parseConnectionString(QString url, ShareInfo& out, QString& error) {
    url = url.trimmed();

    if (url.size() > kMaxEncodedLength) {
        error = QStringLiteral("the share link is too long to be valid");
        return false;
    }
    // Browsers may lowercase the scheme; the prefix compare is therefore
    // case-insensitive. The base64url payload after it is case-sensitive.
    if (url.left(kPrefix.size()).compare(kPrefix, Qt::CaseInsensitive) != 0) {
        error = QStringLiteral("this is not a beatoven share link");
        return false;
    }

    const QByteArray encoded = url.mid(kPrefix.size()).toLatin1();
    if (encoded.isEmpty()) {
        error = QStringLiteral("the share link has no payload");
        return false;
    }

    const auto decoded = QByteArray::fromBase64Encoding(
        encoded,
        QByteArray::Base64UrlEncoding | QByteArray::AbortOnBase64DecodingErrors);
    if (decoded.decodingStatus != QByteArray::Base64DecodingStatus::Ok) {
        error = QStringLiteral("the share link payload is not valid base64url");
        return false;
    }

    QJsonParseError jsonError;
    const QJsonDocument doc = QJsonDocument::fromJson(decoded.decoded, &jsonError);
    if (jsonError.error != QJsonParseError::NoError || !doc.isObject()) {
        error = QStringLiteral("the share link payload is not valid JSON");
        return false;
    }
    const QJsonObject obj = doc.object();

    // Version: exactly 1. Unknown *keys* are tolerated (minor forward
    // compatibility); unknown *versions* are not.
    const QJsonValue version = obj.value(QStringLiteral("v"));
    if (!version.isDouble() || version.toInt(-1) != 1) {
        error = version.toInt(-1) > 1
                    ? QStringLiteral("this link was made by a newer BeatOven — please update")
                    : QStringLiteral("the share link version is invalid");
        return false;
    }

    const QString key = obj.value(QStringLiteral("key")).toString();
    if (!validFingerprint(key)) {
        error = QStringLiteral("the share link identity fingerprint is invalid");
        return false;
    }

    const QJsonValue addrsValue = obj.value(QStringLiteral("addrs"));
    if (!addrsValue.isArray()) {
        error = QStringLiteral("the share link has no address list");
        return false;
    }
    const QJsonArray addrsArray = addrsValue.toArray();
    if (addrsArray.isEmpty() || addrsArray.size() > kMaxAddresses) {
        error = QStringLiteral("the share link address list is invalid");
        return false;
    }
    QStringList addresses;
    for (const QJsonValue& value : addrsArray) {
        if (!value.isString()) {
            error = QStringLiteral("the share link contains an invalid address");
            return false;
        }
        QString host;
        quint16 port = 0;
        const QString addr = value.toString();
        if (!parseAddress(addr, host, port)) {
            error = QStringLiteral("the share link contains an invalid address");
            return false;
        }
        addresses.append(addr);
    }

    const QString project = obj.value(QStringLiteral("project")).toString();
    if (!validProjectName(project)) {
        error = QStringLiteral("the share link project name is invalid");
        return false;
    }

    out.fingerprint = key;
    out.addresses = addresses;
    out.project = project;
    error.clear();
    return true;
}
