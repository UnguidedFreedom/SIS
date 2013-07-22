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

    QElapsedTimer* timer = new QElapsedTimer;
    timer->start();

    QString privateKeyTmp = settings->value("privateKey", "").toString();

    if(privateKeyTmp.trimmed() == "")
    {
        //Generating the private key
        InvertibleRSAFunction params;
        params.GenerateRandomWithKeySize(rng, 4096);

        privateKey = RSA::PrivateKey(params);
        publicKey = RSA::PublicKey(params);

        if(!privateKey.Validate(rng, 3))
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
            QByteArray passPhrase = passOne.toUtf8();

            string privKeyDer;
            StringSink privKeyDerSink(privKeyDer);
            privateKey.DEREncode(privKeyDerSink);

            QString privKeyQString;
            for(unsigned int i=0; i<privKeyDer.length(); i++)
                privKeyQString += QString::number(privKeyDer[i]) + " ";
            qDebug() << privKeyQString;

            //Hash the pass phrase to create 128 bit key
            string hashedPass;
            RIPEMD128 hash;
            StringSource(string(passPhrase.data()), true, new HashFilter(hash, new StringSink(hashedPass)));

            // Generate a random IV
            byte iv[AES::BLOCKSIZE];
            rng.GenerateBlock(iv, AES::BLOCKSIZE);

            //Encrypt private key
            CFB_Mode<AES>::Encryption cfbEncryption((const unsigned char*)hashedPass.c_str(), hashedPass.length(), iv);
            byte encPrivKey[privKeyDer.length()];
            cfbEncryption.ProcessData(encPrivKey, (const byte*)privKeyDer.c_str(), privKeyDer.length());
            string encPrivKeyStr((char *)encPrivKey, privKeyDer.length());

            //Save private key to file
            string hexEncodedPrivKey;
            StringSource encPrivKeySrc(encPrivKeyStr, true);
            HexEncoder sink(new StringSink(hexEncodedPrivKey));
            encPrivKeySrc.CopyTo(sink);
            sink.MessageEnd();

            string encIvStr((char *)iv, AES::BLOCKSIZE);
            string hexEncodedIv;
            StringSource encIvSrc(encIvStr, true);
            HexEncoder sinkIv(new StringSink(hexEncodedIv));
            encIvSrc.CopyTo(sinkIv);
            sinkIv.MessageEnd();

            settings->setValue("privateKey", QString(hexEncodedPrivKey.c_str()));
            settings->setValue("privateKeyIV", QString(hexEncodedIv.c_str()));
        }
        else
        {
            state->setText("No password given");
            return;
        }
    }
    else
    {
        QString pass = QInputDialog::getText(this, "Sign in", "Enter your password to sign in:", QLineEdit::Password);

        QByteArray passPhrase = pass.toUtf8();

        string hexEncodedPrivKey = settings->value("privateKey", "").toString().toStdString();
        string encPrivKeyStr;
        StringSource hexDecPrivKeySrc(hexEncodedPrivKey, true);
        HexDecoder sink(new StringSink(encPrivKeyStr));
        hexDecPrivKeySrc.CopyTo(sink);
        sink.MessageEnd();

        string hexEncodedIv = settings->value("privateKeyIV", "").toString().toStdString();
        string sourceIv;
        StringSource hexDecIvSrc(hexEncodedIv, true);
        HexDecoder sinkIv(new StringSink(sourceIv));
        hexDecIvSrc.CopyTo(sinkIv);
        sinkIv.MessageEnd();

        //Hash the pass phrase to create 128 bit key
        string hashedPass;
        RIPEMD128 hash;
        StringSource(string(passPhrase.data()), true, new HashFilter(hash, new StringSink(hashedPass)));

        CFB_Mode<AES>::Decryption cfbDecryption((const unsigned char*)hashedPass.c_str(), hashedPass.length(), (byte*)sourceIv.c_str());
        byte privKeyDer[encPrivKeyStr.length()];
        cfbDecryption.ProcessData(privKeyDer, (const byte*)encPrivKeyStr.c_str(), encPrivKeyStr.length());
        string privKeyDerStr((char *)privKeyDer, encPrivKeyStr.length());

        StringSource privKeyDerSource(privKeyDerStr, true);
        privateKey.BERDecode(privKeyDerSource);

        publicKey = RSA::PublicKey(privateKey);

        if(!privateKey.Validate(rng, 3))
        {
            state->setText("Invalid secret key");
            qDebug() << "Invalid secret key";
            return;
        }
        else
        {
            state->setText("Secret key successfully loaded");
        }

        if(! true )
        {
            state->setText("Unable to read key from file");
            return;
        }
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
    port = 40000;
    while(!server->listen(QHostAddress::Any, port) && port < 50000)
        port++;
    if(port == 50000)
    {
        qDebug() << "No valid port available";
        return;
    }
    state->append("Now listening on port " + QString::number(port));

    connect(server, SIGNAL(newConnection()), this, SLOT(newConversation()));

    state->append(QString::number(timer->elapsed()) + " ms to launch program");
    button->setEnabled(true);
}

