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

///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandHistory::CommandHistory(QObject* parent, const char* name) : QObject(parent, name),
  currentPosition(commandList.end()),
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
  enforceSize(); // not called when setting a new maximum size, so overriding the maximum size 
                 // will only be fixed after adding a new command

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

///// setMaxLevels ////////////////////////////////////////////////////////////
void CommandHistory::setMaxLevels(const int levels)
/// Sets the maximum allowed number of entries in the history. Setting it to zero
/// disables the history and a negative value sets is to unlimited.
{
  maxLevels = levels;
  if(maxLevels > 0)
    maxRAM = -1;
  else if(maxLevels == 0)
    maxRAM = 0;
}

///// setMaxRAM ///////////////////////////////////////////////////////////////
void CommandHistory::setMaxRAM(const int mb)
/// Sets the maximum memory size of the history to the given number of megabytes.
/// Setting it to zero disables the history and a negative value sets is to
/// unlimited.
{
  maxRAM = mb;
  if(maxRAM > 0)
    maxLevels = -1;
  else if(maxRAM == 0)
    maxLevels = 0;
}

///// pruneCoordinates ////////////////////////////////////////////////////////
void CommandHistory::pruneCoordinates()
/// Removes all Commands that alter the coordinates. To be used when the 
/// coordinates are updated from a calculation.
{
  if(commandList.empty())
    return;

  ///// check whether the current redo action has to be deleted. In that case all
  ///// following redo's have to be deleted.
  if(redoAvailable() && dynamic_cast<CommandCoordinates*>(*currentPosition) != NULL)
  {
    commandList.erase(currentPosition, commandList.end());
    currentPosition = 0;
  }

  ///// remove all other instances of coordinate altering commands
  std::list<Command*>::iterator it = commandList.end();
  --it;
  while(it != NULL)
  {
    if(dynamic_cast<CommandCoordinates*>(*it) != NULL)
    {
      std::list<Command*>::iterator it2 = it--;
      delete *it2;
      commandList.erase(it2);
    }
    else
      --it;
  }

  ///// position again if the current redo command was deleted
  if(currentPosition == 0)
    currentPosition = commandList.end();
}

///////////////////////////////////////////////////////////////////////////////
///// Private Member Functions                                            /////
///////////////////////////////////////////////////////////////////////////////

///// enforceSize /////////////////////////////////////////////////////////////
void CommandHistory::enforceSize()
/// Truncates the size of the history so it's not larger than the set maximum 
/// sizes.
{
  // possible cases: - unlimited:      maxLevels < 0, maxRAM < 0
  //                 - limited levels: maxLevels > 0, maxRAM < 0
  //                 - limited RAM:    maxLevels < 0, maxRAM > 0
  //                 - disabled:       maxLevels = 0, maxRAM = 0
  if(maxLevels < 0 && maxRAM < 0)
    return; // unlimited size

  ///// limit the maximum number of levels
  if(maxLevels >= 0)
  {
    while(commandList.size() > maxLevels)
    {
      delete commandList.front();
      commandList.pop_front();
    }
  }
  if(maxRAM > 0) // if maxRAM == 0, maxLevels should be zero too and the list will have been cleared above
  {
    // get the total size of the history
    unsigned int totalSize = 0;
    for(std::list<Command*>::iterator it = commandList.begin(); it != commandList.end(); it++)
      totalSize += (*it)->ramSize();
    // limit it
    while(totalSize > maxRAM*1024*1024) // totalSize is in bytes, maxRAM in megabytes
    {
      totalSize -= commandList.front()->ramSize();
      delete commandList.front();
      commandList.pop_front();
    }
  } 
}


///////////////////////////////////////////////////////////////////////////////
///// Static variables                                                    /////
///////////////////////////////////////////////////////////////////////////////

int CommandHistory::maxLevels = 100;
int CommandHistory::maxRAM = -1;
