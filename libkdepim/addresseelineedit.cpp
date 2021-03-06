/*
    This file is part of libkdepim.
    Copyright (c) 2002 Helge Deller <deller@gmx.de>
                  2002 Lubos Lunak <llunak@suse.cz>
                  2001,2003 Carsten Pfeiffer <pfeiffer@kde.org>
                  2001 Waldo Bastian <bastian@kde.org>
                  2004 Daniel Molkentin <danimo@klaralvdalens-datakonsult.se>
                  2004 Karl-Heinz Zimmer <khz@klaralvdalens-datakonsult.se>

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

#include "addresseelineedit.h"

#include "resourceabc.h"
#include "completionordereditor.h"
#include "ldapclient.h"

#include <config.h>

#ifdef KDEPIM_NEW_DISTRLISTS
#include "distributionlist.h"
#else
#include <kabc/distributionlist.h>
#endif

#include <kabc/stdaddressbook.h>
#include <kabc/resource.h>
#include <libemailfunctions/email.h>

#include <kcompletionbox.h>
#include <kcursor.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <kstaticdeleter.h>
#include <kstdaccel.h>
#include <kurldrag.h>
#include <klocale.h>

#include <qpopupmenu.h>
#include <qapplication.h>
#include <qobject.h>
#include <qptrlist.h>
#include <qregexp.h>
#include <qevent.h>
#include <qdragobject.h>
#include <qclipboard.h>

using namespace KPIM;

KMailCompletion * AddresseeLineEdit::s_completion = 0L;
KPIM::CompletionItemsMap* AddresseeLineEdit::s_completionItemMap = 0L;
QStringList* AddresseeLineEdit::s_completionSources = 0L;
bool AddresseeLineEdit::s_addressesDirty = false;
QTimer* AddresseeLineEdit::s_LDAPTimer = 0L;
KPIM::LdapSearch* AddresseeLineEdit::s_LDAPSearch = 0L;
QString* AddresseeLineEdit::s_LDAPText = 0L;
AddresseeLineEdit* AddresseeLineEdit::s_LDAPLineEdit = 0L;

static KStaticDeleter<KMailCompletion> completionDeleter;
static KStaticDeleter<KPIM::CompletionItemsMap> completionItemsDeleter;
static KStaticDeleter<QTimer> ldapTimerDeleter;
static KStaticDeleter<KPIM::LdapSearch> ldapSearchDeleter;
static KStaticDeleter<QString> ldapTextDeleter;
static KStaticDeleter<QStringList> completionSourcesDeleter;

// needs to be unique, but the actual name doesn't matter much
static QCString newLineEditDCOPObjectName()
{
    static int s_count = 0;
    QCString name( "KPIM::AddresseeLineEdit" );
    if ( s_count++ ) {
      name += '-';
      name += QCString().setNum( s_count );
    }
    return name;
}

static const QString s_completionItemIndentString = "     ";

static bool itemIsHeader( const QListBoxItem* item )
{
  return item && !item->text().startsWith( s_completionItemIndentString );
}

AddresseeLineEdit::AddresseeLineEdit( QWidget* parent, bool useCompletion, const char *name )
: ClickLineEdit( parent, QString::null, name )
, DCOPObject( newLineEditDCOPObjectName() )
{
  m_useCompletion = useCompletion;
  m_completionInitialized = false;
  m_smartPaste = false;
  m_addressBookConnected = false;
  m_searchExtended = false;

  init();

  if ( m_useCompletion )
    s_addressesDirty = true;
}

void AddresseeLineEdit::init()
{
  if ( !s_completion ) {
    completionDeleter.setObject( s_completion, new KMailCompletion() );
    s_completion->setOrder( completionOrder() );
    s_completion->setIgnoreCase( true );

    completionItemsDeleter.setObject( s_completionItemMap, new KPIM::CompletionItemsMap() );
    completionSourcesDeleter.setObject( s_completionSources, new QStringList() );
  }

//  connect( s_completion, SIGNAL( match( const QString& ) ),
//           this, SLOT( slotMatched( const QString& ) ) );

  if ( m_useCompletion ) {
    if ( !s_LDAPTimer ) {
      ldapTimerDeleter.setObject( s_LDAPTimer, new QTimer( 0, "ldapTimerDeleter" ) );
      ldapSearchDeleter.setObject( s_LDAPSearch, new KPIM::LdapSearch );
      ldapTextDeleter.setObject( s_LDAPText, new QString );

      /* Add completion sources for all ldap server, 0 to n. Added first so
       * that they map to the ldapclient::clientNumber() */
      QValueList< LdapClient* > clients =  s_LDAPSearch->clients();
      for ( QValueList<LdapClient*>::iterator it = clients.begin(); it != clients.end(); ++it ) {
        addCompletionSource( "LDAP server: " + (*it)->server().host() );
      }
    }
    if ( !m_completionInitialized ) {
      setCompletionObject( s_completion, false );
      connect( this, SIGNAL( completion( const QString& ) ),
          this, SLOT( slotCompletion() ) );
      connect( this, SIGNAL( returnPressed( const QString& ) ),
          this, SLOT( slotReturnPressed( const QString& ) ) );

      KCompletionBox *box = completionBox();
      connect( box, SIGNAL( highlighted( const QString& ) ),
          this, SLOT( slotPopupCompletion( const QString& ) ) );
      connect( box, SIGNAL( userCancelled( const QString& ) ),
          SLOT( slotUserCancelled( const QString& ) ) );

      // The emitter is always called KPIM::IMAPCompletionOrder by contract
      if ( !connectDCOPSignal( 0, "KPIM::IMAPCompletionOrder", "orderChanged()",
            "slotIMAPCompletionOrderChanged()", false ) )
        kdError() << "AddresseeLineEdit: connection to orderChanged() failed" << endl;

      connect( s_LDAPTimer, SIGNAL( timeout() ), SLOT( slotStartLDAPLookup() ) );
      connect( s_LDAPSearch, SIGNAL( searchData( const KPIM::LdapResultList& ) ),
          SLOT( slotLDAPSearchData( const KPIM::LdapResultList& ) ) );

      m_completionInitialized = true;
    }
  }
}

