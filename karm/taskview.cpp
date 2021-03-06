#include <qclipboard.h>
#include <qfile.h>
#include <qlayout.h>
#include <qlistbox.h>
#include <qlistview.h>
#include <qptrlist.h>
#include <qptrstack.h>
#include <qstring.h>
#include <qtextstream.h>
#include <qtimer.h>
#include <qxml.h>

#include "kapplication.h"       // kapp
#include <kconfig.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <klocale.h>            // i18n
#include <kmessagebox.h>
#include <kurlrequester.h>

#include "csvexportdialog.h"
#include "desktoptracker.h"
#include "edittaskdialog.h"
#include "idletimedetector.h"
#include "karmstorage.h"
#include "plannerparser.h"
#include "preferences.h"
#include "printdialog.h"
#include "reportcriteria.h"
#include "task.h"
#include "taskview.h"
#include "timekard.h"
#include "taskviewwhatsthis.h"

#define T_LINESIZE 1023
#define HIDDEN_COLUMN -10

class DesktopTracker;

TaskView::TaskView(QWidget *parent, const char *name, const QString &icsfile ):KListView(parent,name)
{
  _preferences = Preferences::instance( icsfile );
  _storage = KarmStorage::instance();

  connect( this, SIGNAL( expanded( QListViewItem * ) ),
           this, SLOT( itemStateChanged( QListViewItem * ) ) );
  connect( this, SIGNAL( collapsed( QListViewItem * ) ),
           this, SLOT( itemStateChanged( QListViewItem * ) ) );

  // setup default values
  previousColumnWidths[0] = previousColumnWidths[1]
  = previousColumnWidths[2] = previousColumnWidths[3] = HIDDEN_COLUMN;

  addColumn( i18n("Task Name") );
  addColumn( i18n("Session Time") );
  addColumn( i18n("Time") );
  addColumn( i18n("Total Session Time") );
  addColumn( i18n("Total Time") );
  setColumnAlignment( 1, Qt::AlignRight );
  setColumnAlignment( 2, Qt::AlignRight );
  setColumnAlignment( 3, Qt::AlignRight );
  setColumnAlignment( 4, Qt::AlignRight );
  adaptColumns();
  setAllColumnsShowFocus( true );

  // set up the minuteTimer
  _minuteTimer = new QTimer(this);
  connect( _minuteTimer, SIGNAL( timeout() ), this, SLOT( minuteUpdate() ));
  _minuteTimer->start(1000 * secsPerMinute);

  // React when user changes iCalFile
  connect(_preferences, SIGNAL(iCalFile(QString)),
      this, SLOT(iCalFileChanged(QString)));

  // resize columns when config is changed
  connect(_preferences, SIGNAL( setupChanged() ), this,SLOT( adaptColumns() ));

  _minuteTimer->start(1000 * secsPerMinute);

  // Set up the idle detection.
  _idleTimeDetector = new IdleTimeDetector( _preferences->idlenessTimeout() );
  connect( _idleTimeDetector, SIGNAL( extractTime(int) ),
           this, SLOT( extractTime(int) ));
  connect( _idleTimeDetector, SIGNAL( stopAllTimersAt(QDateTime) ),
           this, SLOT( stopAllTimersAt(QDateTime) ));
  connect( _preferences, SIGNAL( idlenessTimeout(int) ),
           _idleTimeDetector, SLOT( setMaxIdle(int) ));
  connect( _preferences, SIGNAL( detectIdleness(bool) ),
           _idleTimeDetector, SLOT( toggleOverAllIdleDetection(bool) ));
  if (!_idleTimeDetector->isIdleDetectionPossible())
    _preferences->disableIdleDetection();

  // Setup auto save timer
  _autoSaveTimer = new QTimer(this);
  connect( _preferences, SIGNAL( autoSave(bool) ),
           this, SLOT( autoSaveChanged(bool) ));
  connect( _preferences, SIGNAL( autoSavePeriod(int) ),
           this, SLOT( autoSavePeriodChanged(int) ));
  connect( _autoSaveTimer, SIGNAL( timeout() ), this, SLOT( save() ));

  // Setup manual save timer (to save changes a little while after they happen)
  _manualSaveTimer = new QTimer(this);
  connect( _manualSaveTimer, SIGNAL( timeout() ), this, SLOT( save() ));

  // Connect desktop tracker events to task starting/stopping
  _desktopTracker = new DesktopTracker();
  connect( _desktopTracker, SIGNAL( reachedtActiveDesktop( Task* ) ),
           this, SLOT( startTimerFor(Task*) ));
  connect( _desktopTracker, SIGNAL( leftActiveDesktop( Task* ) ),
           this, SLOT( stopTimerFor(Task*) ));
  new TaskViewWhatsThis( this );
}

