#ifndef FRIEND_H
#define FRIEND_H

#include <QtCrypto/qca.h>

class Friend
{
public:
    Friend();
    Friend(QCA::PublicKey);
    QCA::PublicKey getPubKey();
    void setNickname(QString name);
    QString getNickname();

private:
    QCA::PublicKey pubKey;
    QString nickname;
};

#endif // FRIEND_H
