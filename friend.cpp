#include "friend.h"

Friend::Friend()
{
    nickname = "Other";
}

Friend::Friend(RSA* key)
    : pubKey(key)
{
    nickname = "Other";
}

RSA* Friend::getPubKey()
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
