#ifndef _IMAPINFO_H
#define _IMAPINFO_H
/**********************************************************************
 *
 *   imapinfo.h  - IMAP4rev1 SELECT / EXAMINE handler
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

#include <qstringlist.h>
#include <qstring.h>

//class handling the info we get on EXAMINE and SELECT
class imapInfo
{
public:


  enum MessageAttribute
  {
    Seen = 1 << 0,
    Answered = 1 << 1,
    Flagged = 1 << 2,
    Deleted = 1 << 3,
    Draft = 1 << 4,
    Recent = 1 << 5,
    User = 1 << 6,
    // non standard flags
    Forwarded = 1 << 7,
    Todo = 1 << 8,
    Watched = 1 << 9,
    Ignored = 1 << 10
  };


    imapInfo ();
    imapInfo (const QStringList &);
    imapInfo (const imapInfo &);
    imapInfo & operator = (const imapInfo &);

  static ulong _flags (const QCString &);

  void setCount (ulong l)
  {
    countAvailable_ = true;
    count_ = l;
  }

  void setRecent (ulong l)
  {
    recentAvailable_ = true;
    recent_ = l;
  }

  void setUnseen (ulong l)
  {
    unseenAvailable_ = true;
    unseen_ = l;
  }

  void setUidValidity (ulong l)
  {
    uidValidityAvailable_ = true;
    uidValidity_ = l;
  }

  void setUidNext (ulong l)
  {
    uidNextAvailable_ = true;
    uidNext_ = l;
  }

  void setFlags (ulong l)
  {
    flagsAvailable_ = true;
    flags_ = l;
  }

  void setFlags (const QCString & inFlag)
  {
    flagsAvailable_ = true;
    flags_ = _flags (inFlag);
  }

  void setPermanentFlags (ulong l)
  {
    permanentFlagsAvailable_ = true;
    permanentFlags_ = l;
  }

  void setPermanentFlags (const QCString & inFlag)
  {
    permanentFlagsAvailable_ = true;
    permanentFlags_ = _flags (inFlag);
  }

  void setReadWrite (bool b)
  {
    readWriteAvailable_ = true;
    readWrite_ = b;
  }

  void setAlert( const char* cstr )
  {
    alert_ = cstr;
  }

  ulong count () const
  {
    return count_;
  }

  ulong recent () const
  {
    return recent_;
  }

  ulong unseen () const
  {
    return unseen_;
  }

  ulong uidValidity () const
  {
    return uidValidity_;
  }

  ulong uidNext () const
  {
    return uidNext_;
  }

  ulong flags () const
  {
    return flags_;
  }

  ulong permanentFlags () const
  {
    return permanentFlags_;
  }

  bool readWrite () const
  {
    return readWrite_;
  }

  ulong countAvailable () const
  {
    return countAvailable_;
  }

  ulong recentAvailable () const
  {
    return recentAvailable_;
  }

  ulong unseenAvailable () const
  {
    return unseenAvailable_;
  }

  ulong uidValidityAvailable () const
  {
    return uidValidityAvailable_;
  }

  ulong uidNextAvailable () const
  {
    return uidNextAvailable_;
  }

  ulong flagsAvailable () const
  {
    return flagsAvailable_;
  }

  ulong permanentFlagsAvailable () const
  {
    return permanentFlagsAvailable_;
  }

  bool readWriteAvailable () const
  {
    return readWriteAvailable_;
  }

  QCString alert() const
  {
    return alert_;
  }

private:

  QCString alert_;

  ulong count_;
  ulong recent_;
  ulong unseen_;
  ulong uidValidity_;
  ulong uidNext_;
  ulong flags_;
  ulong permanentFlags_;
  bool readWrite_;

  bool countAvailable_;
  bool recentAvailable_;
  bool unseenAvailable_;
  bool uidValidityAvailable_;
  bool uidNextAvailable_;
  bool flagsAvailable_;
  bool permanentFlagsAvailable_;
  bool readWriteAvailable_;
};

#endif
