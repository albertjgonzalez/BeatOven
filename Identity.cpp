#include "Identity.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <iostream>

#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/rand.h>

namespace {

QString identityDirPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
    + QStringLiteral("/identity");
}

QByteArray bioToBytes(BIO* bio) {
    char* data = nullptr;
    const long len = BIO_get_mem_data(bio, &data);
    if (len <= 0 || !data)
        return {};
    return QByteArray(data, static_cast<int>(len));
}

// SECURITY-SENSITIVE: generates the long-lived peer identity.
// Standard OpenSSL goto-cleanup style: every resource is declared up top,
// freed exactly once at the bottom, so each error path is auditable.
bool generateIdentityPem(QByteArray& keyPemOut, QByteArray& certPemOut, QString& error) {
    bool ok = false;
    EVP_PKEY* pkey = nullptr;
    X509* cert = nullptr;
    BIO* keyBio = nullptr;
    BIO* certBio = nullptr;
    BIGNUM* serialBn = nullptr;

    // 1. ECDSA P-256 keypair (OpenSSL 1.1.1-compatible form).
    {
        EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);
        if (!pctx) { error = QStringLiteral("EVP_PKEY_CTX_new_id failed"); goto cleanup; }
        if (EVP_PKEY_keygen_init(pctx) != 1 ||
            EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx, NID_X9_62_prime256v1) != 1 ||
            EVP_PKEY_keygen(pctx, &pkey) != 1) {
            EVP_PKEY_CTX_free(pctx);
            error = QStringLiteral("EC keygen failed");
            goto cleanup;
        }
        EVP_PKEY_CTX_free(pctx);
    }

    // 2. A self-signed X.509v3 certificate around the key. The certificate is
    //    a TLS-compatibility envelope ONLY: trust comes from the pinned key
    //    fingerprint in the connection string, never from cert contents.
    cert = X509_new();
    if (!cert) { error = QStringLiteral("X509_new failed"); goto cleanup; }
    X509_set_version(cert, 2); // 2 == X.509v3

    {
        // Random positive 64-bit serial (some TLS stacks reject zero or
        // colliding serials; randomness costs nothing).
        unsigned char serialBytes[8];
        if (RAND_bytes(serialBytes, sizeof serialBytes) != 1) {
            error = QStringLiteral("RAND_bytes failed"); goto cleanup;
        }
        serialBytes[0] &= 0x7f; // keep it positive
        serialBn = BN_bin2bn(serialBytes, sizeof serialBytes, nullptr);
        if (!serialBn || !BN_to_ASN1_INTEGER(serialBn, X509_get_serialNumber(cert))) {
            error = QStringLiteral("serial number setup failed"); goto cleanup;
        }
    }

    // Validity: from 1 day in the past (tolerate peer clock skew) to
    // +25 years. Long validity is deliberate — the identity is meant to be
    // permanent, and trust is by pinned key, not by expiry policing.
    if (!X509_gmtime_adj(X509_getm_notBefore(cert), -60L * 60 * 24) ||
        !X509_gmtime_adj(X509_getm_notAfter(cert), 60L * 60 * 24 * 365 * 25)) {
        error = QStringLiteral("validity period setup failed"); goto cleanup;
    }

    if (X509_set_pubkey(cert, pkey) != 1) {
        error = QStringLiteral("X509_set_pubkey failed"); goto cleanup;
    }

    {
        // Subject == issuer (self-signed). The CN is purely cosmetic and
        // carries no trust whatsoever.
        X509_NAME* name = X509_get_subject_name(cert);
        X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                                   reinterpret_cast<const unsigned char*>("BeatOven Peer"), -1, -1, 0);
        X509_set_issuer_name(cert, name);
    }

    if (!X509_sign(cert, pkey, EVP_sha256())) {
        error = QStringLiteral("X509_sign failed"); goto cleanup;
    }

    // 3. PEM-encode both into memory. The private key is intentionally NOT
    //    passphrase-encrypted: the app must use it unattended at every
    //    launch, so a baked-in passphrase would add no security. Protection
    //    is the file location + permissions (see writePrivateFile).
    keyBio = BIO_new(BIO_s_mem());
    certBio = BIO_new(BIO_s_mem());
    if (!keyBio || !certBio) { error = QStringLiteral("BIO_new failed"); goto cleanup; }
    if (PEM_write_bio_PrivateKey(keyBio, pkey, nullptr, nullptr, 0, nullptr, nullptr) != 1) {
        error = QStringLiteral("PEM private key write failed"); goto cleanup;
    }
    if (PEM_write_bio_X509(certBio, cert) != 1) {
        error = QStringLiteral("PEM certificate write failed"); goto cleanup;
    }
    keyPemOut = bioToBytes(keyBio);
    certPemOut = bioToBytes(certBio);
    ok = !keyPemOut.isEmpty() && !certPemOut.isEmpty();
    if (!ok)
        error = QStringLiteral("PEM output was empty");

cleanup:
    BIO_free(keyBio);
    BIO_free(certBio);
    BN_free(serialBn);
    X509_free(cert);
    EVP_PKEY_free(pkey);
    return ok;
}

