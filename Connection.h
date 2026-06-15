#ifndef CONNECTION_H
#define CONNECTION_H

#include <QByteArray>
#include <QString>

// An established, encrypted, bidirectional byte stream between two verified
// peers. Transfer logic talks ONLY to this interface and never knows how the
// connection was produced.
//
// V1 has one producer: TlsConnection (direct dial / direct listen over TLS).
// V2 adds more producers (relay, hole-punched) without transfer code changing.
//
// Style: blocking, matching the existing worker-thread idiom (workers live on
// their own QThread and call waitFor* directly).
class Connection {
public:
    virtual ~Connection() = default;

    // Write all of `data`. Returns bytes written, or -1 on error.
    virtual qint64 write(const QByteArray& data) = 0;

    // Read up to `maxBytes` currently-available bytes (may return fewer).
    virtual QByteArray read(qint64 maxBytes) = 0;

    // Block until at least one byte is readable or `msecs` elapses.
    virtual bool waitForReadyRead(int msecs) = 0;

    // Block until the write buffer is flushed to the socket or `msecs` elapses.
    virtual bool waitForBytesWritten(int msecs) = 0;

    virtual void close() = 0;
    virtual bool isOpen() const = 0;

    // The verified SPKI-SHA-256 fingerprint of the remote peer. Only non-empty
    // once identity has been confirmed (for TlsConnection, after the pin check
    // passes). Empty means "not verified" and must never be treated as trusted.
    virtual QString peerFingerprint() const = 0;
};

#endif // CONNECTION_H