KarmStorage* TaskView::storage()
{
  return _storage;
}

void TaskView::contentsMousePressEvent ( QMouseEvent * e )
{
  kdDebug(5970) << "entering contentsMousePressEvent" << endl;
  KListView::contentsMousePressEvent(e);
  Task* task = current_item();
  // This checks that there has been a click onto an item,  
  // not into an empty part of the KListView.
  if ( task != 0 &&  // zero can happen if there is no task
        e->pos().y() >= current_item()->itemPos() &&
        e->pos().y() < current_item()->itemPos()+current_item()->height() )
  { 
    // see if the click was on the completed icon
    int leftborder = treeStepSize() * ( task->depth() + ( rootIsDecorated() ? 1 : 0)) + itemMargin();
    if ((leftborder < e->x()) && (e->x() < 19 + leftborder ))
    {
      if ( e->button() == LeftButton )
        if ( task->isComplete() ) task->setPercentComplete( 0, _storage );
        else task->setPercentComplete( 100, _storage );
    }
    emit updateButtons();
  }
}

void TaskView::contentsMouseDoubleClickEvent ( QMouseEvent * e )
// if the user double-clicks onto a tasks, he says "I am now working exclusively
// on that task". That means, on a doubleclick, we check if it occurs on an item
// not in the blank space, if yes, stop all other tasks and start the new timer.
{
  kdDebug(5970) << "entering contentsMouseDoubleClickEvent" << endl;
  KListView::contentsMouseDoubleClickEvent(e);
  
  Task *task = current_item();

  if ( task != 0 )  // current_item() exists
  {
    if ( e->pos().y() >= task->itemPos() &&  // doubleclick was onto current_item()
          e->pos().y() < task->itemPos()+task->height() )
    {
      if ( activeTasks.findRef(task) == -1 )  // task is active
      {
        stopAllTimers();
        startCurrentTimer();
      }
      else stopCurrentTimer();
    }
  }
}

TaskView::~TaskView()
{
  _preferences->save();
}

Task* TaskView::first_child() const
{
  return static_cast<Task*>(firstChild());
}

Task* TaskView::current_item() const
{
  return static_cast<Task*>(currentItem());
}

Task* TaskView::item_at_index(int i)
{
  return static_cast<Task*>(itemAtIndex(i));
}

void TaskView::load( QString fileName )
{
  // if the program is used as an embedded plugin for konqueror, there may be a need
  // to load from a file without touching the preferences.
  _isloading = true;
  QString err = _storage->load(this, _preferences, fileName);

  if (!err.isEmpty())
  {
    KMessageBox::error(this, err);
    _isloading = false;
    return;
  }

  // Register tasks with desktop tracker
  int i = 0;
  for ( Task* t = item_at_index(i); t; t = item_at_index(++i) )
    _desktopTracker->registerForDesktops( t, t->getDesktops() );

  restoreItemState( first_child() );

  setSelected(first_child(), true);
  setCurrentItem(first_child());
  if ( _desktopTracker->startTracking() != QString() ) 
    KMessageBox::error( 0, i18n("You are on a too high logical desktop, desktop tracking will not work") );
  _isloading = false;
  refresh();
}

void TaskView::restoreItemState( QListViewItem *item )
{
  while( item ) 
  {
    Task *t = (Task *)item;
    t->setOpen( _preferences->readBoolEntry( t->uid() ) );
    if( item->childCount() > 0 ) restoreItemState( item->firstChild() );
    item = item->nextSibling();
  }
}

