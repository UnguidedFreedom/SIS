#ifndef SIS_H
#define SIS_H

#include <QtGui>
#include <QtNetwork>
#include <QtCrypto/qca.h>
#include <unordered_map>
#include "qmessageedit.h"

using namespace std;

/*

 Packets will be :

 quint16    Type    QByteArray
 size       type    data

 */

struct datas {
    int tabId;
    QWidget* container;
    QTextBrowser* browser;
    QCA::SymmetricKey key;
    QCA::PublicKey pubKey;
};

class SIS : public QMainWindow
{
  Q_OBJECT

enum Type
{
    givePubK, replyPubK, giveBF, replyBF, text
};

public:
  SIS(QWidget *parent = 0);

private:
  QTextBrowser* state;
  QPushButton* button;

  QTabsWidget* window;
  QTabBar* tabBar;

  QCA::Initializer init;

  QCA::PrivateKey privateKey;
  QCA::PublicKey publicKey;

  QTcpServer* server;
  quint16 messageSize;

  unordered_map<QMessageEdit*, QTcpSocket*> edit_socket;
  unordered_map<QTcpSocket*, QMessageEdit*> socket_edit;

  unordered_map<QTcpSocket*, datas> networkMap; // int = tabIndex
  unordered_map<int, pair<QMessageEdit*, QTcpSocket*> > tabMap;

  void openTab(QTcpSocket* socket);
private slots:
  void transfer();
  void newConversation();
  void dataReceived();
  void requestNewConnection();
  void connected();
  void disconnected();
  void clearColor(int);
};

#endif // SIS_H
