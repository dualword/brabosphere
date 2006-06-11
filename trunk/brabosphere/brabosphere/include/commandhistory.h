/***************************************************************************
                       commandhistory.h  -  description
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

/// \file
/// Contains the declaration of the class CommandHistory.

#ifndef COMMANDHISTORY_H
#define COMMANDHISTORY_H

///// Forward class declarations & header files ///////////////////////////////

// STL includes
#include <list>

// Qt includes
#include <qobject.h>

// Xbrabo forward class declarations
class Command;

///// class CommandHistory ////////////////////////////////////////////////////

class CommandHistory: public QObject
{
  Q_OBJECT

  public:
    ///// constructor/destructor
    CommandHistory(QObject* parent = 0, const char* name = 0);        // constructor
    ~CommandHistory();                        // destructor

    ///// public member functions
    void addCommand(Command* command);  // adds and executes a new command
    void undo();                        // reverts the current command
    void redo();                        // redoes the current command
    void repeat();                      // repeats the current command
    bool undoAvailable() const;         // returns whether an undo action is possible
    bool redoAvailable() const;         // returns whether a redo action is possible
    bool repeatAvailable() const;       // returns whether a repeat action is possible
    QString undoText() const;           // Returns the description if the current command if it can be reverted
    QString redoText() const;           // Returns the description if the previous command if it can be re-executed
    QString repeatText() const;         // Returns the description if the current command if it can be repeated

    void setMaxSize(const unsigned int size);     // Sets the maximum number of entries in the history
    void pruneCoordinates();            // removes all coordinate altering entries which conflict with a started calculation

  signals:
    void changed();                     ///< Is emitted when a command was added or an undo/redo/repeat action was performed

  private:
    // private member functions
    void enforceSize();                 // Truncates the size of the history if necessary.

    // private member variables
    std::list<Command*> commandList;    ///< A list of all added Command's
    std::list<Command*>::iterator currentPosition;///< Holds the position of the current Command
    unsigned int maxSize;               ///< The maximum number of Command's the history should hold
    bool lastActionAdded;               ///< Keeps track of whether the last action was an addCommand call.
};

#endif

