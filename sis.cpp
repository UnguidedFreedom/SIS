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

#include "sis.h"

SIS::SIS(QWidget *parent)
  : QMainWindow(parent)
{
    settings = new QSettings(QSettings::UserScope, qApp->organizationName(), qApp->applicationName());

    state = new QMessagesBrowser;

    button = new QPushButton("Connect to host");
    connect(button, SIGNAL(pressed()), this, SLOT(requestNewConnection()));
    button->setEnabled(false);

    QWidget* container = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout;
    layout->addWidget(button);
    layout->addWidget(state);
    container->setLayout(layout);
    setCentralWidget(container);
    setWindowTitle("SIS Messaging");

    window = new QWindow(this);
    window->setSettings(settings);
    connect(window, SIGNAL(tabMoved(int,int)), this, SLOT(moveTab(int,int)));
    connect(window, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));
    connect(window, SIGNAL(closed()), this, SLOT(closeAllTabs()));

    window->setGeometry(settings->value("dimensions", QRect(2*QApplication::desktop()->width()/5, QApplication::desktop()->height()/4, QApplication::desktop()->width()/5, QApplication::desktop()->height()/2)).toRect());

    // Initializing
    init = QCA::Initializer();

    //Checking if RSA is supported
    if(!QCA::isSupported("pkey"))
    {
        state->setText("pkey not supported");
        QString info;
        QCA::scanForPlugins();

        // this gives us all the plugin providers as a list
        QCA::ProviderList qcaProviders = QCA::providers();
        for ( int i = 0; i < qcaProviders.size(); ++i )
        {
            // each provider has a name, which we can display
            info += qcaProviders[i]->name() + ": ";
            // ... and also a list of features
            QStringList capabilities = qcaProviders[i]->features();
            // we turn the string list back into a single string,
            // and display it as well
            info += capabilities.join(", ") + "\n";
        }

        // Note that the default provider isn't included in
        // the result of QCA::providers()
        info += "default: ";
        // However it is still possible to get the features
        // supported by the default provider
        QStringList capabilities = QCA::defaultFeatures();
        info += capabilities.join(", ") + "\n";
        state->append(info);

        return;
    }
    if(!QCA::PKey::supportedIOTypes().contains(QCA::PKey::RSA))
    {
        state->setText("RSA not supported");
        return;
    }

    QElapsedTimer* timer = new QElapsedTimer;
    timer->start();

    QString privateKeyTmp = settings->value("privateKey", "").toString();

    if(privateKeyTmp.trimmed() == "")
    {

        //Generating the private key
        privateKey = QCA::KeyGenerator().createRSA(4096);
        if(privateKey.isNull())
        {
            state->setText("Failed to generate secret key");
            qDebug() << "Failed to generate secret key";
            return;
        }

        QString passOne, passTwo;
        do {
            bool ok;
            passOne = QInputDialog::getText(this, "Sign up", "Enter your password to sign up:", QLineEdit::Password, "", &ok);
            if(!ok)
            {
                state->setText("No password given");
                return;
            }
            passTwo = QInputDialog::getText(this, "Sign up", "Confirm your password to sign up:", QLineEdit::Password, "", &ok);
            if(!ok)
            {
                state->setText("No password given");
                return;
            }
        } while(passOne != passTwo || passOne.trimmed().size() == 0);

        if(passOne.trimmed().size() != 0)
        {
            //Saving the private key to the settings
            QCA::SecureArray passPhrase = passOne.toUtf8();
            privateKeyTmp = privateKey.toPEM(passPhrase);
            settings->setValue("privateKey", privateKeyTmp);
        }
        else
        {
            state->setText("No password given");
            return;
        }
    }
    else
    {
        //Reading the private key from the settings
        QCA::ConvertResult conversionResult;
        QString pass = QInputDialog::getText(this, "Sign in", "Enter your password to sign in:", QLineEdit::Password);
        privateKey = QCA::PrivateKey::fromPEM(privateKeyTmp, pass.toUtf8(), &conversionResult);
        if(! (QCA::ConvertGood == conversionResult) )
        {
            state->setText("Unable to read key from file");
            return;
        }
    }

    publicKey = privateKey.toPublicKey();

    //Checking the validity of the public key
    if(!publicKey.canEncrypt()) {
        state->setText("This kind of key cannot encrypt");
        qDebug() << "Error: this kind of key cannot encrypt";
        return;
    }

    nickname = settings->value("nickname", "").toString();

    while(nickname.trimmed() == "")
    {
        bool ok;
        nickname = QInputDialog::getText(this, "Pick a nickname", "Enter your nickname:", QLineEdit::Normal, "", &ok);
        if(!ok)
        {
            state->setText("No nickname given");
            return;
        }
    }

    settings->setValue("nickname", nickname.trimmed());

    server = new QTcpServer;
    if(!server->listen(QHostAddress::Any, 50000))
    {
        state->setText("Unable to start network connection");
        qDebug() << "Unable to start network connection";
        return;
    }

    connect(server, SIGNAL(newConnection()), this, SLOT(newConversation()));

    state->setText(QString::number(timer->elapsed()) + " ms to launch program");
    button->setEnabled(true);
    QString info;
    QCA::scanForPlugins();

    // this gives us all the plugin providers as a list
    QCA::ProviderList qcaProviders = QCA::providers();
    for ( int i = 0; i < qcaProviders.size(); ++i )
    {
        // each provider has a name, which we can display
        info += qcaProviders[i]->name() + ": ";
        // ... and also a list of features
        QStringList capabilities = qcaProviders[i]->features();
        // we turn the string list back into a single string,
        // and display it as well
        info += capabilities.join(", ") + "\n";
    }

    // Note that the default provider isn't included in
    // the result of QCA::providers()
    info += "default: ";
    // However it is still possible to get the features
    // supported by the default provider
    QStringList capabilities = QCA::defaultFeatures();
    info += capabilities.join(", ") + "\n";
    state->append(info);
}

