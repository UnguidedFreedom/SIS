#ifndef FRIEND_H
#define FRIEND_H

#include <QString>
#include <openssl/rsa.h>

class Friend
{
public:
    Friend();
    Friend(RSA*);
    RSA* getPubKey();
    void setNickname(QString name);
    QString getNickname();

private:
    RSA* pubKey;
    QString nickname;
};

#endif // FRIEND_H