AddresseeLineEdit::~AddresseeLineEdit()
{
  if ( s_LDAPSearch && s_LDAPLineEdit == this )
    stopLDAPLookup();
}

void AddresseeLineEdit::setFont( const QFont& font )
{
  KLineEdit::setFont( font );
  if ( m_useCompletion )
    completionBox()->setFont( font );
}

void AddresseeLineEdit::allowSemiColonAsSeparator( bool useSemiColonAsSeparator )
{
  m_useSemiColonAsSeparator = useSemiColonAsSeparator;
}

void AddresseeLineEdit::keyPressEvent( QKeyEvent *e )
{
  bool accept = false;

  if ( KStdAccel::shortcut( KStdAccel::SubstringCompletion ).contains( KKey( e ) ) ) {
    //TODO: add LDAP substring lookup, when it becomes available in KPIM::LDAPSearch
    updateSearchString();
    doCompletion( true );
    accept = true;
  } else if ( KStdAccel::shortcut( KStdAccel::TextCompletion ).contains( KKey( e ) ) ) {
    int len = text().length();

    if ( len == cursorPosition() ) { // at End?
      updateSearchString();
      doCompletion( true );
      accept = true;
    }
  }

  if ( !accept )
    KLineEdit::keyPressEvent( e );

  if ( e->isAccepted() ) {
    updateSearchString();
    QString searchString( m_searchString );
    //LDAP does not know about our string manipulation, remove it
    if ( m_searchExtended )
        searchString = m_searchString.mid( 1 );

    if ( m_useCompletion && s_LDAPTimer != NULL ) {
      if ( *s_LDAPText != searchString || s_LDAPLineEdit != this )
        stopLDAPLookup();

      *s_LDAPText = searchString;
      s_LDAPLineEdit = this;
      s_LDAPTimer->start( 500, true );
    }
  }
}

void AddresseeLineEdit::insert( const QString &t )
{
    if(!m_smartPaste) {
        KLineEdit::insert(t);
        return;
    }

    //kdDebug(5300) << "     AddresseeLineEdit::insert( \"" << t << "\" )" << endl;

    QString newText = t.stripWhiteSpace();
    if (newText.isEmpty()) return;

    // remove newlines in the to-be-pasted string
    QStringList lines = QStringList::split(QRegExp("\r?\n"), newText, false);
    for(QStringList::iterator it = lines.begin(); it != lines.end(); ++it) {
        // remove trailing commas and whitespace
        (*it).remove( QRegExp(",?\\s*$") );
    }
    newText = lines.join(", ");

    if(newText.startsWith("mailto:")) {
        KURL url( newText );
        newText = url.path();
    }
    else if(newText.find(" at ") != -1) {
        // Anti-spam stuff
        newText.replace( " at ", "@" );
        newText.replace( " dot ", "." );
    }
    else if(newText.find("(at)") != -1) {
        newText.replace( QRegExp("\\s*\\(at\\)\\s*"), "@" );
    }

    QString contents = text();
    int start_sel = 0;
    int pos = cursorPosition();
    if(hasSelectedText()) {
        // Cut away the selection.
        start_sel = selectionStart();
        pos = start_sel;
        contents = contents.left(start_sel) + contents.right(start_sel + selectedText().length());
    }

    int eot = contents.length();

    while((eot > 0) && contents[eot-1].isSpace()) eot--;

    if (eot == 0)
        contents = QString::null;
    else if (pos >= eot) {
        if (contents[ eot - 1 ] == ',') eot--;
        contents.truncate(eot);
        contents += ", ";
        pos = eot + 2;
    }

    contents = contents.left(pos) + newText + contents.mid(pos);
    setText(contents);
    setEdited(true);
    setCursorPosition(pos + newText.length());
}

void AddresseeLineEdit::setText( const QString & text )
{
  ClickLineEdit::setText( text.stripWhiteSpace() );
}

void AddresseeLineEdit::paste()
{
  if ( m_useCompletion )
    m_smartPaste = true;

  KLineEdit::paste();
  m_smartPaste = false;
}

