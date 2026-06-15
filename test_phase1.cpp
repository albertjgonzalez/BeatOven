// Phase 1 test harness. Builds as a separate console executable
// (BeatOvenPhase1Test). Exit code 0 = all tests passed.
#include "Identity.h"
#include "ConnectionString.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <iostream>

static int gFailures = 0;

static void check(bool condition, const char* label) {
    std::cout << (condition ? "PASS  " : "FAIL  ") << label << std::endl;
    if (!condition)
        ++gFailures;
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    // Must match main.cpp so the test exercises the SAME identity location
    // the real app uses.
    QCoreApplication::setApplicationName(QStringLiteral("BeatOven"));

    std::cout << "--- Identity ---" << std::endl;

    Identity first = Identity::loadOrCreate();
    check(first.isValid(), "identity loads or generates");
    check(first.fingerprint().size() == 64, "fingerprint is 64 chars");

    bool allHex = !first.fingerprint().isEmpty();
    for (const QChar c : first.fingerprint()) {
        const char16_t u = c.unicode();
        if (!((u >= u'0' && u <= u'9') || (u >= u'a' && u <= u'f')))
            allHex = false;
    }
    check(allHex, "fingerprint is lowercase hex");

    Identity second = Identity::loadOrCreate();
    check(second.isValid() && second.fingerprint() == first.fingerprint(),
          "identity persists across loads (same fingerprint)");
    check(Identity::fingerprintOf(first.certificate()) == first.fingerprint(),
          "fingerprintOf(certificate) matches identity fingerprint");

    std::cout << "--- ConnectionString round trip ---" << std::endl;

    ShareInfo in;
    in.fingerprint = first.isValid() ? first.fingerprint() : QString(64, QLatin1Char('a'));
    in.addresses = { QStringLiteral("[2001:db8::1]:5000"),
                    QStringLiteral("203.0.113.9:5000"),
                    QStringLiteral("192.168.1.42:5000") };
    in.project = QStringLiteral("my-track");

    const QString url = generateConnectionString(in);
    std::cout << "sample link: " << url.toStdString() << std::endl;

    ShareInfo parsed;
    QString error;
    check(parseConnectionString(url, parsed, error), "generated link parses");
    check(parsed.fingerprint == in.fingerprint, "fingerprint survives round trip");
    check(parsed.addresses == in.addresses, "addresses survive round trip in order");
    check(parsed.project == in.project, "project survives round trip");

    QString host;
    quint16 port = 0;
    check(parseAddress(QStringLiteral("203.0.113.9:5000"), host, port)
              && host == QStringLiteral("203.0.113.9") && port == 5000,
          "ipv4 address splits into host/port");
    check(parseAddress(QStringLiteral("[2001:db8::1]:5000"), host, port)
              && host == QStringLiteral("2001:db8::1") && port == 5000,
          "ipv6 address splits into host/port");

    std::cout << "--- ConnectionString rejection of bad input ---" << std::endl;

    // Builds an otherwise-valid payload with one field replaced.
    const auto tamper = [&in](const char* field, const QJsonValue& value) {
        QJsonObject obj;
        obj.insert(QStringLiteral("v"), 1);
        obj.insert(QStringLiteral("key"), in.fingerprint);
        QJsonArray addrs;
        for (const QString& a : in.addresses)
            addrs.append(a);
        obj.insert(QStringLiteral("addrs"), addrs);
        obj.insert(QStringLiteral("project"), in.project);
        obj.insert(QLatin1String(field), value);
        const QByteArray json = QJsonDocument(obj).toJson(QJsonDocument::Compact);
        return QStringLiteral("beatoven://share/") + QString::fromLatin1(
                   json.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
    };

    ShareInfo dummy;
    check(!parseConnectionString(QStringLiteral("https://example.com/x"), dummy, error),
          "rejects wrong scheme");
    check(!parseConnectionString(QStringLiteral("beatoven://share/!!!not-base64!!!"), dummy, error),
          "rejects invalid base64url payload");
    check(!parseConnectionString(QStringLiteral("beatoven://share/"), dummy, error),
          "rejects empty payload");
    check(!parseConnectionString(tamper("v", 2), dummy, error),
          "rejects newer version number");
    check(!parseConnectionString(tamper("key", QStringLiteral("not-a-fingerprint")), dummy, error),
          "rejects malformed fingerprint");
    check(!parseConnectionString(tamper("key", in.fingerprint.toUpper()), dummy, error),
          "rejects uppercase fingerprint (one canonical form)");
    check(!parseConnectionString(tamper("addrs", QJsonArray()), dummy, error),
          "rejects empty address list");
    check(!parseConnectionString(tamper("addrs", QJsonArray{ QStringLiteral("evil.example.com:5000") }), dummy, error),
          "rejects hostnames (IP literals only in V1)");
    check(!parseConnectionString(tamper("addrs", QJsonArray{ QStringLiteral("203.0.113.9:99999") }), dummy, error),
          "rejects out-of-range port");
    check(!parseConnectionString(tamper("addrs", QJsonArray{ QStringLiteral("203.0.113.9") }), dummy, error),
          "rejects address without port");
    check(!parseConnectionString(tamper("project", QStringLiteral("../../etc/passwd")), dummy, error),
          "rejects path traversal in project name");
    check(!parseConnectionString(tamper("project", QStringLiteral("a/b")), dummy, error),
          "rejects slash in project name");
    check(!parseConnectionString(tamper("project", QStringLiteral("a\\b")), dummy, error),
          "rejects backslash in project name");
    check(!parseConnectionString(tamper("project", QString()), dummy, error),
          "rejects empty project name");

    check(validProjectName(QStringLiteral("My Track 27")), "accepts a normal project name");
    check(validProjectName(QStringLiteral("27$$")), "accepts symbols that are fine in filenames");

    std::cout << std::endl
              << (gFailures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED")
              << " (" << gFailures << " failures)" << std::endl;
    return gFailures == 0 ? 0 : 1;
}
