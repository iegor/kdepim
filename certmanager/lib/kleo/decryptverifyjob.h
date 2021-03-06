/*
    decryptverifyjob.h

    This file is part of libkleopatra, the KDE keymanagement library
    Copyright (c) 2004 Klarälvdalens Datakonsult AB

    Libkleopatra is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.

    Libkleopatra is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifndef __KLEO_DECRYPTVERIFYJOB_H__
#define __KLEO_DECRYPTVERIFYJOB_H__

#include "job.h"

#include <qcstring.h>

#include <utility>

namespace GpgME {
  class Error;
  class Key;
  class DecryptionResult;
  class VerificationResult;
}


namespace Kleo {

  /**
     @short An abstract base class for asynchronous combined decrypters and verifiers

     To use a DecryptVerifyJob, first obtain an instance from the
     CryptoBackend implementation, connect the progress() and result()
     signals to suitable slots and then start the operation with a
     call to start(). This call might fail, in which case the
     DecryptVerifyJob instance will have scheduled it's own destruction with
     a call to QObject::deleteLater().

     After result() is emitted, the DecryptVerifyJob will schedule it's own
     destruction by calling QObject::deleteLater().
  */
  class DecryptVerifyJob : public Job {
    Q_OBJECT
  protected:
    DecryptVerifyJob( QObject * parent, const char * name );
  public:
    ~DecryptVerifyJob();

    /**
       Starts the combined decryption and verification operation.
       \a cipherText is the data to decrypt and later verify.
    */
    virtual GpgME::Error start( const QByteArray & cipherText ) = 0;

    /** Synchronous equivalent of start() */
    virtual std::pair<GpgME::DecryptionResult,GpgME::VerificationResult>
      exec( const QByteArray & cipherText, QByteArray & plainText ) = 0;

  signals:
    void result( const GpgME::DecryptionResult & decryptionresult,
		 const GpgME::VerificationResult & verificationresult,
		 const QByteArray & plainText );
  };

}

#endif // __KLEO_DECRYPTVERIFYJOB_H__