void SIS::transfer()
{
    QCA::InitializationVector iv = QCA::InitializationVector(256);

    if (!QCA::isSupported("aes256-cbc-pkcs7"))
    {
      qDebug() << "Error using aes256";
      return;
    }

    QMessageEdit *edit = qobject_cast<QMessageEdit*>(sender());
    if(edit == 0)
        return;

    if(edit->toPlainText().size() == 0)
        return;

    QTcpSocket* socket = edit_socket[edit];
    datas conversation = networkMap[socket];
    QMessagesBrowser* browser = conversation.browser;
    QCA::SymmetricKey key = conversation.key;

    //Encoding with the original key
    QCA::Cipher cipher = QCA::Cipher("aes256", QCA::Cipher::CBC, QCA::Cipher::DefaultPadding, QCA::Encode, key, iv);

    QString currText = edit->toPlainText().replace("\n", "<br />");
    QCA::SecureArray secureData = currText.toUtf8();
    QCA::SecureArray encryptedData = cipher.process(secureData);
    if(!cipher.ok())
    {
      qDebug() << "Encryption failed!";
     // return;
    }

    QByteArray packet; // packet for sending the public key
    QDataStream out(&packet, QIODevice::WriteOnly);

    out << (quint16) 0; // writing 0 while not knowing the size
    out << text;
    out << encryptedData.toByteArray();
    out << iv.toByteArray();
    out.device()->seek(0);
    out << (quint16) (packet.size() - sizeof(quint16)); // overwriting the 0 by the real size

    socket->write(packet);


    QTime time = QDateTime::currentDateTime().time();
    QString t = Qt::escape(currText).replace("&lt;br /&gt;", "<br />").replace("&amp;", "&");
    t.replace(QRegExp("((ftp|https?):\\/\\/[a-zA-Z0-9\\.\\-\\/\\:\\_\\%\\?\\&\\=\\+\\#]+)"), "<a href='\\1'>\\1</a>");
    browser->append("<span style='color:#204a87;' title='" + time.toString() + "'><b>" + nickname + ": </b></span>" + t);
    edit->clear();
}

void SIS::newConversation()
{
    QTcpSocket* socket = server->nextPendingConnection();

    connect(socket, SIGNAL(readyRead()), this, SLOT(dataReceived()));
    connect(socket, SIGNAL(disconnected()), this, SLOT(disconnected()));

    openTab(socket);
}

