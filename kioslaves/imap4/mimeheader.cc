/***************************************************************************
                          mimeheader.cc  -  description
                             -------------------
    begin                : Fri Oct 20 2000
    copyright            : (C) 2000 by Sven Carstens
    email                : s.carstens@gmx.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "mimeheader.h"
#include "mimehdrline.h"
#include "mailheader.h"
#include "rfcdecoder.h"

#include <qregexp.h>

// #include <iostream.h>
#include <kglobal.h>
#include <kinstance.h>
#include <kiconloader.h>
#include <kmimetype.h>
#include <kmimemagic.h>
#include <kmdcodec.h>
#include <kdebug.h>

mimeHeader::mimeHeader ():
typeList (17, false), dispositionList (17, false)
{
  // Case insensitive hashes are killing us.  Also are they too small?
  originalHdrLines.setAutoDelete (true);
  additionalHdrLines.setAutoDelete (false); // is also in original lines
  nestedParts.setAutoDelete (true);
  typeList.setAutoDelete (true);
  dispositionList.setAutoDelete (true);
  nestedMessage = NULL;
  contentLength = 0;
  contentType = "application/octet-stream";
}

mimeHeader::~mimeHeader ()
{
}

/*
QPtrList<mimeHeader> mimeHeader::getAllParts()
{
	QPtrList<mimeHeader> retVal;

	// caller is responsible for clearing
	retVal.setAutoDelete( false );
	nestedParts.setAutoDelete( false );

	// shallow copy
	retVal = nestedParts;

	// can't have duplicate pointers
	nestedParts.clear();

	// restore initial state
	nestedParts.setAutoDelete( true );

	return retVal;
} */

void
mimeHeader::addHdrLine (mimeHdrLine * aHdrLine)
{
  mimeHdrLine *addLine = new mimeHdrLine (aHdrLine);
  if (addLine)
  {
    originalHdrLines.append (addLine);
    if (qstrnicmp (addLine->getLabel (), "Content-", 8))
    {
      additionalHdrLines.append (addLine);
    }
    else
    {
      int skip;
      char *aCStr = addLine->getValue ().data ();
      QDict < QString > *aList = 0;

      skip = mimeHdrLine::parseSeparator (';', aCStr);
      if (skip > 0)
      {
        int cut = 0;
        if (skip >= 2)
        {
          if (aCStr[skip - 1] == '\r')
            cut++;
          if (aCStr[skip - 1] == '\n')
            cut++;
          if (aCStr[skip - 2] == '\r')
            cut++;
          if (aCStr[skip - 1] == ';')
            cut++;
        }
        QCString mimeValue = QCString (aCStr, skip - cut + 1);  // cutting of one because of 0x00


        if (!qstricmp (addLine->getLabel (), "Content-Disposition"))
        {
          aList = &dispositionList;
          _contentDisposition = mimeValue;
        }
        else if (!qstricmp (addLine->getLabel (), "Content-Type"))
        {
          aList = &typeList;
          contentType = mimeValue;
        }
        else
          if (!qstricmp (addLine->getLabel (), "Content-Transfer-Encoding"))
        {
          contentEncoding = mimeValue;
        }
        else if (!qstricmp (addLine->getLabel (), "Content-ID"))
        {
          contentID = mimeValue;
        }
        else if (!qstricmp (addLine->getLabel (), "Content-Description"))
        {
          _contentDescription = mimeValue;
        }
        else if (!qstricmp (addLine->getLabel (), "Content-MD5"))
        {
          contentMD5 = mimeValue;
        }
        else if (!qstricmp (addLine->getLabel (), "Content-Length"))
        {
          contentLength = mimeValue.toULong ();
        }
        else
        {
          additionalHdrLines.append (addLine);
        }
//        cout << addLine->getLabel().data() << ": '" << mimeValue.data() << "'" << endl;

        aCStr += skip;
        while ((skip = mimeHdrLine::parseSeparator (';', aCStr)))
        {
          if (skip > 0)
          {
            addParameter (QCString (aCStr, skip).simplifyWhiteSpace(), aList);
//            cout << "-- '" << aParm.data() << "'" << endl;
            mimeValue = QCString (addLine->getValue ().data (), skip);
            aCStr += skip;
          }
          else
            break;
        }
      }
    }
  }
}