void AddresseeLineEdit::mouseReleaseEvent( QMouseEvent *e )
{
  // reimplemented from QLineEdit::mouseReleaseEvent()
  if ( m_useCompletion
       && QApplication::clipboard()->supportsSelection()
       && !isReadOnly()
       && e->button() == MidButton ) {
    m_smartPaste = true;
  }

  KLineEdit::mouseReleaseEvent( e );
  m_smartPaste = false;
}

void AddresseeLineEdit::dropEvent( QDropEvent *e )
{
  KURL::List uriList;
  if ( !isReadOnly()
       && KURLDrag::canDecode(e) && KURLDrag::decode( e, uriList ) ) {
    QString contents = text();
    // remove trailing white space and comma
    int eot = contents.length();
    while ( ( eot > 0 ) && contents[ eot - 1 ].isSpace() )
      eot--;
    if ( eot == 0 )
      contents = QString::null;
    else if ( contents[ eot - 1 ] == ',' ) {
      eot--;
      contents.truncate( eot );
    }
    bool mailtoURL = false;
    // append the mailto URLs
    for ( KURL::List::Iterator it = uriList.begin();
          it != uriList.end(); ++it ) {
      if ( !contents.isEmpty() )
        contents.append( ", " );
      KURL u( *it );
      if ( u.protocol() == "mailto" ) {
        mailtoURL = true;
        contents.append( (*it).path() );
      }
    }
    if ( mailtoURL ) {
      setText( contents );
      setEdited( true );
      return;
    }
  }

  if ( m_useCompletion )
    m_smartPaste = true;
  QLineEdit::dropEvent( e );
  m_smartPaste = false;
}

void AddresseeLineEdit::cursorAtEnd()
{
  setCursorPosition( text().length() );
}

void AddresseeLineEdit::enableCompletion( bool enable )
{
  m_useCompletion = enable;
}

void AddresseeLineEdit::doCompletion( bool ctrlT )
{
  m_lastSearchMode = ctrlT;

  KGlobalSettings::Completion  mode = completionMode();

  if ( mode == KGlobalSettings::CompletionNone  )
    return;

  if ( s_addressesDirty ) {
    loadContacts(); // read from local address book
    s_completion->setOrder( completionOrder() );
  }

  // cursor at end of string - or Ctrl+T pressed for substring completion?
  if ( ctrlT ) {
    const QStringList completions = getAdjustedCompletionItems( false );

    if ( completions.count() > 1 )
      ; //m_previousAddresses = prevAddr;
    else if ( completions.count() == 1 )
      setText( m_previousAddresses + completions.first().stripWhiteSpace() );

    setCompletedItems( completions, true ); // this makes sure the completion popup is closed if no matching items were found

    cursorAtEnd();
    setCompletionMode( mode ); //set back to previous mode
    return;
  }


  switch ( mode ) {
    case KGlobalSettings::CompletionPopupAuto:
    {
      if ( m_searchString.isEmpty() )
        break;
    }

    case KGlobalSettings::CompletionPopup:
    {
      const QStringList items = getAdjustedCompletionItems( true );
      setCompletedItems( items, false );
      break;
    }

    case KGlobalSettings::CompletionShell:
    {
      QString match = s_completion->makeCompletion( m_searchString );
      if ( !match.isNull() && match != m_searchString ) {
        setText( m_previousAddresses + match );
        setEdited( true );
        cursorAtEnd();
      }
      break;
    }

    case KGlobalSettings::CompletionMan: // Short-Auto in fact
    case KGlobalSettings::CompletionAuto:
    {
      //force autoSuggest in KLineEdit::keyPressed or setCompletedText will have no effect
      setCompletionMode( completionMode() );

      if ( !m_searchString.isEmpty() ) {

        //if only our \" is left, remove it since user has not typed it either
        if ( m_searchExtended && m_searchString == "\"" ){
          m_searchExtended = false;
          m_searchString = QString::null;
          setText( m_previousAddresses );
          break;
        }

        QString match = s_completion->makeCompletion( m_searchString );

        if ( !match.isEmpty() ) {
          if ( match != m_searchString ) {
            QString adds = m_previousAddresses + match;
            setCompletedText( adds );
          }
        } else {
          if ( !m_searchString.startsWith( "\"" ) ) {
            //try with quoted text, if user has not type one already
            match = s_completion->makeCompletion( "\"" + m_searchString );
            if ( !match.isEmpty() && match != m_searchString ) {
              m_searchString = "\"" + m_searchString;
              m_searchExtended = true;
              setText( m_previousAddresses + m_searchString );
              setCompletedText( m_previousAddresses + match );
            }
          } else if ( m_searchExtended ) {
            //our added \" does not work anymore, remove it
            m_searchString = m_searchString.mid( 1 );
            m_searchExtended = false;
            setText( m_previousAddresses + m_searchString );
            //now try again
            match = s_completion->makeCompletion( m_searchString );
            if ( !match.isEmpty() && match != m_searchString ) {
              QString adds = m_previousAddresses + match;
              setCompletedText( adds );
            }
          }
        }
      }
      break;
    }

    case KGlobalSettings::CompletionNone:
    default: // fall through
      break;
  }
}

void AddresseeLineEdit::slotPopupCompletion( const QString& completion )
{
  setText( m_previousAddresses + completion.stripWhiteSpace() );
  cursorAtEnd();
//  slotMatched( m_previousAddresses + completion );
  updateSearchString();
}