bool writePrivateFile(const QString& path, const QByteArray& data, QString& error) {
    QSaveFile file(path); // atomic: written to temp, renamed on commit
    if (!file.open(QIODevice::WriteOnly)) {
        error = QStringLiteral("could not open for writing: ") + path;
        return false;
    }
    if (file.write(data) != data.size() || !file.commit()) {
        error = QStringLiteral("could not write: ") + path;
        return false;
    }
    // Owner-only permissions. NOTE (flagged for review): on Windows,
    // QFile::setPermissions does not write NTFS ACLs — the real protection
    // there is that the file lives inside the user's own profile directory.
    QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    return true;
}

} // namespace

Identity Identity::loadOrCreate(QString* errorOut) {
    Identity id;
    QString err;

    const QString dir = identityDirPath();
    const QString keyPath = dir + QStringLiteral("/key.pem");
    const QString certPath = dir + QStringLiteral("/cert.pem");

    QByteArray keyPem;
    QByteArray certPem;
    const bool haveKey = QFile::exists(keyPath);
    const bool haveCert = QFile::exists(certPath);

    if (haveKey && haveCert) {
        QFile keyFile(keyPath);
        QFile certFile(certPath);
        if (!keyFile.open(QIODevice::ReadOnly) || !certFile.open(QIODevice::ReadOnly)) {
            err = QStringLiteral("identity exists but could not be read in ") + dir;
        } else {
            keyPem = keyFile.readAll();
            certPem = certFile.readAll();
        }
    } else if (haveKey != haveCert) {
        // Half an identity on disk. SECURITY-SENSITIVE decision: do NOT
        // silently regenerate — that would quietly change this install's
        // fingerprint. Surface it and let the user delete the directory
        // deliberately if they want a fresh identity.
        err = QStringLiteral("identity is corrupt (one of key.pem/cert.pem missing) in ") + dir;
    } else {
        std::cout << "Identity: first run, generating keypair" << std::endl;
        if (!QDir().mkpath(dir)) {
            err = QStringLiteral("could not create ") + dir;
        } else if (generateIdentityPem(keyPem, certPem, err)) {
            if (!writePrivateFile(keyPath, keyPem, err) ||
                !writePrivateFile(certPath, certPem, err)) {
                keyPem.clear(); // generated but not persisted -> treat as failure
            }
        }
    }

    if (err.isEmpty() && !keyPem.isEmpty()) {
        id.mKey = QSslKey(keyPem, QSsl::Ec, QSsl::Pem, QSsl::PrivateKey);
        id.mCert = QSslCertificate(certPem, QSsl::Pem);
        if (id.mKey.isNull())
            err = QStringLiteral("private key failed to parse: ") + keyPath;
        else if (id.mCert.isNull())
            err = QStringLiteral("certificate failed to parse: ") + certPath;
        else if (fingerprintOf(id.mCert).isEmpty())
            err = QStringLiteral("fingerprint computation failed");
    } else if (err.isEmpty()) {
        err = QStringLiteral("identity generation failed for an unknown reason");
    }

    if (!err.isEmpty()) {
        std::cout << "Identity ERROR: " << err.toStdString() << std::endl;
        if (errorOut)
            *errorOut = err;
        id.mKey.clear();
        id.mCert.clear();
        return id;
    }

    std::cout << "Identity fingerprint: " << id.fingerprint().toStdString() << std::endl;
    return id;
}

bool Identity::isValid() const {
    return !mCert.isNull() && !mKey.isNull();
}

QSslCertificate Identity::certificate() const {
    return mCert;
}

QSslKey Identity::privateKey() const {
    return mKey;
}

QString Identity::fingerprint() const {
    return fingerprintOf(mCert);
}

// SECURITY-SENSITIVE: the single definition of "fingerprint" for the whole
// app. SHA-256 over the DER-encoded SubjectPublicKeyInfo (SPKI) of the
// certificate — i.e. a hash of the public key itself, not of the whole
// certificate. That means the sharer could reissue a new cert around the same
// key (e.g. after expiry) without changing its identity, and an attacker
// cannot match the fingerprint without possessing the private key.
QString Identity::fingerprintOf(const QSslCertificate& cert) {
    if (cert.isNull())
        return {};

    const QByteArray der = cert.toDer();
    const unsigned char* derPtr = reinterpret_cast<const unsigned char*>(der.constData());
    X509* x = d2i_X509(nullptr, &derPtr, der.size());
    if (!x)
        return {};

    QString result;
    unsigned char* spkiDer = nullptr;
    // X509_get_X509_PUBKEY returns an internal pointer (not freed separately);
    // i2d_X509_PUBKEY allocates spkiDer, which we must OPENSSL_free.
    const int spkiLen = i2d_X509_PUBKEY(X509_get_X509_PUBKEY(x), &spkiDer);
    if (spkiLen > 0 && spkiDer) {
        unsigned char hash[32];
        unsigned int hashLen = 0;
        if (EVP_Digest(spkiDer, static_cast<size_t>(spkiLen),
                       hash, &hashLen, EVP_sha256(), nullptr) == 1 && hashLen == 32) {
            result = QString::fromLatin1(
                QByteArray(reinterpret_cast<const char*>(hash), 32).toHex());
        }
    }
    OPENSSL_free(spkiDer);
    X509_free(x);
    return result;
}