void
mimeHeader::addParameter (const QCString& aParameter, QDict < QString > *aList)
{
  if ( !aList )
    return;

  QString *aValue;
  QCString aLabel;
  int pos = aParameter.find ('=');
//  cout << aParameter.left(pos).data();
  aValue = new QString ();
  aValue->setLatin1 (aParameter.right (aParameter.length () - pos - 1));
  aLabel = aParameter.left (pos);
  if ((*aValue)[0] == '"')
    *aValue = aValue->mid (1, aValue->length () - 2);

  aList->insert (aLabel, aValue);
//  cout << "=" << aValue->data() << endl;
}

QString
mimeHeader::getDispositionParm (const QCString& aStr)
{
  return getParameter (aStr, &dispositionList);
}

QString
mimeHeader::getTypeParm (const QCString& aStr)
{
  return getParameter (aStr, &typeList);
}

void
mimeHeader::setDispositionParm (const QCString& aLabel, const QString& aValue)
{
  setParameter (aLabel, aValue, &dispositionList);
  return;
}

void
mimeHeader::setTypeParm (const QCString& aLabel, const QString& aValue)
{
  setParameter (aLabel, aValue, &typeList);
}

QDictIterator < QString > mimeHeader::getDispositionIterator ()
{
  return QDictIterator < QString > (dispositionList);
}

QDictIterator < QString > mimeHeader::getTypeIterator ()
{
  return QDictIterator < QString > (typeList);
}

QPtrListIterator < mimeHdrLine > mimeHeader::getOriginalIterator ()
{
  return QPtrListIterator < mimeHdrLine > (originalHdrLines);
}

QPtrListIterator < mimeHdrLine > mimeHeader::getAdditionalIterator ()
{
  return QPtrListIterator < mimeHdrLine > (additionalHdrLines);
}

void
mimeHeader::outputHeader (mimeIO & useIO)
{
  if (!getDisposition ().isEmpty ())
  {
    useIO.outputMimeLine (QCString ("Content-Disposition: ")
                          + getDisposition ()
                          + outputParameter (&dispositionList));
  }

  if (!getType ().isEmpty ())
  {
    useIO.outputMimeLine (QCString ("Content-Type: ")
                          + getType () + outputParameter (&typeList));
  }
  if (!getDescription ().isEmpty ())
    useIO.outputMimeLine (QCString ("Content-Description: ") +
                          getDescription ());
  if (!getID ().isEmpty ())
    useIO.outputMimeLine (QCString ("Content-ID: ") + getID ());
  if (!getMD5 ().isEmpty ())
    useIO.outputMimeLine (QCString ("Content-MD5: ") + getMD5 ());
  if (!getEncoding ().isEmpty ())
    useIO.outputMimeLine (QCString ("Content-Transfer-Encoding: ") +
                          getEncoding ());

  QPtrListIterator < mimeHdrLine > ait = getAdditionalIterator ();
  while (ait.current ())
  {
    useIO.outputMimeLine (ait.current ()->getLabel () + ": " +
                          ait.current ()->getValue ());
    ++ait;
  }
  useIO.outputMimeLine (QCString (""));
}

QString
mimeHeader::getParameter (const QCString& aStr, QDict < QString > *aDict)
{
  QString retVal, *found;
  if (aDict)
  {
    //see if it is a normal parameter
    found = aDict->find (aStr);
    if (!found)
    {
      //might be a continuated or encoded parameter
      found = aDict->find (aStr + "*");
      if (!found)
      {
        //continuated parameter
        QString decoded, encoded;
        int part = 0;

        do
        {
          QCString search;
          search.setNum (part);
          search = aStr + "*" + search;
          found = aDict->find (search);
          if (!found)
          {
            found = aDict->find (search + "*");
            if (found)
              encoded += rfcDecoder::encodeRFC2231String (*found);
          }
          else
          {
            encoded += *found;
          }
          part++;
        }
        while (found);
        if (encoded.find ('\'') >= 0)
        {
          retVal = rfcDecoder::decodeRFC2231String (encoded.local8Bit ());
        }
        else
        {
          retVal =
            rfcDecoder::decodeRFC2231String (QCString ("''") +
                                             encoded.local8Bit ());
        }
      }
      else
      {
        //simple encoded parameter
        retVal = rfcDecoder::decodeRFC2231String (found->local8Bit ());
      }
    }
    else
    {
      retVal = *found;
    }
  }
  return retVal;
}