void TaskView::itemStateChanged( QListViewItem *item )
{
  if ( !item || _isloading ) return;
  Task *t = (Task *)item;
  kdDebug(5970) << "TaskView::itemStateChanged()" 
    << " uid=" << t->uid() << " state=" << t->isOpen()
    << endl;
  if( _preferences ) _preferences->writeEntry( t->uid(), t->isOpen() );
}

void TaskView::closeStorage() { _storage->closeStorage( this ); }

void TaskView::iCalFileModified(ResourceCalendar *rc)
{
  kdDebug(5970) << "entering iCalFileModified" << endl;
  kdDebug(5970) << rc->infoText() << endl;
  rc->dump();
  _storage->buildTaskView(rc,this);
  kdDebug(5970) << "exiting iCalFileModified" << endl;
}

void TaskView::refresh()
{
  kdDebug(5970) << "entering TaskView::refresh()" << endl;
  this->setRootIsDecorated(true);
  int i = 0;
  for ( Task* t = item_at_index(i); t; t = item_at_index(++i) )
  {
    t->setPixmapProgress();
  }
  
  // remove root decoration if there is no more children.
  bool anyChilds = false;
  for(Task* child = first_child();
            child;
            child = child->nextSibling()) {
    if (child->childCount() != 0) {
      anyChilds = true;
      break;
    }
  }
  if (!anyChilds) {
    setRootIsDecorated(false);
  }
  emit updateButtons();
  kdDebug(5970) << "exiting TaskView::refresh()" << endl;
}
    
void TaskView::loadFromFlatFile()
{
  kdDebug(5970) << "TaskView::loadFromFlatFile()" << endl;

  //KFileDialog::getSaveFileName("icalout.ics",i18n("*.ics|ICalendars"),this);

  QString fileName(KFileDialog::getOpenFileName(QString::null, QString::null,
        0));
  if (!fileName.isEmpty()) {
    QString err = _storage->loadFromFlatFile(this, fileName);
    if (!err.isEmpty())
    {
      KMessageBox::error(this, err);
      return;
    }
    // Register tasks with desktop tracker
    int task_idx = 0;
    Task* task = item_at_index(task_idx++);
    while (task)
    {
      // item_at_index returns 0 where no more items.
      _desktopTracker->registerForDesktops( task, task->getDesktops() );
      task = item_at_index(task_idx++);
    }

    setSelected(first_child(), true);
    setCurrentItem(first_child());

    if ( _desktopTracker->startTracking() != QString() )
      KMessageBox::error(0, i18n("You are on a too high logical desktop, desktop tracking will not work") );
  }
}

QString TaskView::importPlanner(QString fileName)
{
  kdDebug(5970) << "entering importPlanner" << endl;
  PlannerParser* handler=new PlannerParser(this);
  if (fileName.isEmpty()) fileName=KFileDialog::getOpenFileName(QString::null, QString::null, 0);
  QFile xmlFile( fileName );
  QXmlInputSource source( xmlFile );
  QXmlSimpleReader reader;
  reader.setContentHandler( handler );
  reader.parse( source );
  refresh();
  return "";
}

QString TaskView::report( const ReportCriteria& rc )
{
  return _storage->report( this, rc );
}

void TaskView::exportcsvFile()
{
  kdDebug(5970) << "TaskView::exportcsvFile()" << endl;

  CSVExportDialog dialog( ReportCriteria::CSVTotalsExport, this );
  if ( current_item() && current_item()->isRoot() )
    dialog.enableTasksToExportQuestion();
  dialog.urlExportTo->KURLRequester::setMode(KFile::File);
  if ( dialog.exec() ) {
    QString err = _storage->report( this, dialog.reportCriteria() );
    if ( !err.isEmpty() ) KMessageBox::error( this, i18n(err.ascii()) );
  }
}