void SIS::dataReceived()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if(socket == 0)
        return;

    datas& conversation = networkMap[socket];

    QDataStream in(socket);

    if (conversation.messageSize == 0)
    {
        if (socket->bytesAvailable() < (int)sizeof(quint16)) // messageSize not entierly received
             return;

        in >> conversation.messageSize; // messageSize received
    }

    if(socket->bytesAvailable() < conversation.messageSize)
        return;

    int type;
    in >> type;

    QByteArray data;
    in >> data;

    int tabId = conversation.tabId;
    QMessagesBrowser* browser = conversation.browser;

    conversation.messageSize = 0;

    if(type == givePubK)
    {
        QString pubKey;
        char *datas = new char[data.size()];
        strcpy(datas, data.data());
        pubKey = datas;
        delete [] datas;

        Friend contact(QCA::PublicKey::fromPEM(pubKey));
        conversation.contact = contact;

        QByteArray packet; // packet for sending the public key
        QDataStream out(&packet, QIODevice::WriteOnly);

        out << (quint16) 0; // writing 0 while not knowing the size
        out << replyPubK;
        out << publicKey.toPEM().toUtf8();
        out.device()->seek(0);
        out << (quint16) (packet.size() - sizeof(quint16)); // overwriting the 0 by the real size

        socket->write(packet);
    }
    else if(type == replyPubK)
    {
        QString pubKey;
        char *received = new char[data.size()];
        strcpy(received, data.data());
        pubKey = received;

        Friend contact(QCA::PublicKey::fromPEM(pubKey));
        conversation.contact = contact;

        delete [] received;

        QByteArray packet; // packet for sending the public key
        QDataStream out(&packet, QIODevice::WriteOnly);

        out << (quint16) 0; // writing 0 while not knowing the size
        out << giveBF;

        //Generating a symmetric key
        QCA::SymmetricKey key = QCA::SymmetricKey(256);
        conversation.key = key;

        QCA::SecureArray keyArr = key.toByteArray();

        //Encoding it
        QCA::SecureArray result = conversation.contact.getPubKey().encrypt(keyArr, QCA::EME_PKCS1_OAEP);

        //Sending it
        out << result.toByteArray();
        out.device()->seek(0);
        out << (quint16) (packet.size() - sizeof(quint16)); // overwriting the 0 by the real size

        socket->write(packet);
    }
    else if(type == giveBF)
    {
        QCA::SecureArray result = data;
        QCA::SecureArray decrypt;
        if(0 == privateKey.decrypt(result, &decrypt, QCA::EME_PKCS1_OAEP)) {
            browser->setText("Error decrypting");
            qDebug() << "Error decrypting";
            return;
        }

        QCA::SymmetricKey key = decrypt;
        conversation.key = key;

        QByteArray packet; // packet for sending the public key
        QDataStream out(&packet, QIODevice::WriteOnly);

        out << (quint16) 0; // writing 0 while not knowing the size
        out << replyBF;

        //Encoding it back
        QCA::SecureArray keyArr = key.toByteArray();
        QCA::SecureArray resultBis = conversation.contact.getPubKey().encrypt(keyArr, QCA::EME_PKCS1_OAEP);

        //Sending it
        out << resultBis.toByteArray();
        out.device()->seek(0);
        out << (quint16) (packet.size() - sizeof(quint16)); // overwriting the 0 by the real size

        socket->write(packet);
    }
    else if(type == replyBF)
    {
        QCA::SecureArray result = data;
        QCA::SecureArray decrypt;
        if(0 == privateKey.decrypt(result, &decrypt, QCA::EME_PKCS1_OAEP)) {
            browser->append("Error decrypting");
            qDebug() << "Error decrypting";
            return;
        }

        if(decrypt != conversation.key)
            socket->disconnectFromHost();

        QCA::InitializationVector iv = QCA::InitializationVector(256);
        QCA::Cipher cipher = QCA::Cipher("aes256", QCA::Cipher::CBC, QCA::Cipher::DefaultPadding, QCA::Encode, conversation.key, iv);

        QCA::SecureArray secureData = nickname.toUtf8();
        QCA::SecureArray encryptedData = cipher.process(secureData);
        if(!cipher.ok())
        {
          qDebug() << "Encryption failed!";
         // return;
        }

        QByteArray packet; // packet for sending the public key
        QDataStream out(&packet, QIODevice::WriteOnly);

        out << (quint16) 0; // writing 0 while not knowing the size
        out << giveNick;
        out << encryptedData.toByteArray();
        out << iv.toByteArray();
        out.device()->seek(0);
        out << (quint16) (packet.size() - sizeof(quint16)); // overwriting the 0 by the real size

        socket->write(packet);
    }
    else if(type == giveNick)
    {
        QByteArray initVector;
        in >> initVector;
        QCA::InitializationVector iv = initVector;
        QCA::Cipher cipher = QCA::Cipher("aes256", QCA::Cipher::CBC, QCA::Cipher::DefaultPadding, QCA::Decode, conversation.key, iv);
        QCA::SecureArray datas = data;
        QCA::SecureArray decryptedData = cipher.process(datas);
        if(!cipher.ok())
        {
            qDebug() << "Decryption failed!";
            browser->append("<b>Decryption failed</b>");
            return;
        }
        conversation.contact.setNickname(QString::fromUtf8(decryptedData.data()));

        iv = QCA::InitializationVector(256);
        cipher = QCA::Cipher("aes256", QCA::Cipher::CBC, QCA::Cipher::DefaultPadding, QCA::Encode, conversation.key, iv);

        QCA::SecureArray secureData = nickname.toUtf8();
        QCA::SecureArray encryptedData = cipher.process(secureData);
        if(!cipher.ok())
        {
          qDebug() << "Encryption failed!";
         // return;
        }

        QByteArray packet; // packet for sending the public key
        QDataStream out(&packet, QIODevice::WriteOnly);

        out << (quint16) 0; // writing 0 while not knowing the size
        out << replyNick;
        out << encryptedData.toByteArray();
        out << iv.toByteArray();
        out.device()->seek(0);
        out << (quint16) (packet.size() - sizeof(quint16)); // overwriting the 0 by the real size

      //  QSound::play(qApp->applicationDirPath() + "/sounds/login.wav");
        Phonon::createPlayer(Phonon::NoCategory, Phonon::MediaSource(qApp->applicationDirPath() + "/sounds/LoginP.mp3"))->play();

        socket->write(packet);
    }
    else if(type == replyNick)
    {
        QByteArray initVector;
        in >> initVector;
        QCA::InitializationVector iv = initVector;
        QCA::Cipher cipher = QCA::Cipher("aes256", QCA::Cipher::CBC, QCA::Cipher::DefaultPadding, QCA::Decode, conversation.key, iv);
        QCA::SecureArray datas = data;
        QCA::SecureArray decryptedData = cipher.process(datas);
        if(!cipher.ok())
        {
            qDebug() << "Decryption failed!";
            browser->append("<b>Decryption failed</b>");
            return;
        }
        conversation.contact.setNickname(QString::fromUtf8(decryptedData.data()));
        //QSound::play(qApp->applicationDirPath() + "/sounds/login.wav");
        Phonon::createPlayer(Phonon::NoCategory, Phonon::MediaSource(qApp->applicationDirPath() + "/sounds/LoginP.mp3"))->play();
    }
    else if(type == text)
    {
        if(tabId == -1)
        {
            reOpenTab(socket);
            tabId = conversation.tabId;
        }
        if(window->currentIndex() != tabId)
            window->setTabTextColor(tabId, Qt::blue);

        if (!QCA::isSupported("aes256-cbc-pkcs7"))
        {
          qDebug() << "Error using AES";
          return;
        }
      //  QSound receive(qApp->applicationDirPath() + "/sounds/receive.wav");
        Phonon::createPlayer(Phonon::NoCategory, Phonon::MediaSource(qApp->applicationDirPath() + "/sounds/ReceiveP.mp3"))->play();
        QByteArray initVector;
        in >> initVector;
        QCA::InitializationVector iv = initVector;
        QCA::Cipher cipher = QCA::Cipher("aes256", QCA::Cipher::CBC, QCA::Cipher::DefaultPadding, QCA::Decode, conversation.key, iv);
        QCA::SecureArray myDatas = data;
        QCA::SecureArray decryptedData = cipher.process(myDatas);
        if(!cipher.ok())
        {
            qDebug() << "Decryption failed!";
            browser->append("<b>Decryption failed</b>");
            return;
        }

        QTime time = QDateTime::currentDateTime().time();

        QString t = Qt::escape(QString::fromUtf8(decryptedData.data())).replace("&lt;br /&gt;", "<br />").replace("&amp;", "&");
        t.replace(QRegExp("((ftp|https?):\\/\\/[a-zA-Z0-9\\.\\-\\/\\:\\_\\%\\?\\&\\=\\+\\#]+)"), "<a href='\\1'>\\1</a>");
        browser->append("<span style='color:#cc0000;' title='" + time.toString() + "'><b>" + conversation.contact.getNickname() + ": </b></span>" + t);
    }
}

