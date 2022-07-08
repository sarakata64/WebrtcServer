#include "webrtcserver.h"
#include <QTcpServer>
#include <QUdpSocket>
#include <QSslSocket>
#include <QDataStream>
#include <QFile>
#include <QSslKey>
#include <QEventLoop>
#include <QTimer>
#include <QSslConfiguration>

#define GW_SERVER_UDP_PORT 5500
#define GW_SERVER_TCP_PORT 5939
#define GW_SERVER_LOOKUP_PORT 6060
QByteArray wrapString(QString message) {

  QByteArray block;
  QDataStream out(&block, QIODevice::WriteOnly);
  out << QVariant(message);

  return block;
}
QString readBroadcast(QUdpSocket *sock, QHostAddress *senderPtr,
                      bool readFromLocal) {
  if (!sock)
    return QString();

  QHostAddress sender;
  QString data;

  while (sock->hasPendingDatagrams()) {
    QByteArray datagram;
    datagram.resize(sock->pendingDatagramSize());
    if (sock->readDatagram(datagram.data(), datagram.size(), &sender) == -1) {
      qDebug() << sock->errorString();
      continue;
    }

    if (readFromLocal) {
      QDataStream in(&datagram, QIODevice::ReadOnly);


      QVariant msg;
      in >> msg;
      data += msg.toString();

    }


  }
  if (senderPtr)
    *senderPtr = sender;
  return data;
}

void broadcast(QString msg, int port) {
  QUdpSocket socket;
  if (socket.writeDatagram(wrapString(msg), QHostAddress::Broadcast, port) ==
      -1) {
    qDebug() << socket.errorString();
    return;
  }
  socket.close();
}
WebrtcServer::WebrtcServer(QObject *parent) : QTcpServer(parent),m_udpListener(new QUdpSocket),childSocket(Q_NULLPTR)
{

}

WebrtcServer *WebrtcServer::instance() {
  static WebrtcServer *mgr;
  if (!mgr)
    mgr = new WebrtcServer;

  return mgr;
}

void WebrtcServer::listen() {
  bool b =
      m_udpListener->bind(GW_SERVER_UDP_PORT, QUdpSocket::DontShareAddress);
  qDebug() << "sdfsd  " << b;
  int i = 0;
  while (b && (i % 1000 != 0)) {
    qDebug() << "Connecting . . . ";
    i++;
  }
  qDebug() << "is  " << (i % 1000);
  setUpAndListenForSocket();
}

void WebrtcServer::send(QTcpSocket *socket, const QVariant &data, int delay) {

  if (!socket) {
    qDebug() << "Cannot send, Socket is null ";
    return;
  }
  // some times the message is not recieved , hence pause for 3000(default)
//  QEventLoop loop;
//  QTimer::singleShot(delay, &loop, &QEventLoop::quit);
//  loop.exec();
  tcpSend(data, socket);
  qDebug() << "is sent ";
}


void WebrtcServer::setUpAndListenForSocket() {
  connect(m_udpListener, SIGNAL(readyRead()), this, SLOT(readDatagrams()));

  if (!QTcpServer::listen(QHostAddress::Any, GW_SERVER_TCP_PORT)) {
    qDebug() << "Failed to listen ";
  } else {
    qDebug() << "ready ";
    connect(this, SIGNAL(newConnection()), this, SLOT(connectToSocket()));
  }
}
void WebrtcServer::tcpSend(QVariant msg, QTcpSocket *tcpSocket) {

  if (tcpSocket) {

    if (!tcpSocket->write(streamOut(msg))) {
      qDebug() << tcpSocket->errorString();
      return;
    }
  }
}

QByteArray WebrtcServer::streamOut(const QVariant &data) {
  QByteArray bytedata = QByteArray();
  QDataStream outStream(&bytedata, QIODevice::WriteOnly);
  outStream << QVariant(data);
  return bytedata;
}


void WebrtcServer::connectToSocket() {
  qDebug() << "is connect ";

  childSocket = reinterpret_cast<QSslSocket *>(nextPendingConnection());
  qDebug() << "child socket  " << childSocket;
  if (childSocket) {
    qDebug() << "Success = = = = = = =  ";
    connect(childSocket, &QSslSocket::readyRead, this,
            &WebrtcServer::handleRequest);

    connect(childSocket, &QSslSocket::disconnected, this, [this]() {
      qDebug() << "Socket Disconnected "<<childSocket;

      connectionSockets.clear();

      childSocket->deleteLater();

    });


    QVariantMap data;
    data["type"] = 0;
    data["status"] = "success";
    connectionSockets.append(childSocket);


    ;
  }
}

QVariant WebrtcServer::receive(QTcpSocket *socket) {
  QVariant m_data;
  if (socket) {
    QDataStream in(socket);
    in >> m_data;
  }
  return m_data;
}

void WebrtcServer::readDatagrams() {
  qDebug() << "is success ";
  readBroadcast(m_udpListener, 0, true);
  broadcast("Pong", GW_SERVER_LOOKUP_PORT);
}

void WebrtcServer::handleRequest() {

  QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
  QVariant data = receive(socket);

  auto dataAsMap = data.toMap();
  qDebug() << "Handle  "<<dataAsMap;
  int type = dataAsMap.value("type").toInt();
  qDebug() << "type is  " << type;
  switch (type) {
  case 1: {
    // login
  //  qDebug() << "is logginf idn  ";
    int peer = dataAsMap.value("peer").toInt();
    int id = connectionSockets.indexOf(socket);
    QVariantMap response;
    response["type"] = type;
    response["loginId"] = id;
    response["peer"] = peer;


    peers[id] = socket;


    response["response"] = 0;

    send(socket, response);


    break;
  }
  case 2: {
    // qDebug() << "Seindinf offer " << dataAsMap;
    auto partherId = dataAsMap.value("partnerId").toInt();

    auto socket_ = connectionSockets.at(partherId);
    send(socket_, dataAsMap);
    //  offer["partnerId"] = partnerId();
    //  offer["type"] = 2; // offer
    //  offer["sdpType"] = static_cast<int>(desc->GetType());
    //  offer["offer"] = QByteArray(cp_str.c_str());

  } break;
  case 3: {
    qDebug() << "Seinding answer";
    auto partherId = dataAsMap.value("partnerId").toInt();
    qDebug() << "partner ";
    auto socket_ = connectionSockets.at(partherId);
    send(socket_, dataAsMap);
    break;
  }
  case 4: {
    qDebug() << "Sending candidates " << dataAsMap;
    auto partherId = dataAsMap.value("partnerId").toInt();
    qDebug() << "partner ";
    auto socket_ = connectionSockets.at(partherId);
    send(socket_, dataAsMap);
    break;
  }
  default:
    qDebug() << "is daulef ";
     send(socket, data);

  }

}

void WebrtcServer::showErrors(const QList<QSslError> &l) {

}

