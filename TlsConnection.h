#ifndef TLSCONNECTION_H
#define TLSCONNECTION_H

#include "Connection.h"
#include <QSslSocket>

// The single V1 Connection implementation: a TLS stream over QSslSocket.
//
// Two free functions produce one (declared at the bottom of this header):
//   dialPeer()   - RECEIVER side: connect out + verify the peer's certificate
//                  against the pinned fingerprint before returning anything.
//   wrapAccepted() - SHARER side: wrap a socket TlsListener already handshook.
//
// The class itself is a thin pass-through to the socket. Ownership is plain:
// TlsConnection owns its socket and deletes it.
class TlsConnection : public Connection {
public:
    // Takes ownership of an already-connected socket. peerFingerprint is the
    // verified remote identity ("" if the remote was not verified, i.e. the
    // sharer side where the receiver is anonymous in V1).
    TlsConnection(QSslSocket* socket, QString peerFingerprint);
    ~TlsConnection() override;

    qint64 write(const QByteArray& data) override;
    QByteArray read(qint64 maxBytes) override;
    bool waitForReadyRead(int msecs) override;
    bool waitForBytesWritten(int msecs) override;
    void close() override;
    bool isOpen() const override;
    QString peerFingerprint() const override;

private:
    QSslSocket* mSocket = nullptr;
    QString mPeerFingerprint;
};

// RECEIVER side. Tries each address in order (IPv6 first, per the connection
// string). For each: TCP connect -> TLS handshake -> pin check. Returns a
// verified, open TlsConnection on first success, or nullptr if all failed
// (with the last error in errorOut). The caller owns the returned pointer.
//
// SECURITY: no application byte is written before the pin check passes; a
// fingerprint mismatch refuses that address.
TlsConnection* dialPeer(const QStringList& addresses,
                        const QString& expectedFingerprint,
                        int perAddressTimeoutMs,
                        QString* errorOut);

// SHARER side. Wraps a socket whose handshake TlsListener already completed.
TlsConnection* wrapAccepted(QSslSocket* socket);

#endif // TLSCONNECTION_H
