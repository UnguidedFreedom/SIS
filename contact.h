/*
SIS - SIS Is Secure - safe instant messaging based on a non-central server architecture
Copyright (C) 2013-2014 Benoît Vernier <vernier.benoit@gmail.com>

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
