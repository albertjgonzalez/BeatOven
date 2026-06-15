#ifndef IDENTITY_H
#define IDENTITY_H

#include <QString>
#include <QSslCertificate>
#include <QSslKey>

// The long-lived peer identity: an ECDSA P-256 keypair and a self-signed
// certificate wrapped around it. Generated once on first run, persisted under
// the per-user app data directory, and reused forever after.
//
// Trust model (important): the certificate is only a TLS-compatibility
// envelope. Peers trust each other via the SPKI SHA-256 *fingerprint* carried
// in the connection string and pinned at connect time (Phase 2) — never via
// certificate contents, names, or chains.
class Identity {
public:
    // Loads the persisted identity, generating and persisting one on first
    // run. On unrecoverable failure returns an invalid Identity and, if
    // errorOut is non-null, a human-readable reason.
    // Requires QCoreApplication::applicationName() to be set ("BeatOven").
    static Identity loadOrCreate(QString* errorOut = nullptr);

    bool isValid() const;
    QSslCertificate certificate() const;
    QSslKey privateKey() const;

    // This identity's SPKI SHA-256 fingerprint: 64 lowercase hex chars.
    QString fingerprint() const;

    // SECURITY-SENSITIVE: computes the SPKI SHA-256 fingerprint of any
    // certificate. This exact function is what the receiver runs against the
    // sharer's presented certificate during pin verification (Phase 2), so
    // generation and verification can never disagree on the definition.
    // Returns an empty string on any failure (callers must treat empty as
    // verification failure, never as a match).
    static QString fingerprintOf(const QSslCertificate& cert);

private:
    QSslCertificate mCert;
    QSslKey mKey;
};

#endif // IDENTITY_H