void AddresseeLineEdit::slotReturnPressed( const QString& item )
{
  Q_UNUSED( item );
  QListBoxItem* i = completionBox()->selectedItem();
  if ( i != 0 )
    slotPopupCompletion( i->text() );
}

void AddresseeLineEdit::loadContacts()
{
  s_completion->clear();
  s_completionItemMap->clear();
  s_addressesDirty = false;
  //m_contactMap.clear();

  QApplication::setOverrideCursor( KCursor::waitCursor() ); // loading might take a while

  KConfig config( "kpimcompletionorder" ); // The weights for non-imap kabc resources is there.
  config.setGroup( "CompletionWeights" );

  KABC::AddressBook *addressBook = KABC::StdAddressBook::self( true );
  // Can't just use the addressbook's iterator, we need to know which subresource
  // is behind which contact.
  QPtrList<KABC::Resource> resources( addressBook->resources() );
  for( QPtrListIterator<KABC::Resource> resit( resources ); *resit; ++resit ) {
    KABC::Resource* resource = *resit;
    KPIM::ResourceABC* resabc = dynamic_cast<ResourceABC *>( resource );
    if ( resabc ) { // IMAP KABC resource; need to associate each contact with the subresource
      const QMap<QString, QString> uidToResourceMap = resabc->uidToResourceMap();
      KABC::Resource::Iterator it;
      for ( it = resource->begin(); it != resource->end(); ++it ) {
        QString uid = (*it).uid();
        QMap<QString, QString>::const_iterator wit = uidToResourceMap.find( uid );
        const QString subresourceLabel = resabc->subresourceLabel( *wit );
        int idx = s_completionSources->findIndex( subresourceLabel );
        if ( idx == -1 ) {
          s_completionSources->append( subresourceLabel );
          idx = s_completionSources->size() -1;
        }
        int weight = ( wit != uidToResourceMap.end() ) ? resabc->subresourceCompletionWeight( *wit ) : 80;
        //kdDebug(5300) << (*it).fullEmail() << " subres=" << *wit << " weight=" << weight << endl;
        addContact( *it, weight, idx );
      }
    } else { // KABC non-imap resource
      int weight = config.readNumEntry( resource->identifier(), 60 );
      s_completionSources->append( resource->resourceName() );
      KABC::Resource::Iterator it;
        if(resource->type() != "ldapkio") {
            for(it = resource->begin(); it != resource->end(); ++it)
                addContact(*it, weight, s_completionSources->size()-1);
        }
    }
  }

#ifndef KDEPIM_NEW_DISTRLISTS // new distr lists are normal contact, already done above
  int weight = config.readNumEntry( "DistributionLists", 60 );
  KABC::DistributionListManager manager( addressBook );
  manager.load();
  const QStringList distLists = manager.listNames();
  QStringList::const_iterator listIt;
  int idx = addCompletionSource( i18n( "Distribution Lists" ) );
  for ( listIt = distLists.begin(); listIt != distLists.end(); ++listIt ) {

    //for KGlobalSettings::CompletionAuto
    addCompletionItem( (*listIt).simplifyWhiteSpace(), weight, idx );

    //for CompletionShell, CompletionPopup
    QStringList sl( (*listIt).simplifyWhiteSpace() );
    addCompletionItem( (*listIt).simplifyWhiteSpace(), weight, idx, &sl );

  }
#endif

  QApplication::restoreOverrideCursor();

  if ( !m_addressBookConnected ) {
    connect( addressBook, SIGNAL( addressBookChanged( AddressBook* ) ), SLOT( loadContacts() ) );
    m_addressBookConnected = true;
  }
}

