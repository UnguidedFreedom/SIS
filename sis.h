#ifndef SIS_H
#define SIS_H

#include <QtGui>
#include <QtNetwork>
#include <QtCrypto/qca.h>
#include <unordered_map>

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

class QTabsWidget : public QTabWidget
{
    Q_OBJECT
public:
    QTabsWidget(QWidget *parent = 0) {}
    ~QTabsWidget() {}

    QTabBar* tabBar() const { return QTabWidget::tabBar(); }
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

  unordered_map<QLineEdit*, QTcpSocket*> edit_socket;
  unordered_map<QTcpSocket*, QLineEdit*> socket_edit;

  unordered_map<QTcpSocket*, datas> networkMap; // int = tabIndex
  unordered_map<int, pair<QLineEdit*, QTcpSocket*> > tabMap;

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
