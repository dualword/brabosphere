/***************************************************************************
                          command.h  -  description
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
/// Contains the declaration of the class Command.

#ifndef COMMAND_H
#define COMMAND_H

///// Forward class declarations & header files ///////////////////////////////

// Qt includes
#include <qstring.h>

// Xbrabo forward class declarations
class Xbrabo;
class XbraboView;

// Xbrabo header files
#include "preferencesbase.h"

///// class Command ///////////////////////////////////////////////////////////
class Command
{
  public:
    ///// constructor/destructor
    Command(Xbrabo* parent, const QString description);    // constructor
    virtual ~Command();                 // Destructor
    virtual Command* clone() const = 0; // 'Virtual (copy) constructor'

    ///// enums
    enum Type{NewCalculation, OpenCalculation, Preferences};

    ///// public member functions
    QString description() const;        // Returns a description of the command.
    virtual bool execute(bool silent = false) = 0;// Executes the command
    virtual bool revert() = 0;          // Reverts the effects of executing the command.
    virtual Type type() const = 0;      // Returns the type of command
    bool isRepeatable() const;          // Returns whether the command can be repeated

  protected:
    ///// protected member variables
    Xbrabo* mainWindow;                 // Contains a pointer to the Xbrabo class where the commands are executed.
    bool repeatable;                    // Contains the Repeatable property

  private:
    ///// private member variables
    QString desc;                       // Contains the command's description
};

///// class CommandNewCalculation /////////////////////////////////////////////
class CommandNewCalculation : public Command
{
  public:
    ///// constructor/destructor
    CommandNewCalculation(Xbrabo* parent, const QString description); // constructor
    virtual CommandNewCalculation* clone() const; // virtual copy constructor

    ///// public member functions
    bool execute(bool silent = false);  // Executes the command
    bool revert();                      // Reverts the effects of executing the command.
    Type type() const;                  // Returns the type of command

  private:
    ///// private member variables
    XbraboView* view;                   // A pointer to the created calculation
};

///// class CommandOpenCalculation ////////////////////////////////////////////
class CommandOpenCalculation : public Command
{
  public:
    ///// constructor/destructor
    CommandOpenCalculation(Xbrabo* parent, const QString description, const QString filename);      // constructor
    virtual CommandOpenCalculation* clone() const;// virtual copy constructor

    ///// public member functions
    bool execute(bool silent = false);  // Executes the command
    bool revert();                      // Reverts the effects of executing the command.
    Type type() const;                  // Returns the type of command

  private:
    ///// private member variables
    XbraboView* view;                   // A pointer to the created calculation
    QString fileName;                   // The name of the opened file
};

///// class CommandPreferences ////////////////////////////////////////////////
class CommandPreferences : public Command
{
  public:
    ///// constructor/destructor
    CommandPreferences(Xbrabo* parent, const QString description);    // constructor
    virtual CommandPreferences* clone() const;    // virtual copy constructor

    ///// public member functions
    bool execute(bool silent = false);  // Executes the command
    bool revert();                      // Reverts the effects of executing the command.
    Type type() const;                  // Returns the type of command

  private:
    ///// private member variables
    PreferencesBase::WidgetData oldData, newData;   // PreferencesBase structs containing all information before and after
    bool oldPvmHostsChanged, newPvmHostsChanged;    // Keeps track of any changes to the PVM host list.
};

#endif

