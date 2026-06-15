// Phase 2 manual test harness. ONE executable, three modes:
//
//   BeatOvenPhase2Test listen
//       Starts a TLS listener on port 5000 using THIS machine's identity and
//       prints its own fingerprint. Leave it running.
//
//   BeatOvenPhase2Test dial <fingerprint>
//       Dials 127.0.0.1:5000 and verifies the peer against <fingerprint>.
//       Pass the fingerprint the listener printed -> expect SUCCESS.
//
//   BeatOvenPhase2Test dial-bad
//       Dials 127.0.0.1:5000 with a deliberately WRONG fingerprint ->
//       expect REFUSED (proves a man-in-the-middle is rejected).
//
// Two terminals:
//   term1:  BeatOvenPhase2Test listen
//   term2:  BeatOvenPhase2Test dial <fingerprint-from-term1>   -> SUCCESS
//   term2:  BeatOvenPhase2Test dial-bad                        -> REFUSED

#include "Identity.h"
#include "TlsConnection.h"
#include "TlsListener.h"

#include <QCoreApplication>
#include <iostream>

static const quint16 kPort = 5000;

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("BeatOven"));

    if (argc < 2) {
        std::cout << "usage: listen | dial <fingerprint> | dial-bad" << std::endl;
        return 2;
    }

    QString err;
    Identity identity = Identity::loadOrCreate(&err);
    if (!identity.isValid()) {
        std::cout << "identity error: " << err.toStdString() << std::endl;
        return 2;
    }

    const QString mode = QString::fromLatin1(argv[1]);

    if (mode == QStringLiteral("listen")) {
        std::cout << "MY FINGERPRINT: " << identity.fingerprint().toStdString()
                  << std::endl;
        std::cout << "(use this with: dial <fingerprint>)" << std::endl;

        auto* listener = new TlsListener(identity, &app);

        QObject::connect(listener, &TlsListener::connectionReady,
                         [](TlsConnection* conn) {
            std::cout << "LISTENER: a verified peer connected. Reading hello..."
                      << std::endl;
            if (conn->waitForReadyRead(5000)) {
                std::cout << "LISTENER: received: "
                          << conn->read(1024).toStdString() << std::endl;
                conn->write(QByteArray("hello back from sharer\n"));
                conn->waitForBytesWritten(2000);
            }
            std::cout << "LISTENER: exchange complete." << std::endl;
            delete conn;
        });

        if (!listener->listen(kPort)) {
            std::cout << "listen failed: "
                      << listener->errorString().toStdString() << std::endl;
            return 1;
        }
        std::cout << "LISTENER: waiting on port " << kPort
                  << " (Ctrl+C to stop)" << std::endl;
        return app.exec();
    }

    if (mode == QStringLiteral("dial") || mode == QStringLiteral("dial-bad")) {
        QString expected;
        if (mode == QStringLiteral("dial")) {
            if (argc < 3) {
                std::cout << "usage: dial <fingerprint>" << std::endl;
                return 2;
            }
            expected = QString::fromLatin1(argv[2]);
        } else {
            expected = QString(64, QLatin1Char('0'));  // cannot match a real id
            std::cout << "DIALER: using deliberately WRONG fingerprint; "
                         "expecting refusal." << std::endl;
        }

        const QStringList addrs {
            QStringLiteral("127.0.0.1:") + QString::number(kPort) };
        QString dialErr;
        TlsConnection* conn = dialPeer(addrs, expected, 5000, &dialErr);

        if (!conn) {
            std::cout << "DIALER: connection refused -> " << dialErr.toStdString()
                      << std::endl;
            return (mode == QStringLiteral("dial-bad")) ? 0 : 1;  // refusal = pass for dial-bad
        }

        if (mode == QStringLiteral("dial-bad")) {
            std::cout << "DIALER: ERROR - bad fingerprint was NOT refused!"
                      << std::endl;
            delete conn;
            return 1;
        }

        std::cout << "DIALER: verified peer "
                  << conn->peerFingerprint().toStdString() << std::endl;
        conn->write(QByteArray("hello from receiver\n"));
        conn->waitForBytesWritten(2000);
        if (conn->waitForReadyRead(5000))
            std::cout << "DIALER: received: "
                      << conn->read(1024).toStdString() << std::endl;
        std::cout << "DIALER: success." << std::endl;
        delete conn;
        return 0;
    }

    std::cout << "unknown mode: " << mode.toStdString() << std::endl;
    return 2;
}
