#ifndef WEBRTCSERVER_H
#define WEBRTCSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QSslError>
#include <QQueue>
class QUdpSocket;
class QSslSocket;
class QHostAddress;


#if defined(server)
#undef server
#endif
#define server                                                    \
  (static_cast< WebrtcServer *>( WebrtcServer::instance()))



class WebrtcServer : public QTcpServer
{
  Q_OBJECT
public:
  static WebrtcServer *instance();

  void listen();
  void send(QTcpSocket *socket, const QVariant &data, int delay = 300);

protected:
  void setUpAndListenForSocket();
  void tcpSend(QVariant msg, QTcpSocket *sock);
  QByteArray streamOut(const QVariant &data);

  explicit WebrtcServer(QObject *parent = nullptr);
  QVariant receive(QTcpSocket *socket);
protected slots:
  void readDatagrams();
  void connectToSocket();
  void handleRequest();
 void showErrors(const QList<QSslError> &);
private:
  QUdpSocket *m_udpListener;
  QList<QSslSocket*> mNewConnections;
  QSslSocket *childSocket;
  QHash<int, QTcpSocket *> peers;
  QList<QTcpSocket *> connectionSockets;
};

#define SEND_MESSAGE(socket, data) server->send(socket, data);

#endif // WEBRTCSERVER_H