void AddresseeLineEdit::addContact( const KABC::Addressee& addr, int weight, int source )
{
#ifdef KDEPIM_NEW_DISTRLISTS
  if ( KPIM::DistributionList::isDistributionList( addr ) ) {
    //kdDebug(5300) << "AddresseeLineEdit::addContact() distribution list \"" << addr.formattedName() << "\" weight=" << weight << endl;

    //for CompletionAuto
    addCompletionItem( addr.formattedName(), weight, source );

    //for CompletionShell, CompletionPopup
    QStringList sl( addr.formattedName() );
    addCompletionItem( addr.formattedName(), weight, source, &sl );

    return;
  }
#endif
  //m_contactMap.insert( addr.realName(), addr );
  const QStringList emails = addr.emails();
  QStringList::ConstIterator it;
  const int prefEmailWeight = 1;     //increment weight by prefEmailWeight
  int isPrefEmail = prefEmailWeight; //first in list is preferredEmail
  for ( it = emails.begin(); it != emails.end(); ++it ) {
    //TODO: highlight preferredEmail
    const QString email( (*it) );
    const QString givenName = addr.givenName();
    const QString familyName= addr.familyName();
    const QString nickName  = addr.nickName();
    const QString domain    = email.mid( email.find( '@' ) + 1 );
    QString fullEmail = addr.fullEmail( email );
    //TODO: let user decide what fields to use in lookup, e.g. company, city, ...

    //for CompletionAuto
    if ( givenName.isEmpty() && familyName.isEmpty() ) {
      addCompletionItem( fullEmail, weight + isPrefEmail, source ); // use whatever is there
    } else {
      const QString byFirstName=  "\"" + givenName + " " + familyName + "\" <" + email + ">";
      const QString byLastName =  "\"" + familyName + ", " + givenName + "\" <" + email + ">";
      addCompletionItem( byFirstName, weight + isPrefEmail, source );
      addCompletionItem( byLastName, weight + isPrefEmail, source );
    }

    addCompletionItem( email, weight + isPrefEmail, source );

    if ( !nickName.isEmpty() ){
      const QString byNick     =  "\"" + nickName + "\" <" + email + ">";
      addCompletionItem( byNick, weight + isPrefEmail, source );
    }

    if ( !domain.isEmpty() ){
      const QString byDomain   =  "\"" + domain + " " + familyName + " " + givenName + "\" <" + email + ">";
      addCompletionItem( byDomain, weight + isPrefEmail, source );
    }

    //for CompletionShell, CompletionPopup
    QStringList keyWords;
    const QString realName  = addr.realName();

    if ( !givenName.isEmpty() && !familyName.isEmpty() ) {
      keyWords.append( givenName  + " "  + familyName );
      keyWords.append( familyName + " "  + givenName );
      keyWords.append( familyName + ", " + givenName);
    }else if ( !givenName.isEmpty() )
      keyWords.append( givenName );
    else if ( !familyName.isEmpty() )
      keyWords.append( familyName );

    if ( !nickName.isEmpty() )
      keyWords.append( nickName );

    if ( !realName.isEmpty() )
      keyWords.append( realName );

    if ( !domain.isEmpty() )
      keyWords.append( domain );

    keyWords.append( email );

    /* KMailCompletion does not have knowledge about identities, it stores emails and
     * keywords for each email. KMailCompletion::allMatches does a lookup on the
     * keywords and returns an ordered list of emails. In order to get the preferred
     * email before others for each identity we use this little trick.
     * We remove the <blank> in getAdjustedCompletionItems.
     */
    if ( isPrefEmail == prefEmailWeight )
      fullEmail.replace( " <", "  <" );

    addCompletionItem( fullEmail, weight + isPrefEmail, source, &keyWords );
    isPrefEmail = 0;

#if 0
    int len = (*it).length();
    if ( len == 0 ) continue;
    if( '\0' == (*it)[len-1] )
      --len;
    const QString tmp = (*it).left( len );
    const QString fullEmail = addr.fullEmail( tmp );
    //kdDebug(5300) << "AddresseeLineEdit::addContact() \"" << fullEmail << "\" weight=" << weight << endl;
    addCompletionItem( fullEmail.simplifyWhiteSpace(), weight, source );
    // Try to guess the last name: if found, we add an extra
    // entry to the list to make sure completion works even
    // if the user starts by typing in the last name.
    QString name( addr.realName().simplifyWhiteSpace() );
    if( name.endsWith("\"") )
      name.truncate( name.length()-1 );
    if( name.startsWith("\"") )
      name = name.mid( 1 );

    // While we're here also add "email (full name)" for completion on the email
    if ( !name.isEmpty() )
      addCompletionItem( addr.preferredEmail() + " (" + name + ")", weight, source );

    bool bDone = false;
    int i = -1;
    while( ( i = name.findRev(' ') ) > 1 && !bDone ) {
      QString sLastName( name.mid( i+1 ) );
      if( ! sLastName.isEmpty() &&
            2 <= sLastName.length() &&   // last names must be at least 2 chars long
          ! sLastName.endsWith(".") ) { // last names must not end with a dot (like "Jr." or "Sr.")
        name.truncate( i );
        if( !name.isEmpty() ){
          sLastName.prepend( "\"" );
          sLastName.append( ", " + name + "\" <" );
        }
        QString sExtraEntry( sLastName );
        sExtraEntry.append( tmp.isEmpty() ? addr.preferredEmail() : tmp );
        sExtraEntry.append( ">" );
        //kdDebug(5300) << "AddresseeLineEdit::addContact() added extra \"" << sExtraEntry.simplifyWhiteSpace() << "\" weight=" << weight << endl;
        addCompletionItem( sExtraEntry.simplifyWhiteSpace(), weight, source );
        bDone = true;
      }
      if( !bDone ) {
        name.truncate( i );
        if( name.endsWith("\"") )
          name.truncate( name.length()-1 );
      }
    }
#endif
  }
}

void AddresseeLineEdit::addCompletionItem( const QString& string, int weight, int completionItemSource, const QStringList * keyWords )
{
  // Check if there is an exact match for item already, and use the max weight if so.
  // Since there's no way to get the information from KCompletion, we have to keep our own QMap
  CompletionItemsMap::iterator it = s_completionItemMap->find( string );
  if ( it != s_completionItemMap->end() ) {
    weight = QMAX( ( *it ).first, weight );
    ( *it ).first = weight;
  } else {
    s_completionItemMap->insert( string, qMakePair( weight, completionItemSource ) );
  }
  if ( keyWords == 0 )
    s_completion->addItem( string, weight );
  else
    s_completion->addItemWithKeys( string, weight, keyWords );
}

