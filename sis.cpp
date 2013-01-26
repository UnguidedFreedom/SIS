#include "sis.h"

SIS::SIS(QWidget *parent)
  : QMainWindow(parent)
{
    state = new QTextBrowser;

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

    window = new QTabsWidget;
  //  window->setTabsClosable(true);
    tabBar = window->tabBar();
    connect(tabBar, SIGNAL(currentChanged(int)), this, SLOT(clearColor(int)));

    // Initializing
    init = QCA::Initializer();

    //Checking if RSA is supported
    if(!QCA::isSupported("pkey"))
    {
        state->setText("pkey not supported");
        return;
    }
    if(!QCA::PKey::supportedIOTypes().contains(QCA::PKey::RSA))
    {
        state->setText("RSA not supported");
        return;
    }

    QElapsedTimer* timer = new QElapsedTimer;
    timer->start();

    if(!QFile::exists(qApp->applicationDirPath() + "/privatekey.pem"))
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
            //Saving the private key to a protected file
            QCA::SecureArray passPhrase = passOne.toUtf8();
            privateKey.toPEMFile(qApp->applicationDirPath() + "/privatekey.pem", passPhrase);
        }
        else
        {
            state->setText("No password given");
            return;
        }
    }
    else
    {
        //Reading the private key from the file
        QCA::ConvertResult conversionResult;
        QString pass = QInputDialog::getText(this, "Sign in", "Enter your password to sign in:", QLineEdit::Password);
        privateKey = QCA::PrivateKey::fromPEMFile(qApp->applicationDirPath() + "/privatekey.pem", pass.toUtf8(), &conversionResult);
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

    server = new QTcpServer;
    if(!server->listen(QHostAddress::Any, 50000))
    {
        state->setText("Unable to start network connection");
        qDebug() << "Unable to start network connection";
        return;
    }

    connect(server, SIGNAL(newConnection()), this, SLOT(newConversation()));
    messageSize = 0;

    state->setText(QString::number(timer->elapsed()) + " ms to launch program");
    button->setEnabled(true);
}

void SIS::transfer()
{
    QCA::InitializationVector iv = QCA::InitializationVector(16);

    if (!QCA::isSupported("blowfish-cbc"))
    {
      qDebug() << "Error using blowfish";
      return;
    }

    QLineEdit *edit = qobject_cast<QLineEdit*>(sender());
    if(edit == 0)
        return;

    if(edit->text().size() == 0)
        return;

    QTcpSocket* socket = edit_socket[edit];
    datas pairs = networkMap[socket];
    QTextBrowser* browser = pairs.browser;
    QCA::SymmetricKey key = pairs.key;

    //Encoding with the original key
    QCA::Cipher cipher = QCA::Cipher("blowfish", QCA::Cipher::CBC, QCA::Cipher::DefaultPadding, QCA::Encode, key, iv);

    QCA::SecureArray secureData = edit->text().toUtf8();
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

    browser->append("<span style='color:#204a87;'>(" + time.toString() + ") <b>Me:\t\t</b></span>" + edit->text());
    edit->clear();
}