void SIS::transfer()
{
    QMessageEdit *edit = qobject_cast<QMessageEdit*>(sender());
    if(edit == 0)
        return;

    if(edit->toPlainText().size() == 0)
        return;

    QTcpSocket* socket = edit_socket[edit];
    datas conversation = networkMap[socket];
    QMessagesBrowser* browser = conversation.browser;

    //Encoding
    QString currText = edit->toPlainText().replace("\n", "<br />");
    QByteArray arr = currText.toUtf8();

    byte iv[AES::BLOCKSIZE];
    rng.GenerateBlock(iv, AES::BLOCKSIZE);

    CTR_Mode<AES>::Encryption encryption(conversation.key, AES::MAX_KEYLENGTH, iv);
    StreamTransformationFilter encryptor(encryption, NULL);

    for(int i = 0; i < arr.size(); i++)
        encryptor.Put((byte)arr.at(i));

    encryptor.MessageEnd();
    size_t ready = encryptor.MaxRetrievable();

    byte* cipher;
    cipher = new byte[ready];
    encryptor.Get(cipher, ready);

    QByteArray packet; // packet for sending the public key
    QDataStream out(&packet, QIODevice::WriteOnly);

    out << (quint16) 0; // writing 0 while not knowing the size
    out << text;
    for(int i=0; i<AES::BLOCKSIZE; i++)
        out << iv[i];
    out << (unsigned int)ready;
    for(unsigned int i=0; i<ready; i++)
        out << cipher[i];
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

    conversation.messageSize = 0;

    if(type == text)
    {
        int tabId = conversation.tabId;
        QMessagesBrowser* browser = conversation.browser;

        if(tabId == -1)
        {
            reOpenTab(socket);
            tabId = conversation.tabId;
        }

        if(window->currentIndex() != tabId)
            window->setTabTextColor(tabId, Qt::blue);

      //  QSound receive(qApp->applicationDirPath() + "/sounds/receive.wav");
        Phonon::createPlayer(Phonon::NoCategory, Phonon::MediaSource(qApp->applicationDirPath() + "/sounds/ReceiveP.mp3"))->play();

        byte iv[AES::BLOCKSIZE];
        for(int i=0; i<AES::BLOCKSIZE; i++)
            in >> iv[i];

        unsigned int ready;
        in >> ready;
        byte* cipher;
        cipher = new byte[ready];

        byte* key = conversation.key;

        CTR_Mode < AES >::Decryption decryption(key, AES::MAX_KEYLENGTH, iv);
        StreamTransformationFilter decryptor(decryption, NULL);

        for(size_t i = 0; i < ready; i++)
        {
            in >> cipher[i];
            decryptor.Put(cipher[i]);
        }

        decryptor.MessageEnd();
        ready = decryptor.MaxRetrievable();
        byte* plain;
        plain = new byte[ready];
        decryptor.Get(plain, ready);

        char* newData;
        newData = new char[ready];
        for(unsigned int i=0; i<ready; i++)
            newData[i] = plain[i];

        QTime time = QDateTime::currentDateTime().time();

        QString t = Qt::escape(QString::fromUtf8(newData, ready)).replace("&lt;br /&gt;", "<br />").replace("&amp;", "&");
        t.replace(QRegExp("((ftp|https?):\\/\\/[a-zA-Z0-9\\.\\-\\/\\:\\_\\%\\?\\&\\=\\+\\#]+)"), "<a href='\\1'>\\1</a>");
        browser->append("<span style='color:#cc0000;' title='" + time.toString() + "'><b>" + conversation.contact.getNickname() + ": </b></span>" + t);
    }
    else if(type == givePubK)
    {
        unsigned int length;
        in >> length;

        string derEncPubKey;
        for(unsigned int i = 0; i<length; i++)
        {
            int tmp;
            in >> tmp;
            derEncPubKey.push_back((char)tmp);
        }

        RSA::PublicKey tmpPubKey;
        StringSource derEncPubSrc(derEncPubKey, true);
        tmpPubKey.BERDecode(derEncPubSrc);

        Friend contact(tmpPubKey);
        conversation.contact = contact;

        string pubKeyDer;
        StringSink pubKeyDerSink(pubKeyDer);
        publicKey.DEREncode(pubKeyDerSink);

        QByteArray packet; // packet for replying the public key
        QDataStream out(&packet, QIODevice::WriteOnly);

        out << (quint16) 0; // writing 0 while not knowing the size
        out << replyPubK;
        out << (unsigned int)pubKeyDer.size();
        for(unsigned int i=0; i<pubKeyDer.size(); i++)
            out << pubKeyDer[i];
        out.device()->seek(0);
        out << (quint16) (packet.size() - sizeof(quint16)); // overwriting the 0 by the real size

        socket->write(packet);
    }
    else if(type == replyPubK)
    {
        unsigned int length;
        in >> length;

        string derEncPubKey;
        for(unsigned int i = 0; i<length; i++)
        {
            int tmp;
            in >> tmp;
            derEncPubKey.push_back((char)tmp);
        }

        RSA::PublicKey tmpPubKey;
        StringSource derEncPubSrc(derEncPubKey, true);
        tmpPubKey.BERDecode(derEncPubSrc);

        Friend contact(tmpPubKey);
        conversation.contact = contact;

        QByteArray packet; // packet for sending the symmetric key
        QDataStream out(&packet, QIODevice::WriteOnly);


        string test;
        StringSink sink(test);
        conversation.contact.getPubKey().DEREncode(sink);

        //Generating a symmetric key
        conversation.key = new byte[AES::MAX_KEYLENGTH];
        rng.GenerateBlock(conversation.key, AES::MAX_KEYLENGTH);

        //Encoding it with RSA

        string plain((char*)conversation.key, AES::MAX_KEYLENGTH), cipher;

        RSAES_OAEP_SHA_Encryptor encryptor(conversation.contact.getPubKey());
        StringSource(plain, true, new PK_EncryptorFilter(rng, encryptor, new StringSink(cipher)));

        //Sending it
        out << (quint16) 0; // writing 0 while not knowing the size
        out << giveAES;
        out << (unsigned int)cipher.size();
        for(unsigned int i=0; i<cipher.size(); i++)
            out << cipher[i];
        out.device()->seek(0);
        out << (quint16) (packet.size() - sizeof(quint16)); // overwriting the 0 by the real size

        socket->write(packet);
    }
    else if(type == giveAES)
    {
        unsigned int length;
        in >> length;
        string cipher;
        for(unsigned int i=0; i<length; i++)
        {
            int tmp;
            in >> tmp;
            cipher.push_back((char)tmp);
        }
        string plain;

        RSAES_OAEP_SHA_Decryptor decryptor(privateKey);
        StringSource(cipher, true, new PK_DecryptorFilter(rng, decryptor, new StringSink(plain)));

        conversation.key = new byte[AES::MAX_KEYLENGTH];
        for(int i=0; i<AES::MAX_KEYLENGTH; i++)
            conversation.key[i] = plain[i];

        plain = string((char*)conversation.key, AES::MAX_KEYLENGTH);
        cipher = "";

        RSAES_OAEP_SHA_Encryptor encryptor(conversation.contact.getPubKey());
        StringSource(plain, true, new PK_EncryptorFilter(rng, encryptor, new StringSink(cipher)));

        QByteArray packet; // packet for replying the private key
        QDataStream out(&packet, QIODevice::WriteOnly);

        //Sending it
        out << (quint16) 0; // writing 0 while not knowing the size
        out << replyAES;
        out << (unsigned int)cipher.size();
        for(unsigned int i=0; i<cipher.size(); i++)
            out << cipher[i];
        out.device()->seek(0);
        out << (quint16) (packet.size() - sizeof(quint16)); // overwriting the 0 by the real size
    }
    else if(type == replyAES)
    {
        qDebug() << "replyAes";
        unsigned int length;
        in >> length;
        string received;
        for(unsigned int i=0; i<length; i++)
        {
            int tmp;
            in >> tmp;
            received.push_back((char)tmp);
        }
        string plain;

        RSAES_OAEP_SHA_Decryptor decryptor(privateKey);
        StringSource(received, true, new PK_DecryptorFilter(rng, decryptor, new StringSink(plain)));
        bool valid = true;
        for(int i=0; i<AES::MAX_KEYLENGTH && valid; i++)
        {
            if((byte)plain[i] != conversation.key[i])
                valid = false;
        }

        if(!valid)
        {
            conversation.browser->setText("Invalid key replied");
            return;
        }

        //Encoding
        QByteArray arr = nickname.toUtf8();

        byte iv[AES::BLOCKSIZE];
        rng.GenerateBlock(iv, AES::BLOCKSIZE);

        CTR_Mode<AES>::Encryption encryption(conversation.key, AES::MAX_KEYLENGTH, iv);
        StreamTransformationFilter encryptor(encryption, NULL);

        for(int i = 0; i < arr.size(); i++)
            encryptor.Put((byte)arr.at(i));

        encryptor.MessageEnd();
        size_t ready = encryptor.MaxRetrievable();

        byte* cipher;
        cipher = new byte[ready];
        encryptor.Get(cipher, ready);

        QByteArray packet; // packet for sending the nickname
        QDataStream out(&packet, QIODevice::WriteOnly);

        out << (quint16) 0; // writing 0 while not knowing the size
        out << giveNick;
        for(int i=0; i<AES::BLOCKSIZE; i++)
            out << iv[i];
        out << (unsigned int)ready;
        for(unsigned int i=0; i<ready; i++)
            out << cipher[i];
        out.device()->seek(0);
        out << (quint16) (packet.size() - sizeof(quint16)); // overwriting the 0 by the real size

        socket->write(packet);
    }
    else if(type == giveNick)
    {
        byte iv[AES::BLOCKSIZE];
        for(int i=0; i<AES::BLOCKSIZE; i++)
            in >> iv[i];

        unsigned int ready;
        in >> ready;
        byte* cipher;
        cipher = new byte[ready];

        byte* key = conversation.key;

        CTR_Mode<AES>::Decryption decryption(key, AES::MAX_KEYLENGTH, iv);
        StreamTransformationFilter decryptor(decryption, NULL);

        for(size_t i = 0; i < ready; i++)
        {
            in >> cipher[i];
            decryptor.Put(cipher[i]);
        }

        decryptor.MessageEnd();
        ready = decryptor.MaxRetrievable();
        byte* plain;
        plain = new byte[ready];
        decryptor.Get(plain, ready);

        char* newData;
        newData = new char[ready];
        for(unsigned int i=0; i<ready; i++)
            newData[i] = plain[i];

        conversation.contact.setNickname(QString::fromUtf8(newData, ready));

        //Encoding
        QByteArray arr = nickname.toUtf8();
        rng.GenerateBlock(iv, AES::BLOCKSIZE);

        CTR_Mode<AES>::Encryption encryption(conversation.key, AES::MAX_KEYLENGTH, iv);
        StreamTransformationFilter encryptor(encryption, NULL);

        for(int i = 0; i < arr.size(); i++)
            encryptor.Put((byte)arr.at(i));

        encryptor.MessageEnd();
        ready = encryptor.MaxRetrievable();

        cipher = new byte[ready];
        encryptor.Get(cipher, ready);

        QByteArray packet; // packet for replying the nickname
        QDataStream out(&packet, QIODevice::WriteOnly);

        out << (quint16) 0; // writing 0 while not knowing the size
        out << replyNick;
        for(int i=0; i<AES::BLOCKSIZE; i++)
            out << iv[i];
        out << (unsigned int)ready;
        for(unsigned int i=0; i<ready; i++)
            out << cipher[i];
        out.device()->seek(0);
        out << (quint16) (packet.size() - sizeof(quint16)); // overwriting the 0 by the real size

        socket->write(packet);

      //  QSound::play(qApp->applicationDirPath() + "/sounds/login.wav");
        Phonon::createPlayer(Phonon::NoCategory, Phonon::MediaSource(qApp->applicationDirPath() + "/sounds/LoginP.mp3"))->play();

    }
    else if(type == replyNick)
    {
        byte iv[AES::BLOCKSIZE];
        for(int i=0; i<AES::BLOCKSIZE; i++)
            in >> iv[i];

        unsigned int ready;
        in >> ready;
        byte* cipher;
        cipher = new byte[ready];

        CTR_Mode<AES>::Decryption decryption(conversation.key, AES::MAX_KEYLENGTH, iv);
        StreamTransformationFilter decryptor(decryption, NULL);

        for(size_t i = 0; i < ready; i++)
        {
            in >> cipher[i];
            decryptor.Put(cipher[i]);
        }

        decryptor.MessageEnd();
        ready = decryptor.MaxRetrievable();
        byte* plain;
        plain = new byte[ready];
        decryptor.Get(plain, ready);

        char* newData;
        newData = new char[ready];
        for(unsigned int i=0; i<ready; i++)
            newData[i] = plain[i];

        conversation.contact.setNickname(QString::fromUtf8(newData, ready));
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

    string pubKeyDer;
    StringSink pubKeyDerSink(pubKeyDer);
    publicKey.DEREncode(pubKeyDerSink);

    openTab(socket);

    out << (quint16) 0; // writing 0 while not knowing the size
    out << givePubK;
    out << (unsigned int)pubKeyDer.size();
    for(unsigned int i=0; i<pubKeyDer.size(); i++)
        out << pubKeyDer[i];
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
    socket_edit.erase(socket_edit.find(socket));
}

void SIS::closeTab(int tab)
{
    networkMap[tabMap[tab].second].tabId = -1;
    for(map<QTcpSocket*, datas>::iterator it = networkMap.begin(); it != networkMap.end(); it++)
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
            tabMap[currTabId-1] = current;
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

    edit_socket[edit] = socket;
    socket_edit[socket] = edit;

    datas current;
    current.browser = browser;
    current.tabId = tabId;
    current.container = cont;
    current.messageSize = 0;

    networkMap[socket] = current;
    tabMap[tabId] = pair<QMessageEdit*, QTcpSocket*>(edit, socket);

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
    tabMap[tabId] = pair<QMessageEdit*, QTcpSocket*>(socket_edit[socket], socket);
    window->update();
}

void SIS::closeAllTabs()
{
    tabMap.clear();
    for(map<QTcpSocket*, datas>::iterator it = networkMap.begin(); it != networkMap.end(); it++)
        it->second.tabId = -1;
}

void SIS::moveTab(int from, int to)
{
    networkMap[tabMap[from].second].tabId = to;
    networkMap[tabMap[to].second].tabId = from;

    pair<QMessageEdit*, QTcpSocket*> dataFrom = tabMap[from];

    tabMap[from] = tabMap[to];
    tabMap[to] = dataFrom;
}
