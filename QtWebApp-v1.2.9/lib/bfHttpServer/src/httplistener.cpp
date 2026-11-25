/**
  @file
  @author Stefan Frings
  Modified 2026-11-25 by Aurel: connections use qintptr, as QTcpServer does since Qt5.
*/

#include "httplistener.h"
#include "httpconnectionhandler.h"
#include "httpconnectionhandlerpool.h"
#include <QCoreApplication>

HttpListener::HttpListener(QSettings* settings, HttpRequestHandler* requestHandler, QObject *parent)
    : QTcpServer(parent), pool(settings,requestHandler)
{
    Q_ASSERT(settings!=0);
    this->settings=settings;
    // the descriptor reaches the handler thread through a queued signal
    qRegisterMetaType<qintptr>("qintptr");
    // Start listening
    int port=settings->value("port").toInt();
    listen(QHostAddress::Any, port);
    if (!isListening()) {
        qCritical("HttpListener: Cannot bind on port %i: %s",port,qPrintable(errorString()));
    }
    else {
        qDebug("HttpListener: Listening on port %i",port);
    }
}

HttpListener::~HttpListener() {
    close();
    qDebug("HttpListener: closed");
}


void HttpListener::incomingConnection(qintptr socketDescriptor) {
#ifdef SUPERVERBOSE
    qDebug("HttpListener: New connection");
#endif
    HttpConnectionHandler* freeHandler=pool.getConnectionHandler();

    // Let the handler process the new connection.
    if (freeHandler) {
        // The descriptor is passed via signal/slot because the handler lives in another
        // thread and cannot open the socket when called by another thread.
        connect(this,SIGNAL(handleConnection(qintptr)),freeHandler,SLOT(handleConnection(qintptr)));
        emit handleConnection(socketDescriptor);
        disconnect(this,SIGNAL(handleConnection(qintptr)),freeHandler,SLOT(handleConnection(qintptr)));
    }
    else {
        // Reject the connection
        qDebug("HttpListener: Too many connections");
        QTcpSocket* socket=new QTcpSocket(this);
        socket->setSocketDescriptor(socketDescriptor);
        connect(socket, SIGNAL(disconnected()), socket, SLOT(deleteLater()));
        socket->write("HTTP/1.1 503 too many connections\r\nConnection: close\r\n\r\nToo many connections\r\n");
        socket->disconnectFromHost();
    }
}
