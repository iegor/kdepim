/**********************************************************************
 *
 *   mailaddress.cc  - mail address parser
 *   Copyright (C) 2000 Sven Carstens
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *   Send comments and bug fixes to
 *
 *********************************************************************/


#include "mailaddress.h"
#include "rfcdecoder.h"
#include "mimehdrline.h"
#include <kmime_util.h>

mailAddress::mailAddress ()
{
}

mailAddress::mailAddress (const mailAddress & lr):
user (lr.user),
host (lr.host),
rawFullName (lr.rawFullName),
rawComment (lr.rawComment)
{
//  kdDebug(7116) << "mailAddress::mailAddress - " << getStr() << endl;
}

mailAddress & mailAddress::operator = (const mailAddress & lr)
{
  // Avoid a = a.
  if (this == &lr)
    return *this;

  user = lr.user;
  host = lr.host;
  rawFullName = lr.rawFullName;
  rawComment = lr.rawComment;

//  kdDebug(7116) << "mailAddress::operator= - " << getStr() << endl;

  return *this;
}




mailAddress::~mailAddress ()
{
}

mailAddress::mailAddress (char *aCStr)
{
  parseAddress (aCStr);
}

int
mailAddress::parseAddress (char *aCStr)
{
  int retVal = 0;
  int skip;
  uint len;
  int pt;

  if (aCStr)
  {
    //skip leading white space
    skip = mimeHdrLine::skipWS ((const char *) aCStr);
    if (skip > 0)
    {
      aCStr += skip;
      retVal += skip;
    }
    while (*aCStr)
    {
      int advance;

      switch (*aCStr)
      {
      case '"':
        advance = mimeHdrLine::parseQuoted ('"', '"', aCStr);
        rawFullName += QCString (aCStr, advance + 1);
        break;
      case '(':
        advance = mimeHdrLine::parseQuoted ('(', ')', aCStr);
        rawComment += QCString (aCStr, advance + 1);
        break;
      case '<':
        advance = mimeHdrLine::parseQuoted ('<', '>', aCStr);
        user = QCString (aCStr, advance + 1); // copy it
        len = advance;
        user = user.mid (1, len - 2);  // strip <>
        len -= 2;
        pt = user.find('@');
        host = user.right (len - pt - 1); // split it into host
        user.truncate(pt); // and user
        break;
      default:
        advance = mimeHdrLine::parseWord ((const char *) aCStr);
        //if we've seen a FQ mailname the rest must be quoted or is just junk
        if (user.isEmpty ())
        {
          if (*aCStr != ',')
          {
            rawFullName += QCString (aCStr, advance + 1);
            if (mimeHdrLine::skipWS ((const char *) &aCStr[advance]) > 0)
            {
              rawFullName += ' ';
            }
          }
        }
        break;
      }
      if (advance)
      {
        retVal += advance;
        aCStr += advance;
      }
      else
        break;
      advance = mimeHdrLine::skipWS ((const char *) aCStr);
      if (advance > 0)
      {
        retVal += advance;
        aCStr += advance;
      }
      //reached end of current address
      if (*aCStr == ',')
      {
        advance++;
        break;
      }
    }
    //let's see what we've got
    if (rawFullName.isEmpty ())
    {
      if (user.isEmpty ())
        retVal = 0;
      else
      {
        if (host.isEmpty ())
        {
          rawFullName = user;
          user.truncate(0);
        }
      }
    }
    else if (user.isEmpty ())
    {
      pt = rawFullName.find ('@');
      if (pt >= 0)
      {
        user = rawFullName;
        host = user.right (user.length () - pt - 1);
        user.truncate(pt);
        rawFullName.truncate(0);
      }
    }

#if 0
// dead
    if (!rawFullName.isEmpty ())
    {
//      if(fullName[0] == '"')
//        fullName = fullName.mid(1,fullName.length()-2);
//      fullName = fullName.simplifyWhiteSpace().stripWhiteSpace();
//      fullName = rfcDecoder::decodeRFC2047String(fullName.ascii());
    }
#endif
    if (!rawComment.isEmpty ())
    {
      if (rawComment[0] == '(')
        rawComment = rawComment.mid (1, rawComment.length () - 2);
      rawComment = rawComment.stripWhiteSpace ();
//      comment = rfcDecoder::decodeRFC2047String(comment.ascii());
    }
  }
  else
  {
    //debug();
  }
  return retVal;
}

const QCString
mailAddress::getStr ()
{
  QCString retVal(128); // Should be generally big enough

  if (!rawFullName.isEmpty ())
  {
    KMime::addQuotes( rawFullName, false );
    retVal = rawFullName + " ";
  }
  if (!user.isEmpty ())
  {
    retVal += '<';
    retVal += user;
    if (!host.isEmpty ()) {
      retVal += '@';
      retVal += host;
    }
    retVal += '>';
  }
  if (!rawComment.isEmpty ())
  {
    retVal = '(' + rawComment + ')';
  }
//  kdDebug(7116) << "mailAddress::getStr - '" << retVal << "'" << endl;
  return retVal;
}

bool
mailAddress::isEmpty () const
{
  return user.isEmpty ();
}

void
mailAddress::setFullName (const QString & _str)
{
  rawFullName = rfcDecoder::encodeRFC2047String (_str).latin1 ();
}
const QString
mailAddress::getFullName () const
{
  return rfcDecoder::decodeRFC2047String (rawFullName);
}

void
mailAddress::setCommentRaw (const QCString & _str)
{
  rawComment = _str;
}

void
mailAddress::setComment (const QString & _str)
{
  rawComment = rfcDecoder::encodeRFC2047String (_str).latin1 ();
}
const QString
mailAddress::getComment () const
{
  return rfcDecoder::decodeRFC2047String (rawComment);
}

const QCString &
mailAddress::getCommentRaw () const
{
  return rawComment;
}

QString
mailAddress::emailAddrAsAnchor (const mailAddress & adr, bool shortAdr)
{
  QString retVal;
  if (!adr.getFullName ().isEmpty ())
  {
    // should do some umlaut escaping
    retVal += adr.getFullName () + " ";
  }
  if (!adr.getUser ().isEmpty () && !shortAdr)
  {
    retVal += "&lt;" + adr.getUser ();
    if (!adr.getHost ().isEmpty ())
      retVal += "@" + adr.getHost ();
    retVal += "&gt; ";
  }
  if (!adr.getComment ().isEmpty ())
  {
    // should do some umlaut escaping
    retVal = '(' + adr.getComment () + ')';
  }

  if (!adr.getUser ().isEmpty ())
  {
    QString mail;
    mail = adr.getUser ();
    if (!mail.isEmpty () && !adr.getHost ().isEmpty ())
      mail += "@" + adr.getHost ();
    if (!mail.isEmpty ())
      retVal = "<A HREF=\"mailto:" + mail + "\">" + retVal + "</A>";
  }
  return retVal;
}

QString
mailAddress::emailAddrAsAnchor (const QPtrList < mailAddress > &list, bool value)
{
  QString retVal;
  QPtrListIterator < mailAddress > it (list);

  while (it.current ())
  {
    retVal += emailAddrAsAnchor ((*it.current ()), value) + "<BR></BR>\n";
    ++it;
  }

  return retVal;
}


void mailAddress::clear() {
  user.truncate(0);
  host.truncate(0);
  rawFullName.truncate(0);
  rawComment.truncate(0);
}

