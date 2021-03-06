/*
    This file is part of kdepim.

    Copyright (c) 2005 Will Stephenson <wstephenson@suse.de>

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

#ifndef GROUPWISE_SETTINGS_WIDGET_H
#define GROUPWISE_SETTINGS_WIDGET_H

#include <qmap.h>

class QListViewItem;

#include "groupwisesettingswidgetbase.h"

class GroupWiseSettingsWidget : public GroupWiseSettingsWidgetBase
{
Q_OBJECT
public:
  GroupWiseSettingsWidget( QWidget * parent );
  QMap<QString, QString> dirtySettings();
  void reset();
protected slots:
  /**
   * Add the renamed item to the list of dirty (changed) settings
   */
  void slotItemRenamed( QListViewItem *Item, int ColumnRenamed );
private: 
  QMap<QString, QString> m_dirtySettings;
};

#endif
