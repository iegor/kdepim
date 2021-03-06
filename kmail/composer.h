/* -*- mode: C++; c-file-style: "gnu" -*-
 * KMComposeWin Header File
 * Author: Markus Wuebben <markus.wuebben@kde.org>
 */
#ifndef __KMAIL_COMPOSER_H__
#define __KMAIL_COMPOSER_H__

#include "secondarywindow.h"

#include <kurl.h>
#include <kglobalsettings.h>

#include <qstring.h>
#include <qcstring.h>

class KMMessage;
class KMFolder;
class KMMessagePart;
class QListViewItem;
class MailComposerIface;

namespace KIO {
  class Job;
}

namespace GpgME {
  class Error;
}

namespace KMail {

  class Composer;

  Composer * makeComposer( KMMessage * msg=0, uint identity=0 );

  class Composer : public KMail::SecondaryWindow {
    Q_OBJECT
  protected:
    Composer( const char * name=0 ) : KMail::SecondaryWindow( name ) {}
  public: // mailserviceimpl
    /**
     * From MailComposerIface
     */
    virtual void send( int how ) = 0;
    virtual void addAttachmentsAndSend(const KURL::List &urls, const QString &comment, int how) = 0;
    virtual void addAttachment( KURL url, QString comment ) = 0;
    virtual void addAttachment( const QString & name,
                                const QCString & cte,
                                const QByteArray & data,
                                const QCString & type,
                                const QCString & subType,
                                const QCString & paramAttr,
                                const QString & paramValue,
                                const QCString & contDisp) = 0;
  public: // kmcommand
    virtual void setBody( QString body ) = 0;

    virtual const MailComposerIface * asMailComposerIFace() const = 0;
    virtual MailComposerIface * asMailComposerIFace() = 0;

  public: // kmkernel, kmcommands, callback
    /**
     * Set the message the composer shall work with. This discards
     * previous messages without calling applyChanges() on them before.
     */
    virtual void setMsg( KMMessage * newMsg, bool mayAutoSign=true,
                         bool allowDecryption=false, bool isModified=false) = 0;

  public: // kmkernel
    /**
     * Set the filename which is used for autosaving.
     */
    virtual void setAutoSaveFilename( const QString & filename ) = 0;

  public: // kmkernel, callback
    /**
     * If this flag is set the message of the composer is deleted when
     * the composer is closed and the message was not sent. Default: FALSE
     */
    virtual void setAutoDelete( bool f ) = 0;

    /**
     * If this flag is set, the compose window will delete itself after
     * the window has been closed.
     */
    virtual void setAutoDeleteWindow( bool f ) = 0;

  public: // kmcommand
    /**
     * If this folder is set, the original message is inserted back after
     * cancelling
     */
    virtual void setFolder( KMFolder * aFolder ) = 0;

  public: // kmkernel, kmcommand, mailserviceimpl
    /**
     * Recode to the specified charset
     */
    virtual void setCharset( const QCString & aCharset, bool forceDefault=false ) = 0;

  public: // kmcommand
    /**
     * Sets the focus to the edit-widget and the cursor below the
     * "On ... you wrote" line when hasMessage is true.
     * Make sure you call this _after_ setMsg().
     */
    virtual void setReplyFocus( bool hasMessage=true ) = 0;

    /**
     * Sets the focus to the subject line edit. For use when creating a
     * message to a known recipient.
     */
    virtual void setFocusToSubject() = 0;

  public: // callback
    /** Disabled signing and encryption completely for this composer window. */
    virtual void setSigningAndEncryptionDisabled( bool v ) = 0;

  public slots: // kmkernel, callback
    virtual void slotSendNow() = 0;

  public slots: // kmkernel
    /**
       Tell the composer to always send the message, even if the user
       hasn't changed the next. This is useful if a message is
       autogenerated (e.g., via a DCOP call), and the user should
       simply be able to confirm the message and send it.
    */
    virtual void slotSetAlwaysSend( bool bAlwaysSend ) = 0;
  public slots: // kmkernel, callback
    /**
     * Switch wordWrap on/off
     */
    virtual void slotWordWrapToggled(bool) = 0;

  public slots: // kmkernel
    virtual void autoSaveMessage() = 0;

  public: // kmkernel, attachmentlistview
    virtual bool addAttach( const KURL url ) = 0;

    virtual void disableWordWrap() = 0;

  public: // kmcommand
    /**
     * Add an attachment to the list.
     */
    virtual void addAttach( const KMMessagePart * msgPart ) = 0;
  };

}

#endif // __KMAIL_COMPOSER_H__
