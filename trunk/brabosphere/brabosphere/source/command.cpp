/***************************************************************************
                         command.cpp  -  description
                             -------------------
    begin                : Thu May 25 2006
    copyright            : (C) 2006 by Ben Swerts
    email                : bswerts@users.sourceforge.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

///// Comments ////////////////////////////////////////////////////////////////
/*!
  \class Command
  \brief This class implements the Command design pattern for use as an
         Undo/Redo stack.

  The Command class itself is an abstract base class for the subclasses also 
  present in this file. The type of the command is given by the Type enum.
*/
/// \file
/// Contains the implementation of the class Command.

///// Header files ////////////////////////////////////////////////////////////

// Qt header files
#include <qstatusbar.h>

// Xbrabo header files
#include "command.h"
#include "xbrabo.h"
#include "xbraboview.h"

///////////////////////////////////////////////////////////////////////////////
///// Class Command                                                       /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
Command::Command(Xbrabo* parent, const QString description) :
  mainWindow(parent),
  repeatable(false),
  desc(description)
/// The default constructor. 
{

}

///// destructor //////////////////////////////////////////////////////////////
Command::~Command()
/// The default destructor.
{

}

///// description /////////////////////////////////////////////////////////////
QString Command::description() const
/// Returns the description of the command
{
  return desc;
}

///// isRepeatable ////////////////////////////////////////////////////////////
bool Command::isRepeatable() const
/// Returns whether the command can be executed repeatedly. The default value is false.
{
  return repeatable;
}


///////////////////////////////////////////////////////////////////////////////
///// Class CommandNewCalculation                                         /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandNewCalculation::CommandNewCalculation(Xbrabo* parent, const QString description) : Command(parent, description)
/// The default constructor. 
{
  repeatable = true;
}

///// copy constructor ////////////////////////////////////////////////////////
CommandNewCalculation* CommandNewCalculation::clone() const
/// The copy constructor using the 'virtual constructor idiom'
{
  return new CommandNewCalculation(*this);
}

///// execute /////////////////////////////////////////////////////////////////
bool CommandNewCalculation::execute(bool silent)
/// Creates a new calculation. This is always a silent operation.
{
  view = mainWindow->createCalculation();
  return true;
}

///// revert //////////////////////////////////////////////////////////////////
bool CommandNewCalculation::revert()
/// Reverts the creation of a new calculation, which means it closes it.
{
  return mainWindow->closeCalculation(view);
}

///// type ////////////////////////////////////////////////////////////////////
Command::Type CommandNewCalculation::type() const
/// Returns the type of command, in this case NewCalculation.
{
  return NewCalculation;
}

///////////////////////////////////////////////////////////////////////////////
///// Class CommandOpenCalculation                                        /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandOpenCalculation::CommandOpenCalculation(Xbrabo* parent, const QString description, const QString filename) : Command(parent, description),
  view(0),
  fileName(filename)
/// The default constructor. 
{

}

///// copy constructor ////////////////////////////////////////////////////////
CommandOpenCalculation* CommandOpenCalculation::clone() const
/// The copy constructor using the 'virtual constructor idiom'
{
  return new CommandOpenCalculation(*this);
}

///// execute /////////////////////////////////////////////////////////////////
bool CommandOpenCalculation::execute(bool silent)
/// Opens an existing calculation. This is always a silent operation because the 
/// filename is already given and never empty.
{
  view = mainWindow->openCalculation(fileName);
  return view != 0;
}

///// revert //////////////////////////////////////////////////////////////////
bool CommandOpenCalculation::revert()
/// Reverts the opening of an existing calculation, which means it closes it.
{
  return mainWindow->closeCalculation(view);
}

///// type ////////////////////////////////////////////////////////////////////
Command::Type CommandOpenCalculation::type() const
/// Returns the type of command, in this case OpenCalculation.
{
  return OpenCalculation;
}


///////////////////////////////////////////////////////////////////////////////
///// Class CommandPreferences                                            /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandPreferences::CommandPreferences(Xbrabo* parent, const QString description) : Command(parent, description)
/// The default constructor. 
{

}

///// copy constructor ////////////////////////////////////////////////////////
CommandPreferences* CommandPreferences::clone() const
/// The copy constructor using the 'virtual constructor idiom'
{
  return new CommandPreferences(*this);
}

///// execute /////////////////////////////////////////////////////////////////
bool CommandPreferences::execute(bool silent)
/// Changes the program's preferences and keeps a copy of the previous state.
/// In silent mode, the Preferences dialog is not shown, but data are restored 
/// from a previous run of execute()
{
  if(silent)
  {
    // re-apply new vales
    mainWindow->editPreferences->data = newData;
    mainWindow->editPreferences->pvmHostsChanged = newPvmHostsChanged;
    mainWindow->editPreferences->widgetChanged = true; // so applyChanges takes effect
    mainWindow->editPreferences->applyChanges();   
    return true;
  }
  else
  {
    // backup old data
    oldData = mainWindow->editPreferences->data;
    oldPvmHostsChanged = mainWindow->editPreferences->pvmHostsChanged;
    // show the dialog
    return mainWindow->changePreferences();
  }
}

///// revert //////////////////////////////////////////////////////////////////
bool CommandPreferences::revert()
/// Reverts any changes made in the Preferences dialog.
{
  // backup new data
  newData = mainWindow->editPreferences->data;
  newPvmHostsChanged = mainWindow->editPreferences->pvmHostsChanged; 
  // revert to old data
  mainWindow->editPreferences->data = oldData;
  mainWindow->editPreferences->pvmHostsChanged = oldPvmHostsChanged;
  mainWindow->editPreferences->widgetChanged = true; // so applyChanges takes effect
  mainWindow->editPreferences->applyChanges();
  return true;
}

///// type ////////////////////////////////////////////////////////////////////
Command::Type CommandPreferences::type() const
/// Returns the type of command, in this case Preferences.
{
  return Preferences;
}