void
mimeHeader::setParameter (const QCString& aLabel, const QString& aValue,
                          QDict < QString > *aDict)
{
  bool encoded = true;
  uint vlen, llen;
  QString val = aValue;

  if (aDict)
  {

    //see if it needs to get encoded
    if (encoded && aLabel.find ('*') == -1)
    {
      val = rfcDecoder::encodeRFC2231String (aValue);
    }
    //kdDebug(7116) << "mimeHeader::setParameter() - val = '" << val << "'" << endl;
    //see if it needs to be truncated
    vlen = val.length();
    llen = aLabel.length();
    if (vlen + llen + 4 > 80 && llen < 80 - 8 - 2 )
    {
      const int limit = 80 - 8 - 2 - (int)llen;
      // the -2 is there to allow extending the length of a part of val
      // by 1 or 2 in order to prevent an encoded character from being
      // split in half
      int i = 0;
      QString shortValue;
      QCString shortLabel;

      while (!val.isEmpty ())
      {
        int partLen; // the length of the next part of the value
        if ( limit >= int(vlen) ) {
          // the rest of the value fits completely into one continued header
          partLen = vlen;
        }
        else {
          partLen = limit;
          // make sure that we don't split an encoded char in half
          if ( val[partLen-1] == '%' ) {
            partLen += 2;
          }
          else if ( partLen > 1 && val[partLen-2] == '%' ) {
            partLen += 1;
          }
          // make sure partLen does not exceed vlen (could happen in case of
          // an incomplete encoded char)
          if ( partLen > int(vlen) ) {
            partLen = vlen;
          }
        }
        shortValue = val.left( partLen );
        shortLabel.setNum (i);
        shortLabel = aLabel + "*" + shortLabel;
        val = val.right( vlen - partLen );
        vlen = vlen - partLen;
        if (encoded)
        {
          if (i == 0)
          {
            shortValue = "''" + shortValue;
          }
          shortLabel += "*";
        }
        //kdDebug(7116) << "mimeHeader::setParameter() - shortLabel = '" << shortLabel << "'" << endl;
        //kdDebug(7116) << "mimeHeader::setParameter() - shortValue = '" << shortValue << "'" << endl;
        //kdDebug(7116) << "mimeHeader::setParameter() - val        = '" << val << "'" << endl;
        aDict->insert (shortLabel, new QString (shortValue));
        i++;
      }
    }
    else
    {
      aDict->insert (aLabel, new QString (val));
    }
  }
}

QCString
mimeHeader::outputParameter (QDict < QString > *aDict)
{
  QCString retVal;
  if (aDict)
  {
    QDictIterator < QString > it (*aDict);
    while (it.current ())
    {
      retVal += (";\n\t" + it.currentKey () + "=").latin1 ();
      if (it.current ()->find (' ') > 0 || it.current ()->find (';') > 0)
      {
        retVal += '"' + it.current ()->utf8 () + '"';
      }
      else
      {
        retVal += it.current ()->utf8 ();
      }
      // << it.current()->utf8() << "'";
      ++it;
    }
    retVal += "\n";
  }
  return retVal;
}

void
mimeHeader::outputPart (mimeIO & useIO)
{
  QPtrListIterator < mimeHeader > nestedParts = getNestedIterator ();
  QCString boundary;
  if (!getTypeParm ("boundary").isEmpty ())
    boundary = getTypeParm ("boundary").latin1 ();

  outputHeader (useIO);
  if (!getPreBody ().isEmpty ())
    useIO.outputMimeLine (getPreBody ());
  if (getNestedMessage ())
    getNestedMessage ()->outputPart (useIO);
  while (nestedParts.current ())
  {
    if (!boundary.isEmpty ())
      useIO.outputMimeLine ("--" + boundary);
    nestedParts.current ()->outputPart (useIO);
    ++nestedParts;
  }
  if (!boundary.isEmpty ())
    useIO.outputMimeLine ("--" + boundary + "--");
  if (!getPostBody ().isEmpty ())
    useIO.outputMimeLine (getPostBody ());
}

