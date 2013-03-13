/*
SIS - SIS Is Secure - safe instant messaging based on a non-central server architecture
Copyright (C) 2013 Benoît Vernier <vernier.benoit@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SIS_H
#define SIS_H

#include <QtGui>
#include <QtNetwork>
#include <phonon/phonon>
#include <QtCrypto/qca.h>
#include <unordered_map>
#include "qmessageedit.h"
#include "qmessagesbrowser.h"

using namespace std;

/*

 Packets will be :

 quint16    Type    QByteArray
 size       type    data

 */

struct datas {
    int tabId;
    QWidget* container;
    QMessagesBrowser* browser;
    QCA::SymmetricKey key;
    QCA::PublicKey pubKey;
    quint16 messageSize;
    QString nickname;
};

class SIS : public QMainWindow
{
  Q_OBJECT

enum Type
{
    givePubK, replyPubK, giveBF, replyBF, text, giveNick, replyNick
};

public:
  SIS(QWidget *parent = 0);

private:
  QMessagesBrowser* state;
  QPushButton* button;

  QTabsWidget* window;
  QTabBar* tabBar;

  QSettings* settings;
  QString nickname;

  QCA::Initializer init;

  QCA::PrivateKey privateKey;
  QCA::PublicKey publicKey;

  QTcpServer* server;

  unordered_map<QMessageEdit*, QTcpSocket*> edit_socket;
  unordered_map<QTcpSocket*, QMessageEdit*> socket_edit;

  unordered_map<QTcpSocket*, datas> networkMap;
  map<int, pair<QMessageEdit*, QTcpSocket*> > tabMap;

  void openTab(QTcpSocket* socket);
  void reOpenTab(QTcpSocket* socket);
  void closeAllTabs();

private slots:
  void transfer();
  void newConversation();
  void dataReceived();
  void requestNewConnection();
  void connected();
  void disconnected();
  void clearColor(int);
  void closeTab(int);
  void moveTab(int from, int to);
};

#endif // SIS_H
