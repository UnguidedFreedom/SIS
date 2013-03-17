#include "friend.h"

Friend::Friend()
{
    nickname = "Other";
}

Friend::Friend(QCA::PublicKey key)
    : pubKey(key)
{
    nickname = "Other";
}

QCA::PublicKey Friend::getPubKey()
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
