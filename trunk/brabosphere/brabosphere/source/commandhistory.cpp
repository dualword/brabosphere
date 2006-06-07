/***************************************************************************
                      commandhistory.cpp  -  description
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
  \class CommandHistory
  \brief This class implements the Command design pattern for use as an
         Undo/Redo stack.

  It holds a list of all Commands that have been run so they can be undone/redone.
  The 'currentPosition' pointer holds the current situation as its name suggest. 
  When its equal to list.end(), the last entry of the list was executed and no 
  'redo' is available. If it's equal to list.begin(), no 'undo' is available.
*/
/// \file
/// Contains the implementation of the class CommandHistory.

///// Header files ////////////////////////////////////////////////////////////

// C++ header files
#include <cassert>

// Qt header files
#include <qapplication.h>

// Xbrabo header files
#include "command.h"
#include "commandhistory.h"
#include "xbrabo.h"

///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandHistory::CommandHistory(QObject* parent, const char* name) : QObject(parent, name),
  currentPosition(commandList.end()),
  maxSize(100),
  lastActionAdded(false)
/// The default constructor.
{

}

///// destructor //////////////////////////////////////////////////////////////
CommandHistory::~CommandHistory()
/// The default destructor.
{

}

///// addCommand //////////////////////////////////////////////////////////////
void CommandHistory::addCommand(Command* command)
/// Adds and executes a new command.
{
  assert(command != 0);

  // execute the Command
  if(!command->execute())
  {
    delete command;
    return; // Only add it to the history if it succeeded
  }

  // If the previous command was not the last, make it the last
  //if(currentPosition != commandList.end()) // should work without this check
  commandList.erase(currentPosition, commandList.end());

  // Attempt to combine it with the previous command
  if(!commandList.empty())
  {
    std::list<Command*>::iterator it = commandList.end();
    --it; // points to the active command
    qDebug("current command to combine the new one with: %X / " + (*it)->description(), *it);
    if((*it)->combine(command))
    {
      // the 2 Commands were succesfully combined
      delete command;
      return;
    }
  }
  // Add the new Command
  commandList.push_back(command);
  lastActionAdded = true;

  // Enforce the maximum size
  enforceSize();

  // Reposition
  currentPosition = commandList.end();

  // Notify that the list has changed
  emit changed();
}

///// undo ////////////////////////////////////////////////////////////////////
void CommandHistory::undo()
/// Reverts the current command
{
  if(!undoAvailable())
    return;
  lastActionAdded = false;

  (*(--currentPosition))->revert();
  emit changed();
}

///// redo ////////////////////////////////////////////////////////////////////
void CommandHistory::redo()
/// Executes the current command again.
{
  if(!redoAvailable())
    return;
  lastActionAdded = false;

  (*currentPosition++)->execute(true); // run in silent mode which re-applies the new state
                                       // instead of creating one.
  emit changed();
}

///// repeat //////////////////////////////////////////////////////////////////
void CommandHistory::repeat()
/// Repeats Execution of the current command.
{
  if(!repeatAvailable())
    return;

  std::list<Command*>::iterator it = currentPosition;
  addCommand((*(--it))->clone()); // duplicate the desired command and add it to the list
}

///// undoAvailable ///////////////////////////////////////////////////////////
bool CommandHistory::undoAvailable() const
/// Returns whether a command can be reverted.
{
  return currentPosition != commandList.begin();
}

///// redoAvailable ///////////////////////////////////////////////////////////
bool CommandHistory::redoAvailable() const
/// Returns whether a command can be executed.
{
  return currentPosition != commandList.end();
}

///// repeatAvailable /////////////////////////////////////////////////////////
bool CommandHistory::repeatAvailable() const
/// Returns whether the current command can be repeated.
{
  std::list<Command*>::iterator it = currentPosition;
  return lastActionAdded && currentPosition != commandList.begin() && (*(--it))->isRepeatable();
}

///// undoText ////////////////////////////////////////////////////////////////
QString CommandHistory::undoText() const
/// Returns the description of the command for the undo action.
{
  if(undoAvailable())
  {
    std::list<Command*>::iterator it = currentPosition;
    return "'" + (*(--it))->description() + "'";
  }
  else
    return QString::null;
}

///// redoText ////////////////////////////////////////////////////////////////
QString CommandHistory::redoText() const
/// Returns the description of the command for the redo action.
{
  if(redoAvailable())
    return "'" + (*currentPosition)->description() + "'";
  else
    return QString::null;
}

///// repeatText //////////////////////////////////////////////////////////////
QString CommandHistory::repeatText() const
/// Returns the description of the command for the repeat action.
{
  if(repeatAvailable())
  {
    std::list<Command*>::iterator it = currentPosition;
    return "'" + (*(--it))->description() + "'";
  }
  else
    return QString::null;
}

///// setMaxSize //////////////////////////////////////////////////////////////
void CommandHistory::setMaxSize(const unsigned int size)
/// Sets the maximum allowed number of entries in the history.
{
  maxSize = size;
  if(maxSize < 1)
    maxSize = 1;

  enforceSize();
}

///////////////////////////////////////////////////////////////////////////////
///// Private Member Functions                                            /////
///////////////////////////////////////////////////////////////////////////////

///// enforceSize /////////////////////////////////////////////////////////////
void CommandHistory::enforceSize()
/// Truncates the size of the history so it's not larger than the set maximum size.
{
  while(commandList.size() > maxSize)
  {
    delete commandList.front();
    commandList.pop_front();
  }
}