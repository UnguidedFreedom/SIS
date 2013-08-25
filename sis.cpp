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
    CRYPTO_malloc_init();
    OPENSSL_add_all_algorithms_conf();
    ERR_load_crypto_strings();

    ERR_print_errors_fp(stderr);

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
    keys = RSA_new();

   // seed_prng(16);

    if(privateKeyTmp.trimmed() == "")
    {
        //Generating the private key
        keys = RSA_generate_key(4096, RSA_F4, NULL, NULL);

        if(!RSA_check_key(keys))
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

            FILE* pFile = NULL;
            pFile = tmpfile();

            PEM_write_RSAPrivateKey(pFile, keys, EVP_des_ede3_cbc(), NULL, 0, NULL, passPhrase.data());

            QString tmp;
            rewind(pFile);

            char buffer[256];
            while(!feof(pFile))
            {
                if(fgets(buffer, 256, pFile) == NULL)
                    break;
                tmp.append(buffer);
            }

            settings->setValue("privateKey", tmp);
        }
        else
        {
            state->setText("No password given");
            return;
        }
    }
    else
    {
        bool ok;
        QString pass = QInputDialog::getText(this, "Sign in", "Enter your password to sign in:", QLineEdit::Password, "", &ok);
        if(!ok)
        {
            state->setText("Login operation canceled");
            return;
        }

        keys = RSA_new();

        QByteArray passPhrase = pass.toUtf8();
        FILE* pFile;
        pFile = tmpfile();
        privateKeyTmp = settings->value("privateKey", "").toString();

        fputs(privateKeyTmp.toUtf8().data(), pFile);

        rewind(pFile);

        PEM_read_RSAPrivateKey(pFile, &keys, NULL, passPhrase.data());

        if(!RSA_check_key(keys))
        {
            state->setText("Invalid secret key");
            qDebug() << "Invalid secret key";
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

    unsigned char iv[16];
    RAND_bytes(iv, 16);

    EVP_CIPHER_CTX *ctx =  new EVP_CIPHER_CTX;
    EVP_CIPHER_CTX_init(ctx);
    EVP_EncryptInit(ctx, EVP_aes_256_gcm(), conversation.key, iv);
    unsigned char *ret;
    int tmp, ol;
    ret = new unsigned char[arr.size() + EVP_CIPHER_CTX_block_size(ctx)];
    EVP_EncryptUpdate(ctx, ret, &tmp, (unsigned char*)arr.data(), arr.size());
    ol = tmp;
    EVP_EncryptFinal(ctx, &ret[tmp], &tmp);
    ol += tmp;

    unsigned char TAG[16];
    EVP_CIPHER_CTX_ctrl (ctx, EVP_CTRL_GCM_GET_TAG, 16, TAG);

    QByteArray packet; // packet for sending the public key
    QDataStream out(&packet, QIODevice::WriteOnly);

    out << (quint16) 0; // writing 0 while not knowing the size
    out << text;
    for(int i=0; i<16; i++)
        out << iv[i];
    for(int i=0; i<16; i++)
        out << TAG[i];
    out << ol;
    for(int i=0; i<ol; i++)
        out << ret[i];
    out.device()->seek(0);
    out << (quint16) (packet.size() - sizeof(quint16)); // overwriting the 0 by the real size

    socket->write(packet);

    QTime time = QDateTime::currentDateTime().time();
    QString t = Qt::escape(currText).replace("&lt;br /&gt;", "<br />").replace("&amp;", "&");
    t.replace(QRegExp("((ftp|https?):\\/\\/[a-zA-Z0-9\\.\\-\\/\\:\\_\\%\\?\\&\\=\\+\\#]+)"), "<a href='\\1'>\\1</a>");
    browser->append("<span style='color:#204a87;' title='" + QString::number(time.hour()) + "h" + (time.minute()<10 ? "0" : "") + QString::number(time.minute()) + "'><b>" + nickname + ": </b></span>" + t);
    edit->clear();
}

void SIS::newConversation()
{
    QTcpSocket* socket = server->nextPendingConnection();

    openTab(socket);

    connect(socket, SIGNAL(readyRead()), this, SLOT(dataReceived()));
    connect(socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
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

        Phonon::createPlayer(Phonon::NoCategory, Phonon::MediaSource(qApp->applicationDirPath() + "/sounds/Receive.mp3"))->play();

        unsigned char iv[16];
        for(int i=0; i<16; i++)
            in >> iv[i];

        unsigned char TAG[16];
        for(int i=0; i<16; i++)
            in >> TAG[i];

        int ol;
        in >> ol;
        unsigned char* cipher;
        cipher = new unsigned char[ol];
        for(int i=0; i<ol; i++)
            in >> cipher[i];

        EVP_CIPHER_CTX *ctx =  new EVP_CIPHER_CTX;
        EVP_CIPHER_CTX_init(ctx);
        EVP_DecryptInit(ctx, EVP_aes_256_gcm(), conversation.key, iv);
        EVP_CIPHER_CTX_ctrl (ctx, EVP_CTRL_GCM_SET_TAG, 16, TAG);
        unsigned char *ret;
        int tmp;
        ret = new unsigned char[ol + EVP_CIPHER_CTX_block_size(ctx)];
        EVP_DecryptUpdate(ctx, ret, &tmp, cipher, ol);
        ol = tmp;
        if(!EVP_DecryptFinal(ctx, &ret[tmp], &tmp))
            return;
        ol += tmp;

        QTime time = QDateTime::currentDateTime().time();

        QString t = Qt::escape(QString::fromUtf8((char*)ret, ol)).replace("&lt;br /&gt;", "<br />").replace("&amp;", "&");
        t.replace(QRegExp("((ftp|https?):\\/\\/[a-zA-Z0-9\\.\\-\\/\\:\\_\\%\\?\\&\\=\\+\\#]+)"), "<a href='\\1'>\\1</a>");
        browser->append("<span style='color:#cc0000;' title='" + QString::number(time.hour()) + "h" + (time.minute()<10 ? "0" : "") + QString::number(time.minute()) + "'><b>" + conversation.contact.getNickname() + ": </b></span>" + t);
    }
    else if(type == givePubK)
    {
        QString tmp;
        in >> tmp;

        FILE* pFile;
        pFile = tmpfile();

        fputs(tmp.toStdString().c_str(), pFile);
        rewind(pFile);

        RSA* friendKey = RSA_new();
        PEM_read_RSAPublicKey(pFile, &friendKey, NULL, NULL);

        Friend contact(friendKey);
        conversation.contact = contact;
        fclose(pFile);

        pFile = tmpfile();

        PEM_write_RSAPublicKey(pFile, keys);

        tmp = "";
        rewind(pFile);

        char buffer[256];
        while(!feof(pFile))
        {
            if(fgets(buffer, 256, pFile) == NULL)
                break;
            tmp.append(buffer);
        }

        QByteArray packet; // packet for replying the public key
        QDataStream out(&packet, QIODevice::WriteOnly);

        out << (quint16) 0; // writing 0 while not knowing the size
        out << replyPubK;
        out << tmp;
        out.device()->seek(0);
        out << (quint16) (packet.size() - sizeof(quint16)); // overwriting the 0 by the real size

        socket->write(packet);
    }
    else if(type == replyPubK)
    {
        QString tmp;
        in >> tmp;

        FILE* pFile;
        pFile = tmpfile();

        fputs(tmp.toStdString().c_str(), pFile);
        rewind(pFile);

        RSA* friendKey = RSA_new();
        PEM_read_RSAPublicKey(pFile, &friendKey, NULL, NULL);

        Friend contact(friendKey);
        conversation.contact = contact;

        //Generating a symmetric key
        conversation.key = new unsigned char[32];
        RAND_bytes(conversation.key, 32);

        //Encoding it with RSA
        const int size = RSA_size(conversation.contact.getPubKey());
        unsigned char* to = NULL;
        to = new unsigned char[size];
        RSA_public_encrypt(32, conversation.key, to, conversation.contact.getPubKey(), RSA_PKCS1_OAEP_PADDING);

        //Sending it
        QByteArray packet; // packet for sending the symmetric key
        QDataStream out(&packet, QIODevice::WriteOnly);
        out << (quint16) 0; // writing 0 while not knowing the size
        out << giveAES;
        for(int i=0; i<size; i++)
            out << to[i];
        out.device()->seek(0);
        out << (quint16) (packet.size() - sizeof(quint16)); // overwriting the 0 by the real size
        socket->write(packet);
    }
    else if(type == giveAES)
    {
        const int mySize = RSA_size(keys);
        unsigned char* from = NULL;
        from = new unsigned char[mySize];
        for(int i=0; i<mySize; i++)
            in >> from[i];

        conversation.key = new unsigned char[32];
        RSA_private_decrypt(mySize, from, conversation.key, keys, RSA_PKCS1_OAEP_PADDING);

        const int size = RSA_size(conversation.contact.getPubKey());
        unsigned char* to = NULL;
        to = new unsigned char[size];
        RSA_public_encrypt(32, conversation.key, to, conversation.contact.getPubKey(), RSA_PKCS1_OAEP_PADDING);

        QByteArray packet; // packet for replying the private key
        QDataStream out(&packet, QIODevice::WriteOnly);

        //Sending it
        out << (quint16) 0; // writing 0 while not knowing the size
        out << replyAES;
        for(int i=0; i<size; i++)
            out << to[i];
        out.device()->seek(0);
        out << (quint16) (packet.size() - sizeof(quint16)); // overwriting the 0 by the real size

        socket->write(packet);
    }
    else if(type == replyAES)
    {
        const int mySize = RSA_size(keys);
        unsigned char* from = NULL;
        from = new unsigned char[mySize];
        for(int i=0; i<mySize; i++)
            in >> from[i];

        unsigned char to[32];
        RSA_private_decrypt(mySize, from, to, keys, RSA_PKCS1_OAEP_PADDING);


        bool valid = true;
        for(int i=0; i<32 && valid; i++)
        {
            if(to[i] != conversation.key[i])
                valid = false;
        }

        if(!valid)
        {
            conversation.browser->setText("Invalid key replied"); // This should close the conversation
            return;
        }

        //Encoding
        QByteArray arr = nickname.toUtf8();

        unsigned char iv[16];
        RAND_bytes(iv, 16);

        EVP_CIPHER_CTX *ctx =  new EVP_CIPHER_CTX;
        EVP_CIPHER_CTX_init(ctx);
        EVP_EncryptInit(ctx, EVP_aes_256_gcm(), conversation.key, iv);
        unsigned char *ret;
        int tmp, ol;
        ret = new unsigned char[arr.size() + EVP_CIPHER_CTX_block_size(ctx)];
        EVP_EncryptUpdate(ctx, ret, &tmp, (unsigned char*)arr.data(), arr.size());
        ol = tmp;
        EVP_EncryptFinal(ctx, &ret[tmp], &tmp);
        ol += tmp;

        unsigned char TAG[16];
        EVP_CIPHER_CTX_ctrl (ctx, EVP_CTRL_GCM_GET_TAG, 16, TAG);

        QByteArray packet; // packet for sending the public key
        QDataStream out(&packet, QIODevice::WriteOnly);

        out << (quint16) 0; // writing 0 while not knowing the size
        out << giveNick;
        for(int i=0; i<16; i++)
            out << iv[i];
        for(int i=0; i<16; i++)
            out << TAG[i];
        out << ol;
        for(int i=0; i<ol; i++)
            out << ret[i];
        out.device()->seek(0);
        out << (quint16) (packet.size() - sizeof(quint16)); // overwriting the 0 by the real size

        socket->write(packet);
    }
    else if(type == giveNick)
    {
        unsigned char iv[16];
        for(int i=0; i<16; i++)
            in >> iv[i];

        unsigned char TAG[16];
        for(int i=0; i<16; i++)
            in >> TAG[i];

        int ol;
        in >> ol;
        unsigned char* cipher;
        cipher = new unsigned char[ol];
        for(int i=0; i<ol; i++)
            in >> cipher[i];

        EVP_CIPHER_CTX *ctx =  new EVP_CIPHER_CTX;
        EVP_CIPHER_CTX_init(ctx);
        EVP_DecryptInit(ctx, EVP_aes_256_gcm(), conversation.key, iv);
        EVP_CIPHER_CTX_ctrl (ctx, EVP_CTRL_GCM_SET_TAG, 16, TAG);
        unsigned char *ret;
        int tmp;
        ret = new unsigned char[ol + EVP_CIPHER_CTX_block_size(ctx)];
        EVP_DecryptUpdate(ctx, ret, &tmp, cipher, ol);
        ol = tmp;
        if(!EVP_DecryptFinal(ctx, &ret[tmp], &tmp))
            return;
        ol += tmp;

        conversation.contact.setNickname(QString::fromUtf8((char*)ret, ol));

        //Encoding
        QByteArray arr = nickname.toUtf8();

        RAND_bytes(iv, 16);

        ctx =  new EVP_CIPHER_CTX;
        EVP_CIPHER_CTX_init(ctx);
        EVP_EncryptInit(ctx, EVP_aes_256_gcm(), conversation.key, iv);
        ret = new unsigned char[arr.size() + EVP_CIPHER_CTX_block_size(ctx)];
        EVP_EncryptUpdate(ctx, ret, &tmp, (unsigned char*)arr.data(), arr.size());
        ol = tmp;
        EVP_EncryptFinal(ctx, &ret[tmp], &tmp);
        ol += tmp;

        EVP_CIPHER_CTX_ctrl (ctx, EVP_CTRL_GCM_GET_TAG, 16, TAG);

        QByteArray packet; // packet for sending the public key
        QDataStream out(&packet, QIODevice::WriteOnly);

        out << (quint16) 0; // writing 0 while not knowing the size
        out << replyNick;
        for(int i=0; i<16; i++)
            out << iv[i];
        for(int i=0; i<16; i++)
            out << TAG[i];
        out << ol;
        for(int i=0; i<ol; i++)
            out << ret[i];
        out.device()->seek(0);
        out << (quint16) (packet.size() - sizeof(quint16)); // overwriting the 0 by the real size

        socket->write(packet);

      //  QSound::play(qApp->applicationDirPath() + "/sounds/login.wav");
        Phonon::createPlayer(Phonon::NoCategory, Phonon::MediaSource(qApp->applicationDirPath() + "/sounds/Login.mp3"))->play();

    }
    else if(type == replyNick)
    {
        unsigned char iv[16];
        for(int i=0; i<16; i++)
            in >> iv[i];

        unsigned char TAG[16];
        for(int i=0; i<16; i++)
            in >> TAG[i];

        int ol;
        in >> ol;
        unsigned char* cipher;
        cipher = new unsigned char[ol];
        for(int i=0; i<ol; i++)
            in >> cipher[i];

        EVP_CIPHER_CTX *ctx =  new EVP_CIPHER_CTX;
        EVP_CIPHER_CTX_init(ctx);
        EVP_DecryptInit(ctx, EVP_aes_256_gcm(), conversation.key, iv);
        EVP_CIPHER_CTX_ctrl (ctx, EVP_CTRL_GCM_SET_TAG, 16, TAG);
        unsigned char *ret;
        int tmp;
        ret = new unsigned char[ol + EVP_CIPHER_CTX_block_size(ctx)];
        EVP_DecryptUpdate(ctx, ret, &tmp, cipher, ol);
        ol = tmp;
        if(!EVP_DecryptFinal(ctx, &ret[tmp], &tmp))
            return;
        ol += tmp;

        conversation.contact.setNickname(QString::fromUtf8((char*)ret, ol));
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

    FILE* pFile = NULL;
    pFile = tmpfile();

    PEM_write_RSAPublicKey(pFile, keys);

    QString tmp;
    rewind(pFile);

    char buffer[256];
    while(!feof(pFile))
    {
        if(fgets(buffer, 256, pFile) == NULL)
            break;
        tmp.append(buffer);
    }

    openTab(socket);

    out << (quint16) 0; // writing 0 while not knowing the size
    out << givePubK;
    out << tmp;
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

    Phonon::createPlayer(Phonon::NoCategory, Phonon::MediaSource(qApp->applicationDirPath() + "/sounds/Logout.mp3"))->play();
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

int SIS::seed_prng(int bytes)
{
    if (!RAND_load_file("/dev/random", bytes))
        return 0;
    return 1;
}

