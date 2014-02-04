#ifndef CONTACT_H
#define CONTACT_H

#include <QString>
#include <openssl/rsa.h>

class Contact
{
public:
    Contact();
    Contact(RSA*);
    RSA* getPubKey();
    void setNickname(QString name);
    QString getNickname();

private:
    RSA* pubKey;
    QString nickname;
};

#endif // CONTACT_H