void AddresseeLineEdit::slotStartLDAPLookup()
{
  if ( !s_LDAPSearch->isAvailable() ) {
    return;
  }
  if (  s_LDAPLineEdit != this )
    return;

  startLoadingLDAPEntries();
}

void AddresseeLineEdit::stopLDAPLookup()
{
  s_LDAPSearch->cancelSearch();
  s_LDAPLineEdit = NULL;
}

void AddresseeLineEdit::startLoadingLDAPEntries()
{
  QString s( *s_LDAPText );
  // TODO cache last?
  QString prevAddr;
  int n = s.findRev( ',' );
  if ( n >= 0 ) {
    prevAddr = s.left( n + 1 ) + ' ';
    s = s.mid( n + 1, 255 ).stripWhiteSpace();
  }

  if ( s.isEmpty() )
    return;

  //loadContacts(); // TODO reuse these?
  s_LDAPSearch->startSearch( s );
}

void AddresseeLineEdit::slotLDAPSearchData( const KPIM::LdapResultList& adrs )
{
  if ( s_LDAPLineEdit != this )
    return;

  for ( KPIM::LdapResultList::ConstIterator it = adrs.begin(); it != adrs.end(); ++it ) {
    KABC::Addressee addr;
    addr.setNameFromString( (*it).name );
    addr.setEmails( (*it).email );

    addContact( addr, (*it).completionWeight, (*it ).clientNumber  );
  }

  if ( (hasFocus() || completionBox()->hasFocus() )
       && completionMode() != KGlobalSettings::CompletionNone
       && completionMode() != KGlobalSettings::CompletionShell) {
    setText( m_previousAddresses + m_searchString );
    doCompletion( m_lastSearchMode );
  }
}

void AddresseeLineEdit::setCompletedItems( const QStringList& items, bool autoSuggest )
{
    KCompletionBox* completionBox = this->completionBox();

    if ( !items.isEmpty() &&
         !(items.count() == 1 && m_searchString == items.first()) )
    {
        QString oldCurrentText = completionBox->currentText();
        QListBoxItem *itemUnderMouse = completionBox->itemAt(
            completionBox->viewport()->mapFromGlobal(QCursor::pos()) );
        QString oldTextUnderMouse;
        QPoint oldPosOfItemUnderMouse;
        if ( itemUnderMouse ) {
            oldTextUnderMouse = itemUnderMouse->text();
            oldPosOfItemUnderMouse = completionBox->itemRect( itemUnderMouse ).topLeft();
        }

        completionBox->setItems( items );

        if ( !completionBox->isVisible() ) {
          if ( !m_searchString.isEmpty() )
            completionBox->setCancelledText( m_searchString );
          completionBox->popup();
          // we have to install the event filter after popup(), since that
          // calls show(), and that's where KCompletionBox installs its filter.
          // We want to be first, though, so do it now.
          if ( s_completion->order() == KCompletion::Weighted )
            qApp->installEventFilter( this );
        }

        // Try to re-select what was selected before, otherrwise use the first
        // item, if there is one
        QListBoxItem* item = 0;
        if ( oldCurrentText.isEmpty()
           || ( item = completionBox->findItem( oldCurrentText ) ) == 0 ) {
            item = completionBox->item( 1 );
        }
        if ( item )
        {
          if ( itemUnderMouse ) {
              QListBoxItem *newItemUnderMouse = completionBox->findItem( oldTextUnderMouse );
              // if the mouse was over an item, before, but now that's elsewhere,
              // move the cursor, so folks don't accidently click the wrong item
              if ( newItemUnderMouse ) {
                  QRect r = completionBox->itemRect( newItemUnderMouse );
                  QPoint target = r.topLeft();
                  if ( oldPosOfItemUnderMouse != target ) {
                      target.setX( target.x() + r.width()/2 );
                      QCursor::setPos( completionBox->viewport()->mapToGlobal(target) );
                  }
              }
          }
          completionBox->blockSignals( true );
          completionBox->setSelected( item, true );
          completionBox->setCurrentItem( item );
          completionBox->ensureCurrentVisible();

          completionBox->blockSignals( false );
        }

        if ( autoSuggest )
        {
            int index = items.first().find( m_searchString );
            QString newText = items.first().mid( index );
            setUserSelection(false);
            setCompletedText(newText,true);
        }
    }
    else
    {
        if ( completionBox && completionBox->isVisible() ) {
            completionBox->hide();
            completionBox->setItems( QStringList() );
        }
    }
}

QPopupMenu* AddresseeLineEdit::createPopupMenu()
{
  QPopupMenu *menu = KLineEdit::createPopupMenu();
  if ( !menu )
    return 0;

  if ( m_useCompletion ){
    menu->setItemVisible( ShortAutoCompletion, false );
    menu->setItemVisible( PopupAutoCompletion, false );
    menu->insertItem( i18n( "Configure Completion Order..." ),
                      this, SLOT( slotEditCompletionOrder() ) );
  }
  return menu;
}

void AddresseeLineEdit::slotEditCompletionOrder()
{
  init(); // for s_LDAPSearch
  CompletionOrderEditor editor( s_LDAPSearch, this );
  editor.exec();
}