void SIS::newConversation()
{
    qDebug() << "New conversation";
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

   // qDebug() << socket->socketDescriptor();

    QDataStream in(socket);

    if (messageSize == 0)
    {
        if (socket->bytesAvailable() < (int)sizeof(quint16)) // messageSize not entierly received
             return;

        in >> messageSize; // messageSize received
    }

    if(socket->bytesAvailable() < messageSize)
        return;

    int type;
    in >> type;

    QByteArray data;
    in >> data;

    datas pairs = networkMap[socket];

    int tabId = pairs.tabId;
    QTextBrowser* browser = pairs.browser;

    messageSize = 0;

    if(type == givePubK)
    {
        QString pubKey;
        char *datas = new char[data.size()];
        strcpy(datas, data.data());
        pubKey = datas;
        delete [] datas;

        networkMap[socket].pubKey = QCA::PublicKey::fromPEM(pubKey);

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

        networkMap[socket].pubKey = QCA::PublicKey::fromPEM(pubKey);

        delete [] received;

        QByteArray packet; // packet for sending the public key
        QDataStream out(&packet, QIODevice::WriteOnly);

        out << (quint16) 0; // writing 0 while not knowing the size
        out << giveBF;

        //Generating a symmetric key
        QCA::SymmetricKey key = QCA::SymmetricKey(16);
        networkMap[socket].key = key;

        QCA::SecureArray keyArr = key.toByteArray();

        //Encoding it
        QCA::SecureArray result = networkMap[socket].pubKey.encrypt(keyArr, QCA::EME_PKCS1_OAEP);

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
        networkMap[socket].key = key;

        QByteArray packet; // packet for sending the public key
        QDataStream out(&packet, QIODevice::WriteOnly);

        out << (quint16) 0; // writing 0 while not knowing the size
        out << replyBF;

        //Encoding it back
        QCA::SecureArray keyArr = key.toByteArray();
        QCA::SecureArray resultBis = networkMap[socket].pubKey.encrypt(keyArr, QCA::EME_PKCS1_OAEP);

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

        if(decrypt != pairs.key)
            socket->disconnectFromHost();
    }
    else if(type == text)
    {
        if(window->currentIndex() != tabId)
            tabBar->setTabTextColor(tabId, Qt::blue);

        if (!QCA::isSupported("blowfish-cbc"))
        {
          qDebug() << "Error using blowfish";
          return;
        }
        QByteArray initVector;
        in >> initVector;
        QCA::InitializationVector iv = initVector;
        QCA::Cipher cipher = QCA::Cipher("blowfish", QCA::Cipher::CBC, QCA::Cipher::DefaultPadding, QCA::Decode, pairs.key, iv);
        QCA::SecureArray datas = data;
        QCA::SecureArray decryptedData = cipher.process(datas);
        if(!cipher.ok())
        {
            qDebug() << "Decryption failed!";
            browser->append("<b>Decryption failed</b>");
            return;
        }

        QTime time = QDateTime::currentDateTime().time();

        browser->append("<span style='color:#cc0000'>(" + time.toString() + ") <b>Other:\t</b></span>" + QString::fromUtf8(decryptedData.data()));
        socket_edit[socket]->setFocus();
    }
}

void SIS::requestNewConnection()
{
    bool ok;
    QString ip = QInputDialog::getText(this, "IP address", "Enter the IP address (IP:port) to which you want to connect:", QLineEdit::Normal, "", &ok);
    if(!ok)
        return;
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

void SIS::clearColor(int tabId)
{
    tabBar->setTabTextColor(tabId, Qt::black);
}

void SIS::openTab(QTcpSocket *socket)
{
    QTextBrowser* browser = new QTextBrowser;
    QLineEdit* edit = new QLineEdit;
    connect(edit, SIGNAL(returnPressed()), this, SLOT(transfer()));
    QVBoxLayout* lay = new QVBoxLayout;
    lay->addWidget(browser);
    lay->addWidget(edit);
    QWidget* cont = new QWidget;
    cont->setLayout(lay);

    if(window->count() == 0)
    {
        window->show();
        window->setGeometry(2*QApplication::desktop()->width()/5, QApplication::desktop()->height()/4, QApplication::desktop()->width()/5, QApplication::desktop()->height()/2);
    }
    if(!window->isVisible())
    {

    }

    int tabId = window->addTab(cont, "Socket " + QString::number(socket->socketDescriptor()));
    tabBar->setTabTextColor(tabId, Qt::black);

    edit_socket.insert({edit,socket});
    socket_edit.insert({socket, edit});

    datas current;
    current.browser = browser;
    current.tabId = tabId;

    networkMap.insert({socket, current});
    tabMap.insert({tabId, {edit, socket}});

    edit->setFocus();
}