QString TaskView::exportcsvHistory()
{
  kdDebug(5970) << "TaskView::exportcsvHistory()" << endl;
  QString err;
  
  CSVExportDialog dialog( ReportCriteria::CSVHistoryExport, this );
  if ( current_item() && current_item()->isRoot() )
    dialog.enableTasksToExportQuestion();
  dialog.urlExportTo->KURLRequester::setMode(KFile::File);
  if ( dialog.exec() ) {
    err = _storage->report( this, dialog.reportCriteria() );
  }
  return err;
}

void TaskView::scheduleSave()
{
  kdDebug(5970) << "Entering TaskView::scheduleSave" << endl;
  // save changes a little while after they happen
  _manualSaveTimer->start( 10, true /*single-shot*/ );
}

Preferences* TaskView::preferences() { return _preferences; }

QString TaskView::save()
// This saves the tasks. If they do not yet have an endDate, their startDate is also not saved.
{
  kdDebug(5970) << "Entering TaskView::save" << endl;
  QString err = _storage->save(this);
  emit(setStatusBar(err));
  return err;
}

void TaskView::startCurrentTimer()
{
  startTimerFor( current_item() );
}

long TaskView::count()
{
  long n = 0;
  for (Task* t = item_at_index(n); t; t=item_at_index(++n));
  return n;
}

void TaskView::startTimerFor(Task* task, QDateTime startTime )
{
  kdDebug(5970) << "Entering TaskView::startTimerFor" << endl;
  if (save()==QString())
  {
    if (task != 0 && activeTasks.findRef(task) == -1) 
    {
      _idleTimeDetector->startIdleDetection();
      if (!task->isComplete())
      {
        task->setRunning(true, _storage, startTime);
        activeTasks.append(task);
        emit updateButtons();
        if ( activeTasks.count() == 1 )
          emit timersActive();
        emit tasksChanged( activeTasks);
      }
    }
  }
  else KMessageBox::error(0,i18n("Saving is impossible, so timing is useless. \nSaving problems may result from a full harddisk, a directory name instead of a file name, or stale locks. Check that your harddisk has enough space, that your calendar file exists and is a file and remove stale locks, typically from ~/.kde/share/apps/kabc/lock."));
}

void TaskView::clearActiveTasks()
{
  activeTasks.clear();
}

void TaskView::stopAllTimers()
{
  kdDebug(5970) << "Entering TaskView::stopAllTimers()" << endl;
  for ( unsigned int i = 0; i < activeTasks.count(); i++ )
    activeTasks.at(i)->setRunning(false, _storage);

  _idleTimeDetector->stopIdleDetection();
  activeTasks.clear();
  emit updateButtons();
  emit timersInactive();
  emit tasksChanged( activeTasks );
}

void TaskView::stopAllTimersAt(QDateTime qdt)
// stops all timers for the time qdt. This makes sense, if the idletimedetector detected
// the last work has been done 50 minutes ago.
{
  kdDebug(5970) << "Entering TaskView::stopAllTimersAt " << qdt << endl;
  for ( unsigned int i = 0; i < activeTasks.count(); i++ )
  {
    activeTasks.at(i)->setRunning(false, _storage, qdt, qdt);
    kdDebug() << activeTasks.at(i)->name() << endl;
  }

  _idleTimeDetector->stopIdleDetection();
  activeTasks.clear();
  emit updateButtons();
  emit timersInactive();
  emit tasksChanged( activeTasks );
}

void TaskView::startNewSession()
{
  QListViewItemIterator item( first_child());
  for ( ; item.current(); ++item ) {
    Task * task = (Task *) item.current();
    task->startNewSession();
  }
}

void TaskView::resetTimeForAllTasks()
{
  QListViewItemIterator item( first_child());
  for ( ; item.current(); ++item ) {
    Task * task = (Task *) item.current();
    task->resetTimes();
  }
}

void TaskView::stopTimerFor(Task* task)
{
  kdDebug(5970) << "Entering stopTimerFor. task = " << task->name() << endl;
  if ( task != 0 && activeTasks.findRef(task) != -1 ) {
    activeTasks.removeRef(task);
    task->setRunning(false, _storage);
    if ( activeTasks.count() == 0 ) {
      _idleTimeDetector->stopIdleDetection();
      emit timersInactive();
    }
    emit updateButtons();
  }
  emit tasksChanged( activeTasks);
}