void KPIM::AddresseeLineEdit::slotIMAPCompletionOrderChanged()
{
  if ( m_useCompletion )
    s_addressesDirty = true;
}

void KPIM::AddresseeLineEdit::slotUserCancelled( const QString& cancelText )
{
  if ( s_LDAPSearch && s_LDAPLineEdit == this )
    stopLDAPLookup();
  userCancelled( m_previousAddresses + cancelText ); // in KLineEdit
}

void AddresseeLineEdit::updateSearchString()
{
  m_searchString = text();

  int n = -1;
  bool inQuote = false;
  for ( uint i = 0; i < m_searchString.length(); ++i ) {
    if ( m_searchString[ i ] == '"' )
      inQuote = !inQuote;
    if ( m_searchString[ i ] == '\\' && (i + 1) < m_searchString.length() && m_searchString[ i + 1 ] == '"' )
      ++i;
    if ( inQuote )
      continue;
    if ( m_searchString[ i ] == ',' || ( m_useSemiColonAsSeparator && m_searchString[ i ] == ';' ) )
      n = i;
  }

  if ( n >= 0 ) {
    ++n; // Go past the ","

    int len = m_searchString.length();

    // Increment past any whitespace...
    while ( n < len && m_searchString[ n ].isSpace() )
      ++n;

    m_previousAddresses = m_searchString.left( n );
    m_searchString = m_searchString.mid( n ).stripWhiteSpace();
  }
  else
  {
    m_previousAddresses = QString::null;
  }
}

void KPIM::AddresseeLineEdit::slotCompletion()
{
  // Called by KLineEdit's keyPressEvent for CompletionModes Auto,Popup -> new text, update search string
  // not called for CompletionShell, this is been taken care of in AddresseeLineEdit::keyPressEvent
  updateSearchString();
  if ( completionBox() )
    completionBox()->setCancelledText( m_searchString );
  doCompletion( false );
}

// not cached, to make sure we get an up-to-date value when it changes
KCompletion::CompOrder KPIM::AddresseeLineEdit::completionOrder()
{
  KConfig config( "kpimcompletionorder" );
  config.setGroup( "General" );
  const QString order = config.readEntry( "CompletionOrder", "Weighted" );

  if ( order == "Weighted" )
    return KCompletion::Weighted;
  else
    return KCompletion::Sorted;
}

int KPIM::AddresseeLineEdit::addCompletionSource( const QString &source )
{
  s_completionSources->append( source );
  return s_completionSources->size()-1;
}

