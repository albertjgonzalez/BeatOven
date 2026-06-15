#include "TlsConnection.h"
#include "ConnectionString.h"
#include "Identity.h"

#include <QSslConfiguration>
#include <iostream>

TlsConnection::TlsConnection(QSslSocket* socket, QString peerFingerprint)
    : mSocket(socket), mPeerFingerprint(std::move(peerFingerprint)) {}

TlsConnection::~TlsConnection() {
    if (mSocket) {
        mSocket->abort();
        delete mSocket;
    }
}

qint64 TlsConnection::write(const QByteArray& data) {
    return mSocket ? mSocket->write(data) : -1;
}

QByteArray TlsConnection::read(qint64 maxBytes) {
    return mSocket ? mSocket->read(maxBytes) : QByteArray();
}

bool TlsConnection::waitForReadyRead(int msecs) {
    return mSocket && mSocket->waitForReadyRead(msecs);
}

bool TlsConnection::waitForBytesWritten(int msecs) {
    return mSocket && mSocket->waitForBytesWritten(msecs);
}

void TlsConnection::close() {
    if (mSocket)
        mSocket->disconnectFromHost();
}

bool TlsConnection::isOpen() const {
    return mSocket && mSocket->state() == QAbstractSocket::ConnectedState;
}

QString TlsConnection::peerFingerprint() const {
    return mPeerFingerprint;
}

// ===========================================================================
// SECURITY-CRITICAL: receiver-side dial with certificate pinning.
//
// The rule, stated once:
//   Trust the peer if and only if the SPKI-SHA-256 fingerprint of the
//   certificate it actually presented equals the fingerprint from the
//   connection string. Nothing else confers trust: not a valid chain, not a
//   matching hostname, not the absence of TLS errors. The self-signed cert has
//   no authority behind it by design.
//
// Why QueryPeer and not VerifyPeer:
//   VerifyPeer makes Qt abort the handshake on the self-signed error before we
//   ever see the certificate. QueryPeer completes the handshake and lets US
//   judge by fingerprint. Because Qt is therefore NOT validating the chain,
//   our fingerprint check below IS the entire authentication -- there is no
//   second line of defense, which is why this is the function to scrutinize.
//
// FLAGGED FOR REVIEW: the pin compare, the QueryPeer choice, and the
// "no bytes before verification" ordering must stay exactly as written.
// ===========================================================================
static QSslSocket* connectAndVerify(const QString& host, quint16 port,
                                    const QString& expectedFingerprint,
                                    int timeoutMs, QString& error) {
    auto* socket = new QSslSocket();

    // Do not let Qt reject the self-signed cert during the handshake; we judge
    // identity ourselves immediately after, by fingerprint.
    QSslConfiguration cfg = socket->sslConfiguration();
    cfg.setPeerVerifyMode(QSslSocket::QueryPeer);
    socket->setSslConfiguration(cfg);

    socket->connectToHostEncrypted(host, port);

    // One wait covers TCP connect + TLS handshake.
    if (!socket->waitForEncrypted(timeoutMs)) {
        error = QStringLiteral("handshake to %1:%2 failed: %3")
                    .arg(host).arg(port).arg(socket->errorString());
        delete socket;
        return nullptr;
    }

    const QSslCertificate presented = socket->peerCertificate();
    if (presented.isNull()) {
        error = QStringLiteral("peer presented no certificate");
        socket->abort();
        delete socket;
        return nullptr;
    }

    // Same function used to generate our own fingerprint, so the two sides can
    // never disagree on what "fingerprint" means. Empty = cannot verify = refuse.
    const QString actual = Identity::fingerprintOf(presented);
    if (actual.isEmpty() || actual != expectedFingerprint) {
        error = actual.isEmpty()
            ? QStringLiteral("could not compute peer fingerprint")
            : QStringLiteral("FINGERPRINT MISMATCH: expected %1 but peer is %2")
                  .arg(expectedFingerprint, actual);
        std::cout << "TlsConnection: " << error.toStdString() << std::endl;
        socket->abort();   // refuse: possible man-in-the-middle
        delete socket;
        return nullptr;
    }

    std::cout << "TlsConnection: peer verified (" << actual.toStdString() << ")"
              << std::endl;
    return socket;  // verified and open
}

TlsConnection* dialPeer(const QStringList& addresses,
                        const QString& expectedFingerprint,
                        int perAddressTimeoutMs,
                        QString* errorOut) {
    QString lastError = QStringLiteral("no addresses to try");

    for (const QString& addr : addresses) {
        QString host;
        quint16 port = 0;
        if (!parseAddress(addr, host, port)) {
            lastError = QStringLiteral("skipping unparseable address: ") + addr;
            continue;
        }
        std::cout << "TlsConnection: dialing " << addr.toStdString() << std::endl;
        QString err;
        QSslSocket* socket = connectAndVerify(host, port, expectedFingerprint,
                                              perAddressTimeoutMs, err);
        if (socket)
            return new TlsConnection(socket, expectedFingerprint);
        lastError = err;   // try next address (e.g. IPv6 failed -> IPv4)
    }

    if (errorOut)
        *errorOut = lastError;
    return nullptr;
}

TlsConnection* wrapAccepted(QSslSocket* socket) {
    // Sharer side: receiver is anonymous in V1, so no peer fingerprint.
    return new TlsConnection(socket, QString());
}