void TaskView::stopCurrentTimer()
{
  stopTimerFor( current_item());
}

void TaskView::minuteUpdate()
{
  addTimeToActiveTasks(1, false);
}

void TaskView::addTimeToActiveTasks(int minutes, bool save_data)
{
  for( unsigned int i = 0; i < activeTasks.count(); i++ )
    activeTasks.at(i)->changeTime(minutes, ( save_data ? _storage : 0 ) );
}

void TaskView::newTask()
{
  newTask(i18n("New Task"), 0);
}

void TaskView::newTask(QString caption, Task *parent)
{
  EditTaskDialog *dialog = new EditTaskDialog(caption, false);
  long total, totalDiff, session, sessionDiff;
  DesktopList desktopList;

  int result = dialog->exec();
  if ( result == QDialog::Accepted ) {
    QString taskName = i18n( "Unnamed Task" );
    if ( !dialog->taskName().isEmpty()) taskName = dialog->taskName();

    total = totalDiff = session = sessionDiff = 0;
    dialog->status( &total, &totalDiff, &session, &sessionDiff, &desktopList );

    // If all available desktops are checked, disable auto tracking,
    // since it makes no sense to track for every desktop.
    if ( desktopList.size() == ( unsigned int ) _desktopTracker->desktopCount() )
      desktopList.clear();

    QString uid = addTask( taskName, total, session, desktopList, parent );
    if ( uid.isNull() )
    {
      KMessageBox::error( 0, i18n(
            "Error storing new task. Your changes were not saved. Make sure you can edit your iCalendar file. Also quit all applications using this file and remove any lock file related to its name from ~/.kde/share/apps/kabc/lock/ " ) );
    }

    delete dialog;
  }
}

QString TaskView::addTask
( const QString& taskname, long total, long session, 
  const DesktopList& desktops, Task* parent )
{
  Task *task;
  kdDebug(5970) << "TaskView::addTask: taskname = " << taskname << endl;

  if ( parent ) task = new Task( taskname, total, session, desktops, parent );
  else          task = new Task( taskname, total, session, desktops, this );

  task->setUid( _storage->addTask( task, parent ) );
  QString taskuid=task->uid();
  if ( ! taskuid.isNull() )
  {
    _desktopTracker->registerForDesktops( task, desktops );
    setCurrentItem( task );
    setSelected( task, true );
    task->setPixmapProgress();
    save();
  }
  else
  {
    delete task;
  }
  return taskuid;
}

void TaskView::newSubTask()
{
  Task* task = current_item();
  if(!task)
    return;
  newTask(i18n("New Sub Task"), task);
  task->setOpen(true);
  refresh();
}

void TaskView::editTask()
{
  Task *task = current_item();
  if (!task)
    return;

  DesktopList desktopList = task->getDesktops();
  EditTaskDialog *dialog = new EditTaskDialog(i18n("Edit Task"), true, &desktopList);
  dialog->setTask( task->name(),
                   task->time(),
                   task->sessionTime() );
  int result = dialog->exec();
  if (result == QDialog::Accepted) {
    QString taskName = i18n("Unnamed Task");
    if (!dialog->taskName().isEmpty()) {
      taskName = dialog->taskName();
    }
    // setName only does something if the new name is different
    task->setName(taskName, _storage);

    // update session time as well if the time was changed
    long total, session, totalDiff, sessionDiff;
    total = totalDiff = session = sessionDiff = 0;
    DesktopList desktopList;
    dialog->status( &total, &totalDiff, &session, &sessionDiff, &desktopList);

    if( totalDiff != 0 || sessionDiff != 0)
      task->changeTimes( sessionDiff, totalDiff, _storage );

    // If all available desktops are checked, disable auto tracking,
    // since it makes no sense to track for every desktop.
    if (desktopList.size() == (unsigned int)_desktopTracker->desktopCount())
      desktopList.clear();

    task->setDesktopList(desktopList);

    _desktopTracker->registerForDesktops( task, desktopList );

    emit updateButtons();
  }
  delete dialog;
}