bool KPIM::AddresseeLineEdit::eventFilter(QObject *obj, QEvent *e)
{
  if ( obj == completionBox() ) {
    if ( e->type() == QEvent::MouseButtonPress
      || e->type() == QEvent::MouseMove
      || e->type() == QEvent::MouseButtonRelease ) {
      QMouseEvent* me = static_cast<QMouseEvent*>( e );
      // find list box item at the event position
      QListBoxItem *item = completionBox()->itemAt( me->pos() );
      if ( !item ) {
        // In the case of a mouse move outside of the box we don't want
        // the parent to fuzzy select a header by mistake.
        bool eat = e->type() == QEvent::MouseMove;
        return eat;
      }
      // avoid selection of headers on button press, or move or release while
      // a button is pressed
      if ( e->type() == QEvent::MouseButtonPress
          || me->state() & LeftButton || me->state() & MidButton
          || me->state() & RightButton ) {
        if ( itemIsHeader(item) ) {
          return true; // eat the event, we don't want anything to happen
        } else {
          // if we are not on one of the group heading, make sure the item
          // below or above is selected, not the heading, inadvertedly, due
          // to fuzzy auto-selection from QListBox
          completionBox()->setCurrentItem( item );
          completionBox()->setSelected( completionBox()->index( item ), true );
          if ( e->type() == QEvent::MouseMove )
            return true; // avoid fuzzy selection behavior
        }
      }
    }
  }
  if ( ( obj == this ) &&
     ( e->type() == QEvent::AccelOverride ) ) {
    QKeyEvent *ke = static_cast<QKeyEvent*>( e );
    if ( ke->key() == Key_Up || ke->key() == Key_Down || ke->key() == Key_Tab ) {
      ke->accept();
      return true;
    }
  }
  if ( ( obj == this ) &&
      ( e->type() == QEvent::KeyPress ) &&
      completionBox()->isVisible() ) {
    QKeyEvent *ke = static_cast<QKeyEvent*>( e );
    unsigned int currentIndex = completionBox()->currentItem();
    if ( ke->key() == Key_Up ) {
      //kdDebug() << "EVENTFILTER: Key_Up currentIndex=" << currentIndex << endl;
      // figure out if the item we would be moving to is one we want
      // to ignore. If so, go one further
      QListBoxItem *itemAbove = completionBox()->item( currentIndex - 1 );
      if ( itemAbove && itemIsHeader(itemAbove) ) {
        // there is a header above us, check if there is even further up
        // and if so go one up, so it'll be selected
        if ( currentIndex > 1 && completionBox()->item( currentIndex - 2 ) ) {
          //kdDebug() << "EVENTFILTER: Key_Up -> skipping " << currentIndex - 1 << endl;
          completionBox()->setCurrentItem( itemAbove->prev() );
          completionBox()->setSelected( currentIndex - 2, true );
        } else if ( currentIndex == 1 ) {
            // nothing to skip to, let's stay where we are, but make sure the
            // first header becomes visible, if we are the first real entry
            completionBox()->ensureVisible( 0, 0 );
            completionBox()->setSelected( currentIndex, true );
        }
        return true;
      }
    } else if ( ke->key() == Key_Down  ) {
      // same strategy for downwards
      //kdDebug() << "EVENTFILTER: Key_Down. currentIndex=" << currentIndex << endl;
      QListBoxItem *itemBelow = completionBox()->item( currentIndex + 1 );
      if ( itemBelow && itemIsHeader( itemBelow ) ) {
        if ( completionBox()->item( currentIndex + 2 ) ) {
          //kdDebug() << "EVENTFILTER: Key_Down -> skipping " << currentIndex+1 << endl;
          completionBox()->setCurrentItem( itemBelow->next() );
          completionBox()->setSelected( currentIndex + 2, true );
        } else {
          // nothing to skip to, let's stay where we are
          completionBox()->setSelected( currentIndex, true );
        }
        return true;
      }
      // special case of the last and only item in the list needing selection
      if ( !itemBelow && currentIndex == 1 ) {
        completionBox()->setSelected( currentIndex, true );
      }
      // special case of the initial selection, which is unfortunately a header.
      // Setting it to selected tricks KCompletionBox into not treating is special
      // and selecting making it current, instead of the one below.
      QListBoxItem *item = completionBox()->item( currentIndex );
      if ( item && itemIsHeader(item) ) {
        completionBox()->setSelected( currentIndex, true );
      }
    } else if ( ke->key() == Key_Tab || ke->key() == Key_Backtab ) {
      /// first, find the header of teh current section
      QListBoxItem *myHeader = 0;
      int i = currentIndex;
      while ( i>=0 ) {
        if ( itemIsHeader( completionBox()->item(i) ) ) {
          myHeader = completionBox()->item( i );
          break;
        }
        i--;
      }
      Q_ASSERT( myHeader ); // we should always be able to find a header

      // find the next header (searching backwards, for Key_Backtab
      QListBoxItem *nextHeader = 0;
      const int iterationstep = ke->key() == Key_Tab ?  1 : -1;
      // when iterating forward, start at the currentindex, when backwards,
      // one up from our header, or at the end
      uint j = ke->key() == Key_Tab ? currentIndex : i==0 ? completionBox()->count()-1 : (i-1) % completionBox()->count();
      while ( ( nextHeader = completionBox()->item( j ) ) && nextHeader != myHeader ) {
          if ( itemIsHeader(nextHeader) ) {
              break;
          }
          j = (j + iterationstep) % completionBox()->count();
      }
      if ( nextHeader && nextHeader != myHeader ) {
        QListBoxItem *item = completionBox()->item( j + 1 );
        if ( item && !itemIsHeader(item) ) {
          completionBox()->setSelected( j+1, true );
          completionBox()->setCurrentItem( item );
          completionBox()->ensureCurrentVisible();
        }
      }
      return true;
    }
  }
  return ClickLineEdit::eventFilter( obj, e );
}

const QStringList KPIM::AddresseeLineEdit::getAdjustedCompletionItems( bool fullSearch )
{
  QStringList items = fullSearch ?
    s_completion->allMatches( m_searchString )
    : s_completion->substringCompletion( m_searchString );

  int lastSourceIndex = -1;
  unsigned int i = 0;
  QMap<int, QStringList> sections;
  QStringList sortedItems;
  for ( QStringList::Iterator it = items.begin(); it != items.end(); ++it, ++i ) {
    CompletionItemsMap::const_iterator cit = s_completionItemMap->find(*it);
    if ( cit == s_completionItemMap->end() )continue;
    int idx = (*cit).second;
    if ( s_completion->order() == KCompletion::Weighted ) {
      if ( lastSourceIndex == -1 || lastSourceIndex != idx ) {
        const QString sourceLabel(  (*s_completionSources)[idx] );
        if ( sections.find(idx) == sections.end() ) {
          items.insert( it, sourceLabel );
        }
        lastSourceIndex = idx;
      }
      (*it) = (*it).prepend( s_completionItemIndentString );
      // remove preferred email sort <blank> added in  addContact()
      (*it).replace( "  <", " <" );
    }
    sections[idx].append( *it );

    if ( s_completion->order() == KCompletion::Sorted ) {
      sortedItems.append( *it );
    }
  }
  if ( s_completion->order() == KCompletion::Weighted ) {
    for ( QMap<int, QStringList>::Iterator it( sections.begin() ), end( sections.end() ); it != end; ++it ) {
      sortedItems.append( (*s_completionSources)[it.key()] );
      for ( QStringList::Iterator sit( (*it).begin() ), send( (*it).end() ); sit != send; ++sit ) {
        sortedItems.append( *sit );
      }
    }
  } else {
    sortedItems.sort();
  }
  return sortedItems;
}
#include "addresseelineedit.moc"