int
mimeHeader::parsePart (mimeIO & useIO, const QString& boundary)
{
  int retVal = 0;
  bool mbox = false;
  QCString preNested, postNested;
  mbox = parseHeader (useIO);

  kdDebug(7116) << "mimeHeader::parsePart - parsing part '" << getType () << "'" << endl;
  if (!qstrnicmp (getType (), "Multipart", 9))
  {
    retVal = parseBody (useIO, preNested, getTypeParm ("boundary"));  //this is a message in mime format stuff
    setPreBody (preNested);
    int localRetVal;
    do
    {
      mimeHeader *aHeader = new mimeHeader;

      // set default type for multipart/digest
      if (!qstrnicmp (getType (), "Multipart/Digest", 16))
        aHeader->setType ("Message/RFC822");

      localRetVal = aHeader->parsePart (useIO, getTypeParm ("boundary"));
      addNestedPart (aHeader);
    }
    while (localRetVal);        //get nested stuff
  }
  if (!qstrnicmp (getType (), "Message/RFC822", 14))
  {
    mailHeader *msgHeader = new mailHeader;
    retVal = msgHeader->parsePart (useIO, boundary);
    setNestedMessage (msgHeader);
  }
  else
  {
    retVal = parseBody (useIO, postNested, boundary, mbox); //just a simple part remaining
    setPostBody (postNested);
  }
  return retVal;
}

int
mimeHeader::parseBody (mimeIO & useIO, QCString & messageBody,
                       const QString& boundary, bool mbox)
{
  QCString inputStr;
  QCString buffer;
  QString partBoundary;
  QString partEnd;
  int retVal = 0;               //default is last part

  if (!boundary.isEmpty ())
  {
    partBoundary = QString ("--") + boundary;
    partEnd = QString ("--") + boundary + "--";
  }

  while (useIO.inputLine (inputStr))
  {
    //check for the end of all parts
    if (!partEnd.isEmpty ()
        && !qstrnicmp (inputStr, partEnd.latin1 (), partEnd.length () - 1))
    {
      retVal = 0;               //end of these parts
      break;
    }
    else if (!partBoundary.isEmpty ()
             && !qstrnicmp (inputStr, partBoundary.latin1 (),
                            partBoundary.length () - 1))
    {
      retVal = 1;               //continue with next part
      break;
    }
    else if (mbox && inputStr.find ("From ") == 0)
    {
      retVal = 0;               // end of mbox
      break;
    }
    buffer += inputStr;
    if (buffer.length () > 16384)
    {
      messageBody += buffer;
      buffer = "";
    }
  }

  messageBody += buffer;
  return retVal;
}

bool
mimeHeader::parseHeader (mimeIO & useIO)
{
  bool mbox = false;
  bool first = true;
  mimeHdrLine my_line;
  QCString inputStr;

  kdDebug(7116) << "mimeHeader::parseHeader - starting parsing" << endl;
  while (useIO.inputLine (inputStr))
  {
    int appended;
    if (inputStr.find ("From ") != 0 || !first)
    {
      first = false;
      appended = my_line.appendStr (inputStr);
      if (!appended)
      {
        addHdrLine (&my_line);
        appended = my_line.setStr (inputStr);
      }
      if (appended <= 0)
        break;
    }
    else
    {
      mbox = true;
      first = false;
    }
    inputStr = (const char *) NULL;
  }

  kdDebug(7116) << "mimeHeader::parseHeader - finished parsing" << endl;
  return mbox;
}

mimeHeader *
mimeHeader::bodyPart (const QString & _str)
{
  // see if it is nested a little deeper
  int pt = _str.find('.');
  if (pt != -1)
  {
    QString tempStr = _str;
    mimeHeader *tempPart;

    tempStr = _str.right (_str.length () - pt - 1);
    if (nestedMessage)
    {
      kdDebug(7116) << "mimeHeader::bodyPart - recursing message" << endl;
      tempPart = nestedMessage->nestedParts.at (_str.left(pt).toULong() - 1);
    }
    else
    {
      kdDebug(7116) << "mimeHeader::bodyPart - recursing mixed" << endl;
      tempPart = nestedParts.at (_str.left(pt).toULong() - 1);
    }
    if (tempPart)
      tempPart = tempPart->bodyPart (tempStr);
    return tempPart;
  }

  kdDebug(7116) << "mimeHeader::bodyPart - returning part " << _str << endl;
  // or pick just the plain part
  if (nestedMessage)
  {
    kdDebug(7116) << "mimeHeader::bodyPart - message" << endl;
    return nestedMessage->nestedParts.at (_str.toULong () - 1);
  }
  kdDebug(7116) << "mimeHeader::bodyPart - mixed" << endl;
  return nestedParts.at (_str.toULong () - 1);
}

