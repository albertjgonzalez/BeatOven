#include "TlsListener.h"

#include <QSslConfiguration>
#include <iostream>

TlsListener::TlsListener(const Identity& identity, QObject* parent)
    : QObject(parent), mIdentity(identity) {

    mServer = new QSslServer(this);

    // Present our identity to every peer. The receiver pins this cert's key
    // fingerprint. We do not ask the receiver for one (anonymous in V1).
    QSslConfiguration cfg = mServer->sslConfiguration();
    cfg.setLocalCertificate(mIdentity.certificate());
    cfg.setPrivateKey(mIdentity.privateKey());
    cfg.setPeerVerifyMode(QSslSocket::VerifyNone);
    mServer->setSslConfiguration(cfg);

    // QSslServer fires this only AFTER the handshake completes, so every socket
    // we get here is already encrypted.
    connect(mServer, &QSslServer::pendingConnectionAvailable,
            this, &TlsListener::onPendingConnection);
}

bool TlsListener::listen(quint16 port) {
    // All interfaces (IPv4 + IPv6) so both address families in the string work.
    if (!mServer->listen(QHostAddress::Any, port)) {
        mError = mServer->errorString();
        std::cout << "TlsListener: listen failed: " << mError.toStdString()
                  << std::endl;
        return false;
    }
    std::cout << "TlsListener: listening on port " << mServer->serverPort()
              << std::endl;
    return true;
}

quint16 TlsListener::serverPort() const {
    return mServer ? mServer->serverPort() : 0;
}

QString TlsListener::errorString() const {
    return mError;
}

void TlsListener::onPendingConnection() {
    while (mServer->hasPendingConnections()) {
        auto* ssl = qobject_cast<QSslSocket*>(mServer->nextPendingConnection());
        if (!ssl) {
            std::cout << "TlsListener: dropping non-TLS socket" << std::endl;
            continue;
        }
        std::cout << "TlsListener: accepted encrypted connection from "
                  << ssl->peerAddress().toString().toStdString() << std::endl;
        ssl->setParent(nullptr);   // detach so it can move to a worker thread
        emit connectionReady(wrapAccepted(ssl));
    }
}