void SIS::requestNewConnection()
{
    bool ok;
    QString ip = QInputDialog::getText(this, "IP address", "Enter the IP address (IP:port) to which you want to connect:", QLineEdit::Normal, settings->value("lastIP", "").toString(), &ok);
    if(!ok)
        return;
    settings->setValue("lastIP", ip);
    QStringList address = ip.split(':');
    QTcpSocket* socket = new QTcpSocket;
    socket->connectToHost(address[0], address.at(1).toInt());

    connect(socket, SIGNAL(connected()), this, SLOT(connected()));
    connect(socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
    connect(socket, SIGNAL(readyRead()), this, SLOT(dataReceived()));
}

void SIS::connected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if(socket == 0)
        return;

    QByteArray packet; // packet for sending the public key
    QDataStream out(&packet, QIODevice::WriteOnly);

    openTab(socket);

    out << (quint16) 0; // writing 0 while not knowing the size
    out << givePubK;
    out << publicKey.toPEM().toUtf8();
    out.device()->seek(0);
    out << (quint16) (packet.size() - sizeof(quint16)); // overwriting the 0 by the real size

    socket->write(packet);
}

void SIS::disconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if(socket == 0)
        return;

    socket_edit[socket]->setEnabled(false);
}

void SIS::closeTab(int tab)
{
    networkMap[tabMap[tab].second].tabId = -1;
    for(unordered_map<QTcpSocket*, datas>::iterator it = networkMap.begin(); it != networkMap.end(); it++)
    {
        if(it->second.tabId > tab)
            it->second.tabId--;
    }

    tabMap.erase(tab);

    for(map<int, pair<QMessageEdit*, QTcpSocket*> >::iterator it = tabMap.begin(); it!= tabMap.end(); it++)
    {
        if(it->first > tab)
        {
            int currTabId = it->first;
            pair<QMessageEdit*, QTcpSocket*> current = it->second;
            tabMap.erase(currTabId);
            tabMap.insert({currTabId-1, current});
        }
    }
    window->update();
}

