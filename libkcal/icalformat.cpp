/*
    This file is part of libkcal.

    Copyright (c) 2001 Cornelius Schumacher <schumacher@kde.org>

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

#include <qdatetime.h>
#include <qstring.h>
#include <qptrlist.h>
#include <qregexp.h>
#include <qclipboard.h>
#include <qfile.h>
#include <qtextstream.h>

#include <kdebug.h>
#include <klocale.h>

extern "C" {
  #include <ical.h>
  #include <icalss.h>
  #include <icalparser.h>
  #include <icalrestriction.h>
  #include <icalmemory.h>
}

#include "calendar.h"
#include "calendarlocal.h"
#include "journal.h"

#include "icalformat.h"
#include "icalformatimpl.h"
#include <ksavefile.h>

#include <stdio.h>

#define _ICAL_VERSION "2.0"

using namespace KCal;

ICalFormat::ICalFormat() : mImpl(0)
{
  setImplementation( new ICalFormatImpl( this ) );

  mTimeZoneId = "UTC";
  mUtc = true;
}

ICalFormat::~ICalFormat()
{
  delete mImpl;
}

void ICalFormat::setImplementation( ICalFormatImpl *impl )
{
  if ( mImpl ) delete mImpl;
  mImpl = impl;
}

#if defined(_AIX) && defined(open)
#undef open
#endif

bool ICalFormat::load( Calendar *calendar, const QString &fileName)
{
  kdDebug(5800) << "ICalFormat::load() " << fileName << endl;

  clearException();

  QFile file( fileName );
  if (!file.open( IO_ReadOnly ) ) {
    kdDebug(5800) << "ICalFormat::load() load error" << endl;
    setException(new ErrorFormat(ErrorFormat::LoadError));
    return false;
  }
  QTextStream ts( &file );
  ts.setEncoding( QTextStream::Latin1 );
  QString text = ts.read();
  file.close();

  if ( text.stripWhiteSpace().isEmpty() ) // empty files are valid
    return true;
  else
    return fromRawString( calendar, text.latin1() );
}


bool ICalFormat::save( Calendar *calendar, const QString &fileName )
{
  kdDebug(5800) << "ICalFormat::save(): " << fileName << endl;

  clearException();

  QString text = toString( calendar );

  if ( text.isNull() ) return false;

  // Write backup file
  KSaveFile::backupFile( fileName );

  KSaveFile file( fileName );
  if ( file.status() != 0 ) {
    kdDebug(5800) << "ICalFormat::save() errno: " << strerror( file.status() )
              << endl;
    setException( new ErrorFormat( ErrorFormat::SaveError,
                  i18n( "Error saving to '%1'." ).arg( fileName ) ) );
    return false;
  }

  // Convert to UTF8 and save
  QCString textUtf8 = text.utf8();
  file.file()->writeBlock( textUtf8.data(), textUtf8.size() - 1 );

  if ( !file.close() ) {
    kdDebug(5800) << "KSaveFile: close: status was " << file.status() << ". See errno.h." << endl;
    setException(new ErrorFormat(ErrorFormat::SaveError,
                 i18n("Could not save '%1'").arg(fileName)));
    return false;
  }

  return true;
}

bool ICalFormat::fromString( Calendar *cal, const QString &text )
{
  return fromRawString( cal, text.utf8() );
}

bool ICalFormat::fromRawString( Calendar *cal, const QCString &text )
{
  setTimeZone( cal->timeZoneId(), !cal->isLocalTime() );

  // Get first VCALENDAR component.
  // TODO: Handle more than one VCALENDAR or non-VCALENDAR top components
  icalcomponent *calendar;

  // Let's defend const correctness until the very gates of hell^Wlibical
  calendar = icalcomponent_new_from_string( const_cast<char*>( (const char*)text ) );
  //  kdDebug(5800) << "Error: " << icalerror_perror() << endl;
  if (!calendar) {
    kdDebug(5800) << "ICalFormat::load() parse error" << endl;
    setException(new ErrorFormat(ErrorFormat::ParseErrorIcal));
    return false;
  }

  bool success = true;

  if (icalcomponent_isa(calendar) == ICAL_XROOT_COMPONENT) {
    icalcomponent *comp;
    for ( comp = icalcomponent_get_first_component(calendar, ICAL_VCALENDAR_COMPONENT);
          comp != 0; comp = icalcomponent_get_next_component(calendar, ICAL_VCALENDAR_COMPONENT) ) {
      // put all objects into their proper places
      if ( !mImpl->populate( cal, comp ) ) {
        kdDebug(5800) << "ICalFormat::load(): Could not populate calendar" << endl;
        if ( !exception() ) {
          setException(new ErrorFormat(ErrorFormat::ParseErrorKcal));
        }
        success = false;
      } else {
        mLoadedProductId = mImpl->loadedProductId();
      }
      icalcomponent_free( comp );
    }
  } else if (icalcomponent_isa(calendar) != ICAL_VCALENDAR_COMPONENT) {
    kdDebug(5800) << "ICalFormat::load(): No VCALENDAR component found" << endl;
    setException(new ErrorFormat(ErrorFormat::NoCalendar));
    success = false;
  } else {
    // put all objects into their proper places
    if ( !mImpl->populate( cal, calendar ) ) {
      kdDebug(5800) << "ICalFormat::load(): Could not populate calendar" << endl;
      if ( !exception() ) {
        setException(new ErrorFormat(ErrorFormat::ParseErrorKcal));
      }
      success = false;
    } else
      mLoadedProductId = mImpl->loadedProductId();
  }

  icalcomponent_free( calendar );
  icalmemory_free_ring();

  return success;
}

Incidence *ICalFormat::fromString( const QString &text )
{
  CalendarLocal cal( mTimeZoneId );
  fromString(&cal, text);

  Incidence *ical = 0;
  Event::List elist = cal.events();
  if ( elist.count() > 0 ) {
    ical = elist.first();
  } else {
    Todo::List tlist = cal.todos();
    if ( tlist.count() > 0 ) {
      ical = tlist.first();
    } else {
      Journal::List jlist = cal.journals();
      if ( jlist.count() > 0 ) {
        ical = jlist.first();
      }
    }
  }

  return ical ? ical->clone() : 0;
}

QString ICalFormat::toString( Calendar *cal )
{
  setTimeZone( cal->timeZoneId(), !cal->isLocalTime() );

  icalcomponent *calendar = mImpl->createCalendarComponent(cal);

  icalcomponent *component;

  // todos
  Todo::List todoList = cal->rawTodos();
  Todo::List::ConstIterator it;
  for( it = todoList.begin(); it != todoList.end(); ++it ) {
//    kdDebug(5800) << "ICalFormat::toString() write todo "
//                  << (*it)->uid() << endl;
    component = mImpl->writeTodo( *it );
    icalcomponent_add_component( calendar, component );
  }

  // events
  Event::List events = cal->rawEvents();
  Event::List::ConstIterator it2;
  for( it2 = events.begin(); it2 != events.end(); ++it2 ) {
//    kdDebug(5800) << "ICalFormat::toString() write event "
//                  << (*it2)->uid() << endl;
    component = mImpl->writeEvent( *it2 );
    icalcomponent_add_component( calendar, component );
  }

  // journals
  Journal::List journals = cal->journals();
  Journal::List::ConstIterator it3;
  for( it3 = journals.begin(); it3 != journals.end(); ++it3 ) {
    kdDebug(5800) << "ICalFormat::toString() write journal "
                  << (*it3)->uid() << endl;
    component = mImpl->writeJournal( *it3 );
    icalcomponent_add_component( calendar, component );
  }

  QString text = QString::fromUtf8( icalcomponent_as_ical_string( calendar ) );

  icalcomponent_free( calendar );
  icalmemory_free_ring();

  if (!text) {
    setException(new ErrorFormat(ErrorFormat::SaveError,
                 i18n("libical error")));
    return QString::null;
  }

  return text;
}

QString ICalFormat::toICalString( Incidence *incidence )
{
  CalendarLocal cal( mTimeZoneId );
  cal.addIncidence( incidence->clone() );
  return toString( &cal );
}

QString ICalFormat::toString( Incidence *incidence )
{
  icalcomponent *component;

  component = mImpl->writeIncidence( incidence );

  QString text = QString::fromUtf8( icalcomponent_as_ical_string( component ) );

  icalcomponent_free( component );

  return text;
}

QString ICalFormat::toString( RecurrenceRule *recurrence )
{
  icalproperty *property;
  property = icalproperty_new_rrule( mImpl->writeRecurrenceRule( recurrence ) );
  QString text = QString::fromUtf8( icalproperty_as_ical_string( property ) );
  icalproperty_free( property );
  return text;
}

bool ICalFormat::fromString( RecurrenceRule * recurrence, const QString& rrule )
{
  if ( !recurrence ) return false;
  bool success = true;
  icalerror_clear_errno();
  struct icalrecurrencetype recur = icalrecurrencetype_from_string( rrule.latin1() );
  if ( icalerrno != ICAL_NO_ERROR ) {
    kdDebug(5800) << "Recurrence parsing error: " << icalerror_strerror( icalerrno ) << endl;
    success = false;
  }

  if ( success ) {
    mImpl->readRecurrence( recur, recurrence );
  }

  return success;
}


QString ICalFormat::createScheduleMessage(IncidenceBase *incidence,
                                          Scheduler::Method method)
{
  icalcomponent *message = 0;

  // Handle scheduling ID being present
  if ( incidence->type() == "Event" || incidence->type() == "Todo" ) {
    Incidence* i = static_cast<Incidence*>( incidence );
    if ( i->schedulingID() != i->uid() ) {
      // We have a separation of scheduling ID and UID
      i = i->clone();
      i->setUid( i->schedulingID() );
      i->setSchedulingID( QString::null );

      // Build the message with the cloned incidence
      message = mImpl->createScheduleComponent( i, method );

      // And clean up
      delete i;
    }
  }

  if ( message == 0 )
    message = mImpl->createScheduleComponent(incidence,method);

  // FIXME TODO: Don't we have to free message? What about the ical_string? MEMLEAK
  QString messageText = QString::fromUtf8( icalcomponent_as_ical_string(message) );

#if 0
  kdDebug(5800) << "ICalFormat::createScheduleMessage: message START\n"
            << messageText
            << "ICalFormat::createScheduleMessage: message END" << endl;
#endif

  return messageText;
}

FreeBusy *ICalFormat::parseFreeBusy( const QString &str )
{
  clearException();

  icalcomponent *message;
  message = icalparser_parse_string( str.utf8() );

  if ( !message ) return 0;

  FreeBusy *freeBusy = 0;

  icalcomponent *c;
  for ( c = icalcomponent_get_first_component( message, ICAL_VFREEBUSY_COMPONENT );
        c != 0; c = icalcomponent_get_next_component( message, ICAL_VFREEBUSY_COMPONENT ) ) {
    FreeBusy *fb = mImpl->readFreeBusy( c );

    if ( freeBusy ) {
      freeBusy->merge( fb );
      delete fb;
    } else {
      freeBusy = fb;
    }
  }

  if ( !freeBusy )
    kdDebug(5800) << "ICalFormat:parseFreeBusy: object is not a freebusy."
                  << endl;
  return freeBusy;
}

ScheduleMessage *ICalFormat::parseScheduleMessage( Calendar *cal,
                                                   const QString &messageText )
{
  setTimeZone( cal->timeZoneId(), !cal->isLocalTime() );
  clearException();

  if (messageText.isEmpty())
  {
    setException( new ErrorFormat( ErrorFormat::ParseErrorKcal, QString::fromLatin1( "messageText was empty, unable to parse into a ScheduleMessage" ) ) );
    return 0;
  }
  // TODO FIXME: Don't we have to ical-free message??? MEMLEAK
  icalcomponent *message;
  message = icalparser_parse_string(messageText.utf8());

  if (!message)
  {
    setException( new ErrorFormat( ErrorFormat::ParseErrorKcal, QString::fromLatin1( "icalparser was unable to parse messageText into a ScheduleMessage" ) ) );
    return 0;
  }

  icalproperty *m = icalcomponent_get_first_property(message,
                                                     ICAL_METHOD_PROPERTY);
  if (!m)
  {
    setException( new ErrorFormat( ErrorFormat::ParseErrorKcal, QString::fromLatin1( "message didn't contain an ICAL_METHOD_PROPERTY" ) ) );
    return 0;
  }

  icalcomponent *c;

  IncidenceBase *incidence = 0;
  c = icalcomponent_get_first_component(message,ICAL_VEVENT_COMPONENT);
  if (c) {
    icalcomponent *ctz = icalcomponent_get_first_component(message,ICAL_VTIMEZONE_COMPONENT);
    incidence = mImpl->readEvent(c, ctz);
  }

  if (!incidence) {
    c = icalcomponent_get_first_component(message,ICAL_VTODO_COMPONENT);
    if (c) {
      incidence = mImpl->readTodo(c);
    }
  }

  if (!incidence) {
    c = icalcomponent_get_first_component(message,ICAL_VJOURNAL_COMPONENT);
    if (c) {
      incidence = mImpl->readJournal(c);
    }
  }

  if (!incidence) {
    c = icalcomponent_get_first_component(message,ICAL_VFREEBUSY_COMPONENT);
    if (c) {
      incidence = mImpl->readFreeBusy(c);
    }
  }



  if (!incidence) {
    kdDebug(5800) << "ICalFormat:parseScheduleMessage: object is not a freebusy, event, todo or journal" << endl;
    setException( new ErrorFormat( ErrorFormat::ParseErrorKcal, QString::fromLatin1( "object is not a freebusy, event, todo or journal" ) ) );
    return 0;
  }

  kdDebug(5800) << "ICalFormat::parseScheduleMessage() getting method..." << endl;

  icalproperty_method icalmethod = icalproperty_get_method(m);
  Scheduler::Method method;

  switch (icalmethod) {
    case ICAL_METHOD_PUBLISH:
      method = Scheduler::Publish;
      break;
    case ICAL_METHOD_REQUEST:
      method = Scheduler::Request;
      break;
    case ICAL_METHOD_REFRESH:
      method = Scheduler::Refresh;
      break;
    case ICAL_METHOD_CANCEL:
      method = Scheduler::Cancel;
      break;
    case ICAL_METHOD_ADD:
      method = Scheduler::Add;
      break;
    case ICAL_METHOD_REPLY:
      method = Scheduler::Reply;
      break;
    case ICAL_METHOD_COUNTER:
      method = Scheduler::Counter;
      break;
    case ICAL_METHOD_DECLINECOUNTER:
      method = Scheduler::Declinecounter;
      break;
    default:
      method = Scheduler::NoMethod;
      kdDebug(5800) << "ICalFormat::parseScheduleMessage(): Unknow method" << endl;
      break;
  }

  kdDebug(5800) << "ICalFormat::parseScheduleMessage() restriction..." << endl;

  if (!icalrestriction_check(message)) {
    kdWarning(5800) << k_funcinfo << endl << "libkcal reported a problem while parsing:" << endl;
    kdWarning(5800) << Scheduler::translatedMethodName(method) + ": " + mImpl->extractErrorProperty(c)<< endl;
    /*
    setException(new ErrorFormat(ErrorFormat::Restriction,
                                   Scheduler::translatedMethodName(method) + ": " +
                                   mImpl->extractErrorProperty(c)));
    delete incidence;
    return 0;
    */
  }
  icalcomponent *calendarComponent = mImpl->createCalendarComponent(cal);

  Incidence *existingIncidence =
    cal->incidenceFromSchedulingID(incidence->uid());
  if (existingIncidence) {
    // TODO: check, if cast is required, or if it can be done by virtual funcs.
    // TODO: Use a visitor for this!
    if (existingIncidence->type() == "Todo") {
      Todo *todo = static_cast<Todo *>(existingIncidence);
      icalcomponent_add_component(calendarComponent,
                                  mImpl->writeTodo(todo));
    }
    if (existingIncidence->type() == "Event") {
      Event *event = static_cast<Event *>(existingIncidence);
      icalcomponent_add_component(calendarComponent,
                                  mImpl->writeEvent(event));
    }
  } else {
    calendarComponent = 0;
  }

  kdDebug(5800) << "ICalFormat::parseScheduleMessage() classify..." << endl;

  icalproperty_xlicclass result = icalclassify( message, calendarComponent,
                                                (char *)"" );

  kdDebug(5800) << "ICalFormat::parseScheduleMessage() returning..." << endl;
  kdDebug(5800) << "ICalFormat::parseScheduleMessage(), result = " << result << endl;

  ScheduleMessage::Status status;

  switch (result) {
    case ICAL_XLICCLASS_PUBLISHNEW:
      status = ScheduleMessage::PublishNew;
      break;
    case ICAL_XLICCLASS_PUBLISHUPDATE:
      status = ScheduleMessage::PublishUpdate;
      break;
    case ICAL_XLICCLASS_OBSOLETE:
      status = ScheduleMessage::Obsolete;
      break;
    case ICAL_XLICCLASS_REQUESTNEW:
      status = ScheduleMessage::RequestNew;
      break;
    case ICAL_XLICCLASS_REQUESTUPDATE:
      status = ScheduleMessage::RequestUpdate;
      break;
    case ICAL_XLICCLASS_UNKNOWN:
    default:
      status = ScheduleMessage::Unknown;
      break;
  }

  kdDebug(5800) << "ICalFormat::parseScheduleMessage(), status = " << status << endl;
// TODO FIXME: Don't we have to free calendarComponent??? MEMLEAK

  return new ScheduleMessage(incidence,method,status);
}

void ICalFormat::setTimeZone( const QString &id, bool utc )
{
  mTimeZoneId = id;
  mUtc = utc;
}

QString ICalFormat::timeZoneId() const
{
  return mTimeZoneId;
}

bool ICalFormat::utc() const
{
  return mUtc;
}
