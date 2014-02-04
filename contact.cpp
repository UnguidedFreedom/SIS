#include "contact.h"

Contact::Contact()
{
    nickname = "Other";
}

Contact::Contact(RSA* key)
    : pubKey(key)
{
    nickname = "Other";
}

RSA* Contact::getPubKey()
{
    return pubKey;
}

void Contact::setNickname(QString name)
{
    nickname = name;
}

QString Contact::getNickname()
{
    return nickname;
}
