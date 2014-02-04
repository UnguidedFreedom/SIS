/*
SIS - SIS Is Secure - safe instant messaging based on a non-central server architecture
Copyright (C) 2013-2014 Benoît Vernier <vernier.benoit@gmail.com>

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

#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/bio.h>

#include <map>
#include "contact.h"
#include "messageedit.h"
#include "messagesbrowser.h"
#include "conversation.h"

using namespace std;

struct datas {
    int tabId;
    QWidget* container;
    MessagesBrowser* browser;
    unsigned char* key;
    Contact contact;
    quint16 messageSize;
};

class SIS : public QMainWindow
{
  Q_OBJECT

enum Type
{
    text, giveAES, replyAES, giveNick, replyNick, givePubK, replyPubK
};

public:
  SIS(QWidget *parent = 0);

private:
  MessagesBrowser* state;
  QPushButton* button;

  Conversation* window;

  QSettings* settings;
  QString nickname;

  RSA* keys;

  QTcpServer* server;
  int port;

  map<MessageEdit*, QTcpSocket*> edit_socket;
  map<QTcpSocket*, MessageEdit*> socket_edit;

  map<QTcpSocket*, datas> networkMap;
  map<int, pair<MessageEdit*, QTcpSocket*> > tabMap;

  int seed_prng(int);
  void openTab(QTcpSocket* socket);
  void reOpenTab(QTcpSocket* socket);


private slots:
  void transfer();
  void newConversation();
  void dataReceived();
  void requestNewConnection();
  void connected();
  void disconnected();
  void closeTab(int);
  void moveTab(int from, int to);
  void closeAllTabs();
};

#endif // SIS_H
