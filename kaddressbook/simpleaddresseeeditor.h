/*
    This file is part of KAddressBook.

    Copyright (c) 2003 Cornelius Schumacher <schumacher@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#ifndef SIMPLEADDRESSEEEDITOR_H
#define SIMPLEADDRESSEEEDITOR_H

#include "addresseeeditorwidget.h"

class KLineEdit;

class SimpleAddresseeEditor : public AddresseeEditorBase
{
    Q_OBJECT
  public:
    SimpleAddresseeEditor( QWidget *parent, const char *name = 0 );
    ~SimpleAddresseeEditor();

    void setAddressee( const KABC::Addressee& );
    const KABC::Addressee &addressee();

    void load();
    void save();

    bool dirty();

    void setInitialFocus();

  protected slots:
    void emitModified();

  protected:
    void initGui();

  private:
    KLineEdit *mNameEdit;
    KLineEdit *mEmailEdit;

    KABC::Addressee mAddressee;

    bool mDirty;
    bool mBlockModified;
};

#endif