void mimeHeader::serialize(QDataStream& stream)
{
  int nestedcount = nestedParts.count();
  if (nestedParts.isEmpty() && nestedMessage)
    nestedcount = 1;
  stream << nestedcount << contentType << QString (getTypeParm ("name")) << _contentDescription
    << _contentDisposition << contentEncoding << contentLength << partSpecifier;
  // serialize nested message
  if (nestedMessage)
    nestedMessage->serialize(stream);

  // serialize nested parts
  if (!nestedParts.isEmpty())
  {
    QPtrListIterator < mimeHeader > it(nestedParts);
    mimeHeader* part;
    while ( (part = it.current()) != 0 )
    {
      ++it;
      part->serialize(stream);
    }
  }
}

#ifdef KMAIL_COMPATIBLE
// compatibility subroutines
QString
mimeHeader::bodyDecoded ()
{
  kdDebug(7116) << "mimeHeader::bodyDecoded" << endl;
  QByteArray temp;

  temp = bodyDecodedBinary ();
  return QString::fromLatin1 (temp.data (), temp.count ());
}

QByteArray
mimeHeader::bodyDecodedBinary ()
{
  QByteArray retVal;

  if (contentEncoding.find ("quoted-printable", 0, false) == 0)
    retVal = KCodecs::quotedPrintableDecode(postMultipartBody);
  else if (contentEncoding.find ("base64", 0, false) == 0)
    KCodecs::base64Decode(postMultipartBody, retVal);
  else retVal = postMultipartBody;

  kdDebug(7116) << "mimeHeader::bodyDecodedBinary - size is " << retVal.size () << endl;
  return retVal;
}

void
mimeHeader::setBodyEncodedBinary (const QByteArray & _arr)
{
  setBodyEncoded (_arr);
}

void
mimeHeader::setBodyEncoded (const QByteArray & _arr)
{
  QByteArray setVal;

  kdDebug(7116) << "mimeHeader::setBodyEncoded - in size " << _arr.size () << endl;
  if (contentEncoding.find ("quoted-printable", 0, false) == 0)
    setVal = KCodecs::quotedPrintableEncode(_arr);
  else if (contentEncoding.find ("base64", 0, false) == 0)
    KCodecs::base64Encode(_arr, setVal);
  else
    setVal.duplicate (_arr);
  kdDebug(7116) << "mimeHeader::setBodyEncoded - out size " << setVal.size () << endl;

  postMultipartBody.duplicate (setVal);
  kdDebug(7116) << "mimeHeader::setBodyEncoded - out size " << postMultipartBody.size () << endl;
}

QString
mimeHeader::iconName ()
{
  QString fileName;

  // FIXME: bug?  Why throw away this data?
  fileName =
    KMimeType::mimeType (contentType.lower ())->icon (QString::null, false);
  fileName =
    KGlobal::instance ()->iconLoader ()->iconPath (fileName, KIcon::Desktop);
//  if (fileName.isEmpty())
//    fileName = KGlobal::instance()->iconLoader()->iconPath( "unknown", KIcon::Desktop );
  return fileName;
}

void
mimeHeader::setNestedMessage (mailHeader * inPart, bool destroy)
{
//  if(nestedMessage && destroy) delete nestedMessage;
  nestedMessage = inPart;
}

QString
mimeHeader::headerAsString ()
{
  mimeIOQString myIO;

  outputHeader (myIO);
  return myIO.getString ();
}

QString
mimeHeader::magicSetType (bool aAutoDecode)
{
  QString mimetype;
  QByteArray body;
  KMimeMagicResult *result;

  KMimeMagic::self ()->setFollowLinks (TRUE); // is it necessary ?

  if (aAutoDecode)
    body = bodyDecodedBinary ();
  else
    body = postMultipartBody;

  result = KMimeMagic::self ()->findBufferType (body);
  mimetype = result->mimeType ();
  contentType = mimetype;
  return mimetype;
}
#endif
