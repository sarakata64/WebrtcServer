
#include <QCoreApplication>
#include <webrtcserver.h>
int main(int argc, char *argv[])
{
  QCoreApplication a(argc, argv);
  server->listen();
  return a.exec();
}
