#ifndef FRIEND_H
#define FRIEND_H

#include <QString>
#include <cryptopp/rsa.h>

using namespace CryptoPP;

class Friend
{
public:
    Friend();
    Friend(RSA::PublicKey);
    RSA::PublicKey getPubKey();
    void setNickname(QString name);
    QString getNickname();

private:
    RSA::PublicKey pubKey;
    QString nickname;
};

#endif // FRIEND_H
