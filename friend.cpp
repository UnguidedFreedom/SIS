#include "friend.h"

Friend::Friend()
{
    nickname = "Other";
}

Friend::Friend(RSA::PublicKey key)
    : pubKey(key)
{
    nickname = "Other";
}

RSA::PublicKey Friend::getPubKey()
{
    return pubKey;
}

void Friend::setNickname(QString name)
{
    nickname = name;
}

QString Friend::getNickname()
{
    return nickname;
}
