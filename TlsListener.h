#ifndef TLSLISTENER_H
#define TLSLISTENER_H

#include "TlsConnection.h"
#include "Identity.h"
#include <QObject>
#include <QSslServer>

// SHARER side. Listens for incoming TLS connections, presenting this install's
// identity certificate + key so the receiver can pin it. Each handshaken
// socket is wrapped and delivered via the connectionReady signal.
//
// This is a QObject because that is how Qt sockets deliver events -- not an
// abstraction layer, just the framework's shape.
class TlsListener : public QObject {
    Q_OBJECT
public:
    explicit TlsListener(const Identity& identity, QObject* parent = nullptr);

    // Listen on the given port (0 = OS-assigned). false + errorString() on fail.
    bool listen(quint16 port);
    quint16 serverPort() const;
    QString errorString() const;

signals:
    // One per accepted, handshaken connection. The slot takes ownership.
    void connectionReady(TlsConnection* connection);

private slots:
    void onPendingConnection();

private:
    QSslServer* mServer = nullptr;
    Identity mIdentity;
    QString mError;
};

#endif // TLSLISTENER_H