//void TaskView::addCommentToTask()
//{
//  Task *task = current_item();
//  if (!task)
//    return;

//  bool ok;
//  QString comment = KLineEditDlg::getText(i18n("Comment"),
//                       i18n("Log comment for task '%1':").arg(task->name()),
//                       QString(), &ok, this);
//  if ( ok )
//    task->addComment( comment, _storage );
//}

void TaskView::reinstateTask(int completion)
{
  Task* task = current_item();
  if (task == 0) {
    KMessageBox::information(0,i18n("No task selected."));
    return;
  }

  if (completion<0) completion=0;
  if (completion<100)
  {
    task->setPercentComplete(completion, _storage);
    task->setPixmapProgress();
    save();
    emit updateButtons();
  }
}

void TaskView::deleteTask(bool markingascomplete)
{
  Task *task = current_item();
  if (task == 0) {
    KMessageBox::information(0,i18n("No task selected."));
    return;
  }

  int response = KMessageBox::Continue;
  if (!markingascomplete && _preferences->promptDelete()) {
    if (task->childCount() == 0) {
      response = KMessageBox::warningContinueCancel( 0,
          i18n( "Are you sure you want to delete "
          "the task named\n\"%1\" and its entire history?")
          .arg(task->name()),
          i18n( "Deleting Task"), KStdGuiItem::del());
    }
    else {
      response = KMessageBox::warningContinueCancel( 0,
          i18n( "Are you sure you want to delete the task named"
          "\n\"%1\" and its entire history?\n"
          "NOTE: all its subtasks and their history will also "
          "be deleted.").arg(task->name()),
          i18n( "Deleting Task"), KStdGuiItem::del());
    }
  }

  if (response == KMessageBox::Continue)
  {
    if (markingascomplete)
    {
      task->setPercentComplete(100, _storage);
      task->setPixmapProgress();
      save();
      emit updateButtons();

      // Have to remove after saving, as the save routine only affects tasks
      // that are in the view.  Otherwise, the new percent complete does not
      // get saved.   (No longer remove when marked as complete.)
      //task->removeFromView();

    }
    else
    {
      QString uid=task->uid();
      task->remove(activeTasks, _storage);
      task->removeFromView();
      if( _preferences ) _preferences->deleteEntry( uid ); // forget if the item was expanded or collapsed
      save();
    }

    // remove root decoration if there is no more children.
    refresh();

    // Stop idle detection if no more counters are running
    if (activeTasks.count() == 0) {
      _idleTimeDetector->stopIdleDetection();
      emit timersInactive();
    }

    emit tasksChanged( activeTasks );
  }
}

void TaskView::extractTime(int minutes)
// This procedure subtracts ''minutes'' from the active task's time in the memory.
// It is called by the idletimedetector class.
// When the desktop has been idle for the past 20 minutes, the past 20 minutes have 
// already been added to the task's time in order for the time to be displayed correctly.
// That is why idletimedetector needs to subtract this time first.
{
  kdDebug(5970) << "Entering extractTime" << endl;
  addTimeToActiveTasks(-minutes,false); // subtract minutes, but do not store it
}

void TaskView::autoSaveChanged(bool on)
{
  if (on) _autoSaveTimer->start(_preferences->autoSavePeriod()*1000*secsPerMinute);
  else if (_autoSaveTimer->isActive()) _autoSaveTimer->stop();
}

void TaskView::autoSavePeriodChanged(int /*minutes*/)
{
  autoSaveChanged(_preferences->autoSave());
}

void TaskView::adaptColumns()
{
  // to hide a column X we set it's width to 0
  // at that moment we'll remember the original column within
  // previousColumnWidths[X]
  //
  // When unhiding a previously hidden column
  // (previousColumnWidths[X] != HIDDEN_COLUMN !)
  // we restore it's width from the saved value and set
  // previousColumnWidths[X] to HIDDEN_COLUMN

  for( int x=1; x <= 4; x++) {
    // the column was invisible before and were switching it on now
    if(   _preferences->displayColumn(x-1)
       && previousColumnWidths[x-1] != HIDDEN_COLUMN )
    {
      setColumnWidth( x, previousColumnWidths[x-1] );
      previousColumnWidths[x-1] = HIDDEN_COLUMN;
      setColumnWidthMode( x, QListView::Maximum );
    }
    // the column was visible before and were switching it off now
    else
    if( ! _preferences->displayColumn(x-1)
       && previousColumnWidths[x-1] == HIDDEN_COLUMN )
    {
      setColumnWidthMode( x, QListView::Manual ); // we don't want update()
                                                  // to resize/unhide the col
      previousColumnWidths[x-1] = columnWidth( x );
      setColumnWidth( x, 0 );
    }
  }
}

