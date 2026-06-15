#ifndef CONNECTIONSTRING_H
#define CONNECTIONSTRING_H

#include <QString>
#include <QStringList>

// Everything a receiver needs to fetch a shared project:
//   beatoven://share/<base64url(json)>
//   json = { "v":1, "key":"<spki-sha256 hex>", "addrs":[...], "project":"..." }
struct ShareInfo {
    QString fingerprint;    // sharer identity: 64 lowercase hex chars (SPKI SHA-256)
    QStringList addresses;  // dial order: "[2001:db8::1]:5000" then "203.0.113.9:5000" etc.
    QString project;        // the one project this link grants access to
};

// Builds the link. Assumes the fields are already valid (the sharer side
// constructs them from its own Identity and ShareSession).
QString generateConnectionString(const ShareInfo& info);

// SECURITY-SENSITIVE: parses and strictly validates an untrusted link (it
// arrives via a browser handoff from anywhere). Returns false with a
// user-displayable reason in `error`; `out` is only written on success.
bool parseConnectionString(QString url, ShareInfo& out, QString& error);

// Splits one validated address entry into host + port for the dialer.
// Accepts only literal IPs: "ipv4:port" or "[ipv6]:port". No hostnames in V1.
bool parseAddress(const QString& address, QString& hostOut, quint16& portOut);

// True if `name` is acceptable as a project name: non-empty, <=128 chars,
// no path separators, no "..", no control characters.
bool validProjectName(const QString& name);

#endif // CONNECTIONSTRING_H