void SIS::openTab(QTcpSocket *socket)
{
    QMessagesBrowser* browser = new QMessagesBrowser;
    browser->setOpenExternalLinks(true);
    QMessageEdit* edit = new QMessageEdit;
    edit->setFocusPolicy(Qt::StrongFocus);
    connect(edit, SIGNAL(returnPressed()), this, SLOT(transfer()));
    connect(edit, SIGNAL(nextTab()), window, SLOT(nextTab()));
    connect(edit, SIGNAL(previousTab()), window, SLOT(previousTab()));
    edit->setFixedHeight(42);

    connect(browser, SIGNAL(giveFocus(QKeyEvent*)), edit, SLOT(setFocus()));
    connect(browser, SIGNAL(giveFocus(QKeyEvent*)), edit, SLOT(acceptKey(QKeyEvent*)));

    QVBoxLayout* lay = new QVBoxLayout;
    lay->addWidget(browser);
    lay->addWidget(edit);
    QWidget* cont = new QWidget;
    cont->setLayout(lay);

    if(!window->isVisible())
    {
        if(settings->value("maximized", false).toBool())
            window->showMaximized();
        else
            window->show();
    }


    int tabId = window->addTab(cont, "Socket " + QString::number(socket->socketDescriptor()));
    window->setTabTextColor(tabId, Qt::black);

    edit_socket.insert({edit,socket});
    socket_edit.insert({socket, edit});

    datas current;
    current.browser = browser;
    current.tabId = tabId;
    current.container = cont;
    current.messageSize = 0;

    networkMap.insert({socket, current});
    tabMap.insert({tabId, {edit, socket}});

    edit->setFocus();
    window->update();
}

void SIS::reOpenTab(QTcpSocket *socket)
{
    if(!window->isVisible())
    {
        if(settings->value("maximized", false).toBool())
            window->showMaximized();
        else
            window->show();
    }

    datas data = networkMap[socket];
    int tabId = window->addTab(data.container, "Socket " + QString::number(socket->socketDescriptor()));
    networkMap[socket].tabId = tabId;
    tabMap.insert({tabId, {socket_edit[socket], socket}});
    window->update();
}

void SIS::closeAllTabs()
{
    tabMap.clear();
    for(unordered_map<QTcpSocket*, datas>::iterator it = networkMap.begin(); it != networkMap.end(); it++)
        it->second.tabId = -1;
}

void SIS::moveTab(int from, int to)
{
    networkMap[tabMap[from].second].tabId = to;
    networkMap[tabMap[to].second].tabId = from;

    pair<QMessageEdit*, QTcpSocket*> dataFrom = tabMap[from];
    tabMap.erase(from);

    pair<QMessageEdit*, QTcpSocket*> dataTo = tabMap[to];
    tabMap.erase(to);

    tabMap.insert({to, dataFrom});
    tabMap.insert({from, dataTo});
}