void TaskView::deletingTask(Task* deletedTask)
{
  DesktopList desktopList;

  _desktopTracker->registerForDesktops( deletedTask, desktopList );
  activeTasks.removeRef( deletedTask );

  emit tasksChanged( activeTasks);
}

void TaskView::iCalFileChanged(QString file)
// User might have picked a new file in the preferences dialog.
// This is not iCalFileModified.
{
  kdDebug(5970) << "TaskView:iCalFileChanged: " << file << endl;
  if (_storage->icalfile() != file)
  {
    stopAllTimers();
    _storage->save(this);
    load();
  }
}

QValueList<HistoryEvent> TaskView::getHistory(const QDate& from,
    const QDate& to) const
{
  return _storage->getHistory(from, to);
}

void TaskView::markTaskAsComplete()
{
  if (current_item())
    kdDebug(5970) << "TaskView::markTaskAsComplete: "
      << current_item()->uid() << endl;
  else
    kdDebug(5970) << "TaskView::markTaskAsComplete: null current_item()" << endl;

  bool markingascomplete = true;
  deleteTask(markingascomplete);
}

void TaskView::markTaskAsIncomplete()
{
  if (current_item())
    kdDebug(5970) << "TaskView::markTaskAsComplete: "
      << current_item()->uid() << endl;
  else
    kdDebug(5970) << "TaskView::markTaskAsComplete: null current_item()" << endl;

  reinstateTask(50); // if it has been reopened, assume half-done
}


void TaskView::clipTotals()
{
  TimeKard t;
  if (current_item() && current_item()->isRoot())
  {
    int response = KMessageBox::questionYesNo( 0,
        i18n("Copy totals for just this task and its subtasks, or copy totals for all tasks?"),
        i18n("Copy Totals to Clipboard"),
        i18n("Copy This Task"), i18n("Copy All Tasks") );
    if (response == KMessageBox::Yes) // This task only
    {
      KApplication::clipboard()->setText(t.totalsAsText(this, true, TimeKard::TotalTime));
    }
    else // All tasks
    {
      KApplication::clipboard()->setText(t.totalsAsText(this, false, TimeKard::TotalTime));
    }
  }
  else
  {
    KApplication::clipboard()->setText(t.totalsAsText(this, true, TimeKard::TotalTime));
  }
}

void TaskView::clipSession()
{
  TimeKard t;
  if (current_item() && current_item()->isRoot())
  {
    int response = KMessageBox::questionYesNo( 0,
        i18n("Copy session time for just this task and its subtasks, or copy session time for all tasks?"),
        i18n("Copy Session Time to Clipboard"),
        i18n("Copy This Task"), i18n("Copy All Tasks") );
    if (response == KMessageBox::Yes) // this task only
    {
      KApplication::clipboard()->setText(t.totalsAsText(this, true, TimeKard::SessionTime));
    }
    else // only task
    {
      KApplication::clipboard()->setText(t.totalsAsText(this, false, TimeKard::SessionTime));
    }
  }
  else
  {
    KApplication::clipboard()->setText(t.totalsAsText(this, true, TimeKard::SessionTime));
  }
}

void TaskView::clipHistory()
{
  PrintDialog dialog;
  if (dialog.exec()== QDialog::Accepted)
  {
    TimeKard t;
    KApplication::clipboard()->
      setText( t.historyAsText(this, dialog.from(), dialog.to(), !dialog.allTasks(), dialog.perWeek(), dialog.totalsOnly() ) );
  }
}

#include "taskview.moc"
