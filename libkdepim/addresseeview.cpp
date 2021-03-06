/*
    This file is part of libkdepim.

    Copyright (c) 2003 Tobias Koenig <tokoe@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <qbuffer.h>
#include <qimage.h>
#include <qpopupmenu.h>
#include <qurl.h>

#include <kabc/address.h>
#include <kabc/addressee.h>
#include <kabc/phonenumber.h>
#include <kabc/resource.h>
#include <kactionclasses.h>
#include <kapplication.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <kio/job.h>
#include <klocale.h>
#include <kmdcodec.h>
#include <kmessagebox.h>
#include <krun.h>
#include <kstringhandler.h>
#include <ktempfile.h>

#include <kdebug.h>

#include "addresseeview.h"
#include "sendsmsdialog.h"
#include "resourceabc.h"

using namespace KPIM;

AddresseeView::AddresseeView( QWidget *parent, const char *name,
                              KConfig *config )
  : KTextBrowser( parent, name ), mDefaultConfig( false ), mImageJob( 0 ),
    mLinkMask( AddressLinks | EmailLinks | PhoneLinks | URLLinks | IMLinks | CustomFields )
{
  setWrapPolicy( QTextEdit::AtWordBoundary );
  setLinkUnderline( false );
  setVScrollBarMode( QScrollView::AlwaysOff );
  setHScrollBarMode( QScrollView::AlwaysOff );

  QStyleSheet *sheet = styleSheet();
  QStyleSheetItem *link = sheet->item( "a" );
  link->setColor( KGlobalSettings::linkColor() );

  connect( this, SIGNAL( mailClick( const QString&, const QString& ) ),
           this, SLOT( slotMailClicked( const QString&, const QString& ) ) );
  connect( this, SIGNAL( urlClick( const QString& ) ),
           this, SLOT( slotUrlClicked( const QString& ) ) );
  connect( this, SIGNAL( highlighted( const QString& ) ),
           this, SLOT( slotHighlighted( const QString& ) ) );

  setNotifyClick( true );

  mActionShowBirthday = new KToggleAction( i18n( "Show Birthday" ) );
  mActionShowBirthday->setCheckedState( i18n( "Hide Birthday" ) );
  mActionShowAddresses = new KToggleAction( i18n( "Show Postal Addresses" ) );
  mActionShowAddresses->setCheckedState( i18n( "Hide Postal Addresses" ) );
  mActionShowEmails = new KToggleAction( i18n( "Show Email Addresses" ) );
  mActionShowEmails->setCheckedState( i18n( "Hide Email Addresses" ) );
  mActionShowPhones = new KToggleAction( i18n( "Show Telephone Numbers" ) );
  mActionShowPhones->setCheckedState( i18n( "Hide Telephone Numbers" ) );
  mActionShowURLs = new KToggleAction( i18n( "Show Web Pages (URLs)" ) );
  mActionShowURLs->setCheckedState( i18n( "Hide Web Pages (URLs)" ) );
  mActionShowIMAddresses = new KToggleAction( i18n( "Show Instant Messaging Addresses" ) );
  mActionShowIMAddresses->setCheckedState( i18n( "Hide Instant Messaging Addresses" ) );
  mActionShowCustomFields = new KToggleAction( i18n( "Show Custom Fields" ) );
  mActionShowCustomFields->setCheckedState( i18n( "Hide Custom Fields" ) );

  connect( mActionShowBirthday, SIGNAL( toggled( bool ) ), SLOT( configChanged() ) );
  connect( mActionShowAddresses, SIGNAL( toggled( bool ) ), SLOT( configChanged() ) );
  connect( mActionShowEmails, SIGNAL( toggled( bool ) ), SLOT( configChanged() ) );
  connect( mActionShowPhones, SIGNAL( toggled( bool ) ), SLOT( configChanged() ) );
  connect( mActionShowURLs, SIGNAL( toggled( bool ) ), SLOT( configChanged() ) );
  connect( mActionShowIMAddresses, SIGNAL( toggled( bool ) ), SLOT( configChanged() ) );
  connect( mActionShowCustomFields, SIGNAL( toggled( bool ) ), SLOT( configChanged() ) );

  if ( !config ) {
    mConfig = new KConfig( "kaddressbookrc" );
    mDefaultConfig = true;
  } else
    mConfig = config;

  load();

  // set up IMProxy to display contacts' IM presence and make connections to keep the display live
  mKIMProxy = ::KIMProxy::instance( kapp->dcopClient() );
  connect( mKIMProxy, SIGNAL( sigContactPresenceChanged( const QString& ) ),
           this, SLOT( slotPresenceChanged( const QString& ) ) );
  connect( mKIMProxy, SIGNAL( sigPresenceInfoExpired() ),
           this, SLOT( slotPresenceInfoExpired() ) );
}

AddresseeView::~AddresseeView()
{
  if ( mDefaultConfig )
    delete mConfig;
  mConfig = 0;

  delete mActionShowBirthday;
  delete mActionShowAddresses;
  delete mActionShowEmails;
  delete mActionShowPhones;
  delete mActionShowURLs;
  delete mActionShowIMAddresses;
  delete mActionShowCustomFields;

  mKIMProxy = 0;
}

void AddresseeView::setAddressee( const KABC::Addressee& addr )
{
  mAddressee = addr;

  if ( mImageJob ) {
    mImageJob->kill();
    mImageJob = 0;
  }

  mImageData.truncate( 0 );

  updateView();
}

void AddresseeView::enableLinks( int linkMask )
{
  mLinkMask = linkMask;
}

QString AddresseeView::vCardAsHTML( const KABC::Addressee& addr, ::KIMProxy *proxy, LinkMask linkMask,
                                    bool internalLoading, FieldMask fieldMask )
{
  QString image = QString( "contact_%1_image" ).arg( addr.uid() );

  // Style strings from Gentix; this is just an initial version.
  //
  // These will be substituted into various HTML strings with .arg().
  // Search for @STYLE@ to find where. Note how we use %1 as a
  // placeholder where we fill in something else (in this case,
  // the global background color).
  //
  QString backgroundColor = KGlobalSettings::alternateBackgroundColor().name();
  QString cellStyle = QString::fromLatin1(
        "style=\""
        "padding-right: 2px; "
        "border-right: #000 dashed 1px; "
        "background: %1;\"").arg(backgroundColor);
  QString backgroundColor2 = KGlobalSettings::baseColor().name();
  QString cellStyle2 = QString::fromLatin1(
        "style=\""
        "padding-left: 2px; "
        "background: %1;\"").arg(backgroundColor2);
  QString tableStyle = QString::fromLatin1(
        "style=\""
        "border: solid 1px; "
        "margin: 0em;\"");

  // We'll be building a table to display the vCard in.
  // Each row of the table will be built using this string for its HTML.
  //
  QString rowFmtStr = QString::fromLatin1(
        "<tr>"
        "<td align=\"right\" valign=\"top\" width=\"30%\" "); // Tag unclosed
  rowFmtStr.append( cellStyle );
  rowFmtStr.append( QString::fromLatin1(
	">" // Close tag
        "<b>%1</b>"
        "</td>"
        "<td align=\"left\" valign=\"top\" width=\"70%\" ") ); // Tag unclosed
  rowFmtStr.append( cellStyle2 );
  rowFmtStr.append( QString::fromLatin1(
	">" // Close tag
        "%2"
        "</td>"
        "</tr>\n"
        ) );

  // Build the table's rows here
  QString dynamicPart;


  if ( !internalLoading ) {
    KABC::Picture pic = addr.photo();
    if ( pic.isIntern() && !pic.data().isNull() ) {
      image = pixmapAsDataUrl( pic.data() );
    } else if ( !pic.url().isEmpty() ) {
      image = (pic.url().startsWith( "http://" ) || pic.url().startsWith( "https://" ) ? pic.url() : "http://" + pic.url());
    } else {
      image = "file:" + KGlobal::iconLoader()->iconPath( "personal", KIcon::Desktop );
    }
  }

  if ( fieldMask & BirthdayFields ) {
    QDate date = addr.birthday().date();

    if ( date.isValid() )
      dynamicPart += rowFmtStr
        .arg( KABC::Addressee::birthdayLabel() )
        .arg( KGlobal::locale()->formatDate( date, true ) );
  }

  if ( fieldMask & PhoneFields ) {
    KABC::PhoneNumber::List phones = addr.phoneNumbers();
    KABC::PhoneNumber::List::ConstIterator phoneIt;
    for ( phoneIt = phones.begin(); phoneIt != phones.end(); ++phoneIt ) {
      QString number = QStyleSheet::escape( (*phoneIt).number() );

      QString url;
      if ( (*phoneIt).type() & KABC::PhoneNumber::Fax )
        url = QString::fromLatin1( "fax:" ) + number;
      else
        url = QString::fromLatin1( "phone:" ) + number;

      if ( linkMask & PhoneLinks ) {
        QString smsURL;
        if ( (*phoneIt).type() & KABC::PhoneNumber::Cell )
          smsURL = QString(" (<a href=\"sms:%1\">%2</a>)" ).arg( number ).arg( i18n( "SMS") );

        dynamicPart += rowFmtStr
          .arg( (*phoneIt).typeLabel().replace( " ", "&nbsp;" ) )
          .arg( QString::fromLatin1( "<a href=\"%1\">%2</a>%3" ).arg( url ).arg( number ).arg( smsURL ) );
      } else {
        dynamicPart += rowFmtStr
          .arg( (*phoneIt).typeLabel().replace( " ", "&nbsp;" ) )
          .arg( number );
      }
    }
  }

  if ( fieldMask & EmailFields ) {
    QStringList emails = addr.emails();
    QStringList::ConstIterator emailIt;
    QString type = i18n( "Email" );
    for ( emailIt = emails.begin(); emailIt != emails.end(); ++emailIt ) {
      QString fullEmail = addr.fullEmail( *emailIt );
      QUrl::encode( fullEmail );

      if ( linkMask & EmailLinks ) {
        dynamicPart += rowFmtStr.arg( type )
          .arg( QString::fromLatin1( "<a href=\"mailto:%1\">%2</a>" )
          .arg( fullEmail, QStyleSheet::escape( *emailIt ) ) );
      } else {
        dynamicPart += rowFmtStr.arg( type ).arg( *emailIt );
      }
    }
  }

  if ( fieldMask & URLFields ) {
    if ( !addr.url().url().isEmpty() ) {
      QString url;
      if ( linkMask & URLLinks ) {
        url = (addr.url().url().startsWith( "http://" ) || addr.url().url().startsWith( "https://" ) ? addr.url().prettyURL() :
          "http://" + addr.url().prettyURL());
        url = KStringHandler::tagURLs( url );
      } else {
        url = addr.url().prettyURL();
      }
      dynamicPart += rowFmtStr.arg( i18n("Homepage") ).arg( url );
    }

    QString blog = addr.custom( "KADDRESSBOOK", "BlogFeed" );
    if ( !blog.isEmpty() ) {
      if ( linkMask & URLLinks ) {
        blog = KStringHandler::tagURLs( blog );
      }
      dynamicPart += rowFmtStr.arg( i18n("Blog Feed") ).arg( blog );
    }
  }

  if ( fieldMask & AddressFields ) {
    KABC::Address::List addresses = addr.addresses();
    KABC::Address::List::ConstIterator addrIt;
    for ( addrIt = addresses.begin(); addrIt != addresses.end(); ++addrIt ) {
      if ( (*addrIt).label().isEmpty() ) {
        QString formattedAddress;

        formattedAddress = QStyleSheet::escape( (*addrIt).formattedAddress().stripWhiteSpace() );
        formattedAddress = formattedAddress.replace( '\n', "<br>" );

        QString link = "<a href=\"addr:" + (*addrIt).id() + "\">" +
                       formattedAddress + "</a>";

        if ( linkMask & AddressLinks ) {
          dynamicPart += rowFmtStr
            .arg( KABC::Address::typeLabel( (*addrIt).type() ) )
            .arg( link );
        } else {
          dynamicPart += rowFmtStr
            .arg( KABC::Address::typeLabel( (*addrIt).type() ) )
            .arg( formattedAddress );
        }
      } else {
        QString link = "<a href=\"addr:" + (*addrIt).id() + "\">" +
                       (*addrIt).label().replace( '\n', "<br>" ) + "</a>";

        if ( linkMask & AddressLinks ) {
          dynamicPart += rowFmtStr
            .arg( KABC::Address::typeLabel( (*addrIt).type() ) )
            .arg( link );
        } else {
          dynamicPart += rowFmtStr
            .arg( KABC::Address::typeLabel( (*addrIt).type() ) )
            .arg( (*addrIt).label().replace( '\n', "<br>" ) );
        }
      }
    }
  }

  QString notes;
  if ( !addr.note().isEmpty() ) {
    // @STYLE@ - substitute the cell style in first, and append
    // the data afterwards (keeps us safe from possible % signs
    // in either one).
    notes = QStyleSheet::escape( addr.note() );
    notes = rowFmtStr.arg( i18n( "Notes" ) ).arg( notes.replace( '\n', "<br>" ) ) ;
  }

  QString customData;
  if ( fieldMask & CustomFields ) {
    static QMap<QString, QString> titleMap;
    if ( titleMap.isEmpty() ) {
      titleMap.insert( "Department", i18n( "Department" ) );
      titleMap.insert( "Profession", i18n( "Profession" ) );
      titleMap.insert( "AssistantsName", i18n( "Assistant's Name" ) );
      titleMap.insert( "ManagersName", i18n( "Manager's Name" ) );
      titleMap.insert( "SpousesName", i18n( "Partner's Name" ) );
      titleMap.insert( "Office", i18n( "Office" ) );
      titleMap.insert( "Anniversary", i18n( "Anniversary" ) );
    }

    if ( !addr.customs().empty() ) {
      QStringList customs = addr.customs();
      QStringList::Iterator it( customs.begin() );
      const QStringList::Iterator endIt( customs.end() );
      for ( ; it != endIt; ++it ) {
        QString customEntry = *it;
        if ( customEntry.startsWith ( "KADDRESSBOOK-" ) ) {
          customEntry.remove( "KADDRESSBOOK-X-" );
          customEntry.remove( "KADDRESSBOOK-" );

          int pos = customEntry.find( ':' );
          QString key = customEntry.left( pos );
          const QString value = customEntry.mid( pos + 1 );

          // blog and im address is handled separated
          if ( key == "BlogFeed" || key == "IMAddress" )
            continue;

          const QMap<QString, QString>::ConstIterator keyIt = titleMap.find( key );
          if ( keyIt != titleMap.end() )
            key = keyIt.data();

          customData += rowFmtStr.arg( key ).arg( QStyleSheet::escape( value ) ) ;
        }
      }
    }
  }

  QString name( QStyleSheet::escape( addr.realName() ) );
  QString role( QStyleSheet::escape( addr.role() ) );
  QString organization( QStyleSheet::escape( addr.organization() ) );

  if ( fieldMask & IMFields ) {

    const QString imAddress = addr.custom( "KADDRESSBOOK", "X-IMAddress" );
    if ( !imAddress.isEmpty() ) {
      customData += rowFmtStr.arg( i18n( "IM Address" ) ).arg( QStyleSheet::escape( imAddress ) ) ;
    }

    if ( proxy ) {
      if ( proxy->isPresent( addr.uid() ) && proxy->presenceNumeric( addr.uid() ) > 0 ) {
        // set image source to either a QMimeSourceFactory key or a data:/ URL
        QString imgSrc;
        if ( internalLoading ) {
          imgSrc = QString::fromLatin1( "im_status_%1_image").arg( addr.uid() );
          QMimeSourceFactory::defaultFactory()->setPixmap( imgSrc, proxy->presenceIcon( addr.uid() ) );
        } else
          imgSrc = pixmapAsDataUrl( proxy->presenceIcon( addr.uid() ) );

        // make the status a link, if required
        QString imStatus;
        if ( linkMask & IMLinks )
          imStatus = QString::fromLatin1( "<a href=\"im:\"><img src=\"%1\"> (%2)</a>" );
        else
          imStatus = QString::fromLatin1( "<img src=\"%1\"> (%2)" );

        // append our status to the rest of the dynamic part of the addressee
        dynamicPart += rowFmtStr
                .arg( i18n( "Presence" ) )
                .arg( imStatus
                          .arg( imgSrc )
                          .arg( proxy->presenceString( addr.uid() ) )
                    );
      }
    }
  }

  // @STYLE@ - construct the string by parts, substituting in
  // the styles first. There are lots of appends, but we need to
  // do it this way to avoid cases where the substituted string
  // contains %1 and the like.
  //
  QString strAddr = QString::fromLatin1(
    "<div align=\"center\">"
    "<table cellpadding=\"1\" cellspacing=\"0\" %1>"
    "<tr>").arg(tableStyle);

  strAddr.append( QString::fromLatin1(
    "<td align=\"right\" valign=\"top\" width=\"30%\" rowspan=\"3\" %2>")
    .arg( cellStyle ) );
  strAddr.append( QString::fromLatin1(
    "<img src=\"%1\" width=\"50\" vspace=\"1\">" // image
    "</td>")
    .arg( image ) );
  strAddr.append( QString::fromLatin1(
    "<td align=\"left\" width=\"70%\" %2>")
    .arg( cellStyle2 ) );
  strAddr.append( QString::fromLatin1(
    "<font size=\"+2\"><b>%2</b></font></td>"  // name
    "</tr>")
    .arg( name ) );
  strAddr.append( QString::fromLatin1(
    "<tr>"
    "<td align=\"left\" width=\"70%\" %2>")
    .arg( cellStyle2 ) );
  strAddr.append( QString::fromLatin1(
    "%3</td>"  // role
    "</tr>")
    .arg( role ) );
  strAddr.append( QString::fromLatin1(
    "<tr>"
    "<td align=\"left\" width=\"70%\" %2>")
    .arg( cellStyle2 ) );
  strAddr.append( QString::fromLatin1(
    "%4</td>"  // organization
    "</tr>")
    .arg( organization ) );
  strAddr.append( QString::fromLatin1(
    "<tr><td %2>")
    .arg( cellStyle ) );
  strAddr.append( QString::fromLatin1(
    "&nbsp;</td><td %2>&nbsp;</td></tr>")
    .arg( cellStyle2 ) );
  strAddr.append( dynamicPart );
  strAddr.append( notes );
  strAddr.append( customData );
  strAddr.append( QString::fromLatin1( "</table></div>\n" ) );

  if ( addr.resource() ) {
    QString addrBookName = addr.resource()->resourceName();
    ResourceABC *r = dynamic_cast<ResourceABC*>( addr.resource() );
    if ( r && !r->subresources().isEmpty() ) {
      const QString subRes = r->uidToResourceMap()[ addr.uid() ];
      const QString label = r->subresourceLabel( subRes );
      if ( !label.isEmpty() )
        addrBookName = label;
    }
    strAddr.append( i18n( "<p><b>Address book</b>: %1</p>" ).arg( addrBookName ) );
  }
  return strAddr;
}

QString AddresseeView::pixmapAsDataUrl( const QPixmap& pixmap )
{
  QByteArray ba;
  QBuffer buffer( ba );
  buffer.open( IO_WriteOnly );
  pixmap.save( &buffer, "PNG" );
  QString encoded( "data:image/png;base64," );
  encoded.append( KCodecs::base64Encode( ba ) );
  return encoded;
}

void AddresseeView::updateView()
{
  // clear view
  setText( QString::null );

  if ( mAddressee.isEmpty() )
    return;

  if ( mImageJob ) {
    mImageJob->kill();
    mImageJob = 0;

    mImageData.truncate( 0 );
  }

  int fieldMask = NoFields;
  if ( mActionShowBirthday->isChecked() )
    fieldMask |= ( FieldMask )BirthdayFields;
  if ( mActionShowAddresses->isChecked() )
    fieldMask |= AddressFields;
  if ( mActionShowEmails->isChecked() )
    fieldMask |= EmailFields;
  if ( mActionShowPhones->isChecked() )
    fieldMask |= PhoneFields;
  if ( mActionShowURLs->isChecked() )
    fieldMask |= URLFields;
  if ( mActionShowIMAddresses->isChecked() )
    fieldMask |= IMFields;
  if ( mActionShowCustomFields->isChecked() )
    fieldMask |= CustomFields;

  QString strAddr = vCardAsHTML( mAddressee, mKIMProxy, (LinkMask)mLinkMask,
                                 true, (FieldMask)fieldMask );

  strAddr = QString::fromLatin1(
    "<html>"
    "<body text=\"%1\" bgcolor=\"%2\">" // text and background color
    "%3" // dynamic part
    "</body>"
    "</html>" )
     .arg( KGlobalSettings::textColor().name() )
     .arg( KGlobalSettings::baseColor().name() )
     .arg( strAddr );

  QString imageURL = QString( "contact_%1_image" ).arg( mAddressee.uid() );

  KABC::Picture picture = mAddressee.photo();
  if ( picture.isIntern() && !picture.data().isNull() )
    QMimeSourceFactory::defaultFactory()->setImage( imageURL, picture.data() );
  else {
    if ( !picture.url().isEmpty() ) {
      if ( mImageData.count() > 0 )
        QMimeSourceFactory::defaultFactory()->setImage( imageURL, mImageData );
      else {
        mImageJob = KIO::get( KURL( picture.url() ), false, false );
        connect( mImageJob, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
                 this, SLOT( data( KIO::Job*, const QByteArray& ) ) );
        connect( mImageJob, SIGNAL( result( KIO::Job* ) ),
                 this, SLOT( result( KIO::Job* ) ) );
      }
    } else {
      QMimeSourceFactory::defaultFactory()->setPixmap( imageURL,
        KGlobal::iconLoader()->loadIcon( "personal", KIcon::Desktop, 128 ) );
    }
  }

  // at last display it...
  setText( strAddr );
}

KABC::Addressee AddresseeView::addressee() const
{
  return mAddressee;
}

void AddresseeView::urlClicked( const QString &url )
{
  kapp->invokeBrowser( url );
}

void AddresseeView::emailClicked( const QString &email )
{
  if ( email.startsWith( "mailto:" ) )
    kapp->invokeMailer( email.mid( 7 ), QString::null );
  else
    kapp->invokeMailer( email, QString::null );
}

void AddresseeView::phoneNumberClicked( const QString &number )
{
  KConfig config( "kaddressbookrc" );
  config.setGroup( "General" );
  QString commandLine = config.readEntry( "PhoneHookApplication" );

  if ( commandLine.isEmpty() ) {
    KMessageBox::sorry( this, i18n( "There is no application set which could be executed. Please go to the settings dialog and configure one." ) );
    return;
  }

  commandLine.replace( "%N", number );
  KRun::runCommand( commandLine );
}

void AddresseeView::smsTextClicked( const QString &number )
{
  KConfig config( "kaddressbookrc" );
  config.setGroup( "General" );
  QString commandLine = config.readEntry( "SMSHookApplication" );

  if ( commandLine.isEmpty() ) {
    KMessageBox::sorry( this, i18n( "There is no application set which could be executed. Please go to the settings dialog and configure one." ) );
    return;
  }

  SendSMSDialog dlg( mAddressee.realName(), this );

  if ( dlg.exec() )
    sendSMS ( number, dlg.text() );
}

void AddresseeView::sendSMS( const QString &number, const QString &text )
{
  KConfig config( "kaddressbookrc" );
  config.setGroup( "General" );
  QString commandLine = config.readEntry( "SMSHookApplication" );

  KTempFile file ;
  QTextStream* stream = file.textStream();
  *stream << text;
  file.close();

  commandLine.replace( "%N", number );
  commandLine.replace( "%F", file.name() );

  KRun::runCommand( commandLine );
}

void AddresseeView::faxNumberClicked( const QString &number )
{
  KConfig config( "kaddressbookrc" );
  config.setGroup( "General" );
  QString commandLine = config.readEntry( "FaxHookApplication", "kdeprintfax --phone %N" );

  if ( commandLine.isEmpty() ) {
    KMessageBox::sorry( this, i18n( "There is no application set which could be executed. Please go to the settings dialog and configure one." ) );
    return;
  }

  commandLine.replace( "%N", number );
  KRun::runCommand( commandLine );
}

void AddresseeView::imAddressClicked()
{
  mKIMProxy->chatWithContact( mAddressee.uid() );
}

QPopupMenu *AddresseeView::createPopupMenu( const QPoint& )
{
  QPopupMenu *menu = new QPopupMenu( this );
  mActionShowBirthday->plug( menu );
  mActionShowAddresses->plug( menu );
  mActionShowEmails->plug( menu );
  mActionShowPhones->plug( menu );
  mActionShowURLs->plug( menu );
  mActionShowIMAddresses->plug( menu );
  mActionShowCustomFields->plug( menu );

  return menu;
}

void AddresseeView::slotMailClicked( const QString&, const QString &email )
{
  emailClicked( email );
}

void AddresseeView::slotUrlClicked( const QString &url )
{
  if ( url.startsWith( "phone:" ) )
    phoneNumberClicked( strippedNumber( url.mid( 8 ) ) );
  else if ( url.startsWith( "sms:" ) )
    smsTextClicked( strippedNumber( url.mid( 6 ) ) );
  else if ( url.startsWith( "fax:" ) )
    faxNumberClicked( strippedNumber( url.mid( 6 ) ) );
  else if ( url.startsWith( "addr:" ) )
    emit addressClicked( url.mid( 7 ) );
  else if ( url.startsWith( "im:" ) )
    imAddressClicked();
  else
    urlClicked( url );
}

void AddresseeView::slotHighlighted( const QString &link )
{
  if ( link.startsWith( "mailto:" ) ) {
    QString email = link.mid( 7 );

    emit emailHighlighted( email );
    emit highlightedMessage( i18n( "Send mail to '%1'" ).arg( email ) );
  } else if ( link.startsWith( "phone:" ) ) {
    QString number = link.mid( 8 );

    emit phoneNumberHighlighted( strippedNumber( number ) );
    emit highlightedMessage( i18n( "Call number %1" ).arg( number ) );
  } else if ( link.startsWith( "fax:" ) ) {
    QString number = link.mid( 6 );

    emit faxNumberHighlighted( strippedNumber( number ) );
    emit highlightedMessage( i18n( "Send fax to %1" ).arg( number ) );
  } else if ( link.startsWith( "addr:" ) ) {
    emit highlightedMessage( i18n( "Show address on map" ) );
  } else if ( link.startsWith( "sms:" ) ) {
    QString number = link.mid( 6 );
    emit highlightedMessage( i18n( "Send SMS to %1" ).arg( number ) );
  } else if ( link.startsWith( "http:" ) || link.startsWith( "https:" ) ) {
    emit urlHighlighted( link );
    emit highlightedMessage( i18n( "Open URL %1" ).arg( link ) );
  } else if ( link.startsWith( "im:" ) ) {
    emit highlightedMessage( i18n( "Chat with %1" ).arg( mAddressee.realName() ) );
  } else
    emit highlightedMessage( "" );
}

void AddresseeView::slotPresenceChanged( const QString &uid )
{
  kdDebug() << k_funcinfo << " uid is: " << uid << " mAddressee is: " << mAddressee.uid() << endl;
  if ( uid == mAddressee.uid() )
    updateView();
}


void AddresseeView::slotPresenceInfoExpired()
{
  updateView();
}

void AddresseeView::configChanged()
{
  save();
  updateView();
}

void AddresseeView::data( KIO::Job*, const QByteArray &d )
{
  unsigned int oldSize = mImageData.size();
  mImageData.resize( oldSize + d.size() );
  memcpy( mImageData.data() + oldSize, d.data(), d.size() );
}

void AddresseeView::result( KIO::Job *job )
{
  mImageJob = 0;

  if ( job->error() )
    mImageData.truncate( 0 );
  else
    updateView();
}

void AddresseeView::load()
{
  mConfig->setGroup( "AddresseeViewSettings" );
  mActionShowBirthday->setChecked( mConfig->readBoolEntry( "ShowBirthday", false ) );
  mActionShowAddresses->setChecked( mConfig->readBoolEntry( "ShowAddresses", true ) );
  mActionShowEmails->setChecked( mConfig->readBoolEntry( "ShowEmails", true ) );
  mActionShowPhones->setChecked( mConfig->readBoolEntry( "ShowPhones", true ) );
  mActionShowURLs->setChecked( mConfig->readBoolEntry( "ShowURLs", true ) );
  mActionShowIMAddresses->setChecked( mConfig->readBoolEntry( "ShowIMAddresses", false ) );
  mActionShowCustomFields->setChecked( mConfig->readBoolEntry( "ShowCustomFields", false ) );
}

void AddresseeView::save()
{
  mConfig->setGroup( "AddresseeViewSettings" );
  mConfig->writeEntry( "ShowBirthday", mActionShowBirthday->isChecked() );
  mConfig->writeEntry( "ShowAddresses", mActionShowAddresses->isChecked() );
  mConfig->writeEntry( "ShowEmails", mActionShowEmails->isChecked() );
  mConfig->writeEntry( "ShowPhones", mActionShowPhones->isChecked() );
  mConfig->writeEntry( "ShowURLs", mActionShowURLs->isChecked() );
  mConfig->writeEntry( "ShowIMAddresses", mActionShowIMAddresses->isChecked() );
  mConfig->writeEntry( "ShowCustomFields", mActionShowCustomFields->isChecked() );
  mConfig->sync();
}

QString AddresseeView::strippedNumber( const QString &number )
{
  QString retval;

  for ( uint i = 0; i < number.length(); ++i ) {
    QChar c = number[ i ];
    if ( c.isDigit() || c == '*' || c == '#' || c == '+' && i == 0 )
      retval.append( c );
  }

  return retval;
}

#include "addresseeview.moc"
