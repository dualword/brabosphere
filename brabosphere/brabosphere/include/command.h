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
/// Contains the declaration of the class Command and its subclasses.

#ifndef COMMAND_H
#define COMMAND_H

///// Forward class declarations & header files ///////////////////////////////

// STL header files
#include <list>

// Qt header files
#include <qstring.h>

// Xbrabo forward class declarations
class AtomSet;
class NewAtomBase;
class XbraboView;

// Xbrabo header files
#include "brabobase.h"
#include "globalbase.h"
#include "quaternion.h"
#include "relaxbase.h"

///// class Command ///////////////////////////////////////////////////////////
class Command
{
  public:
    ///// constructor/destructor
    Command(XbraboView* parent, const QString description);    // constructor
    virtual ~Command();                 // Destructor
    virtual Command* clone() const = 0; // 'Virtual (copy) constructor'

    ///// public member functions
    QString description() const;        // Returns a description of the command.
    virtual bool execute(bool fromBackup = false) = 0;      // Executes the command
    virtual bool revert() = 0;          // Reverts the effects of executing the command.
    virtual bool combine(Command* command);       // Combines the command with another one
    virtual unsigned int ramSize() const;         // Returns the RAM size needed in bytes
    bool isRepeatable() const;          // Returns whether the command can be repeated

  protected:
    ///// protected member variables
    XbraboView* view;                   // Contains a pointer to the XbraboView class where the commands are executed.
    bool repeatable;                    // Contains the Repeatable property

  private:
    ///// private member variables
    QString desc;                       // Contains the command's description
};

/*
///// class CommandNewCalculation /////////////////////////////////////////////
class CommandNewCalculation : public Command
{
  public:
    ///// constructor/destructor
    CommandNewCalculation(Xbrabo* parent, const QString description); // constructor
    virtual CommandNewCalculation* clone() const; // virtual copy constructor

    ///// public member functions
    bool execute(bool fromBackup = false);        // Executes the command
    bool revert();                      // Reverts the effects of executing the command.

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
    bool execute(bool fromBackup = false);        // Executes the command
    bool revert();                      // Reverts the effects of executing the command.

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
    bool execute(bool fromBackup = false);        // Executes the command
    bool revert();                      // Reverts the effects of executing the command.

  private:
    ///// private member variables
    PreferencesBase::WidgetData oldData, newData;   // PreferencesBase structs containing all information before and after
};

///// class CommandDockWindow /////////////////////////////////////////////////
class CommandDockWindow : public Command
{
  public:
    ///// constructor/destructor
    CommandDockWindow(Xbrabo* parent, const QString description, QDockWindow* dock);      // constructor
    virtual CommandDockWindow* clone() const;     // virtual copy constructor

    ///// public member functions
    bool execute(bool fromBackup = false);        // Executes the command
    bool revert();                      // Reverts the effects of executing the command.    

  private:
    ///// private member variables
    QDockWindow* dockWindow;            ///< A pointer to the actual QDockWindow of which the visibility is to be tracked
};

///// class CommandStatusBar //////////////////////////////////////////////////
class CommandStatusBar : public Command
{
  public:
    ///// constructor/destructor
    CommandStatusBar(Xbrabo* parent, const QString description);      // constructor
    virtual CommandStatusBar* clone() const;      // virtual copy constructor

    ///// public member functions
    bool execute(bool fromBackup = false);        // Executes the command
    bool revert();                      // Reverts the effects of executing the command.    
};
*/

///// class CommandCoordinates ////////////////////////////////////////////////
class CommandCoordinates : public Command
{
  public:
    ///// constructor/destructor
    CommandCoordinates(XbraboView* parent, const QString description);  // constructor
    virtual ~CommandCoordinates();  // destructor
    
    ///// public member functions
    bool execute(bool fromBackup = false);        // Executes the command
    bool revert();                      // Reverts the effects of executing the command.    
    virtual bool combine(Command* command);       // Combines the command with another one.
    virtual unsigned int ramSize() const;         // Returns the needed RAM size in bytes
    virtual bool initialRun() = 0;      // Gets called in execute when fromBackup == false by subclasses

  protected:
    ///// protected member variables
    AtomSet* oldAtoms, * newAtoms;      ///< Backups of the coordinates
    std::list<unsigned int> oldSelectionList, newSelectionList;       ///< List that holds the selected atoms and their order for the old and new set of atoms.
};

///// class CommandReadCoordinates ////////////////////////////////////////////
class CommandReadCoordinates : public CommandCoordinates
{
  public:
    ///// constructor/destructor
    CommandReadCoordinates(XbraboView* parent, const QString description);  // constructor
    virtual CommandReadCoordinates* clone() const;// virtual copy constructor

    ///// public member functions
    bool execute(bool fromBackup = false);        // Executes the command
    bool revert();                      // Reverts the effects of executing the command.    
    virtual bool initialRun();          // Duplicate in this subclass

  private:
    ///// private member variables
    float oldX, oldY, oldZ;             // values of the translation vector as they are reset when reading new coordinates
    Quaternion<float> oldRotation;      // value of the rotation quaternion (also reset)
};

///// class CommandAddAtoms ///////////////////////////////////////////////////
class CommandAddAtoms : public CommandCoordinates
{
  public:
    ///// constructor/destructor
    CommandAddAtoms(XbraboView* parent, const QString description, NewAtomBase* newAtomDialog);     // constructor
    virtual CommandAddAtoms* clone() const;        // virtual copy constructor

    ///// public member functions
    virtual bool initialRun();          // Adds atoms to the molecular system

  private:
    ///// private member variables
    NewAtomBase* newAtomBase;           ///< The dialog providing an interface to adding new atoms.
};

///// class CommandDeleteAtoms ////////////////////////////////////////////////
class CommandDeleteAtoms : public CommandCoordinates
{
  public:
    ///// constructor/destructor
    CommandDeleteAtoms(XbraboView* parent, const QString description);     // constructor
    virtual CommandDeleteAtoms* clone() const;    // virtual copy constructor

    ///// public member functions
    virtual bool initialRun();          // Deletes selected atoms from the molecular system
};

///// class CommandAlterCartesian /////////////////////////////////////////////
class CommandAlterCartesian : public CommandCoordinates
{
  public:
    ///// constructor/destructor
    CommandAlterCartesian(XbraboView* parent, const QString description);       // constructor
    virtual CommandAlterCartesian* clone() const; // virtual copy constructor

    ///// public member functions
    virtual bool initialRun();          // Changes the cartesian coordinates of the selected atoms
    virtual bool combine(Command* command);       // combines with another command
};

///// class CommandAlterInternal //////////////////////////////////////////////
class CommandAlterInternal : public CommandCoordinates
{
  public:
    ///// constructor/destructor
    CommandAlterInternal(XbraboView* parent, const QString description);        // constructor
    virtual CommandAlterInternal* clone() const;  // virtual copy constructor

    ///// public member functions
    virtual bool initialRun();          // Changes the internal coordinate of the selection.
    virtual bool combine(Command* command);       // combines with another command
};

///// class CommandTranslateSelectionXY ///////////////////////////////////////
class CommandTranslateSelectionXY : public CommandCoordinates
{
  public:
    ///// constructor/destructor
    CommandTranslateSelectionXY(XbraboView* parent, const QString description, const int amountX, const int amountY);   // constructor
    virtual CommandTranslateSelectionXY* clone() const;  // virtual copy constructor

    ///// public member functions
    virtual bool initialRun();          // Changes the internal coordinate of the selection.
    virtual bool combine(Command* command);       // combines with another command

  private:
    ///// private member variables
    int incX, incY;                     // hold copies of the constructor's arguments
};

///// class CommandTranslateSelectionZ ////////////////////////////////////////
class CommandTranslateSelectionZ : public CommandCoordinates
{
  public:
    ///// constructor/destructor
    CommandTranslateSelectionZ(XbraboView* parent, const QString description, const int amountZ);   // constructor
    virtual CommandTranslateSelectionZ* clone() const;      // virtual copy constructor

    ///// public member functions
    virtual bool initialRun();          // Changes the internal coordinate of the selection.
    virtual bool combine(Command* command);       // combines with another command

  private:
    ///// private member variables
    int incZ;                           // holds a copy of the constructor's argument
};

///// class CommandRotateSelection ////////////////////////////////////////////
class CommandRotateSelection : public CommandCoordinates
{
  public:
    ///// constructor/destructor
    CommandRotateSelection(XbraboView* parent, const QString description, const double amountX, const double amountY, const double amountZ);   // constructor
    virtual CommandRotateSelection* clone() const;// virtual copy constructor

    ///// public member functions
    virtual bool initialRun();          // Changes the internal coordinate of the selection.
    virtual bool combine(Command* command);       // combines with another command

  private:
    ///// private member variables
    double incX, incY, incZ;            // hold copies of the constructor's arguments
};

///// class CommandChangeIC ///////////////////////////////////////////////////
class CommandChangeIC : public CommandCoordinates
{
  public:
    ///// constructor/destructor
    CommandChangeIC(XbraboView* parent, const QString description, const int range);      // constructor
    virtual CommandChangeIC* clone() const;       // virtual copy constructor

    ///// public member functions
    virtual bool initialRun();          // Changes the internal coordinate of the selection.
    virtual bool combine(Command* command);       // combines with another command

  private:
    ///// private member variables
    int amount;                         // holds a copy of the constructor's argument
};

///// class CommandSelection //////////////////////////////////////////////////
class CommandSelection : public Command
{
  public:
    ///// constructor/destructor
    CommandSelection(XbraboView* parent, const QString description);  // constructor
    
    ///// public member functions
    bool execute(bool fromBackup = false);        // Executes the command
    bool revert();                      // Reverts the effects of executing the command.    
    virtual unsigned int ramSize() const;         // Returns the needed RAM size in bytes
    virtual bool initialRun() = 0;      // Gets called in execute when fromBackup == false by subclasses

  private:
    ///// private member variables
    std::list<unsigned int> oldSelectionList, newSelectionList;       ///< List that holds the selected atoms and their order for the old and new set of atoms.
};

///// class CommandSelectAll //////////////////////////////////////////////////
class CommandSelectAll : public CommandSelection
{
  public:
    ///// constructor/destructor
    CommandSelectAll(XbraboView* parent, const QString description);  // constructor
    virtual CommandSelectAll* clone() const;  // virtual copy constructor

    ///// public member functions
    virtual bool initialRun();          // Selects all atoms
};

///// class CommandSelectNone //////////////////////////////////////////////////
class CommandSelectNone : public CommandSelection
{
  public:
    ///// constructor/destructor
    CommandSelectNone(XbraboView* parent, const QString description); // constructor
    virtual CommandSelectNone* clone() const;  // virtual copy constructor

    ///// public member functions
    virtual bool initialRun();          // Selects all atoms
};

///// class CommandSelectEntity ///////////////////////////////////////////////
class CommandSelectEntity : public CommandSelection
{
  public:
    ///// constructor/destructor
    CommandSelectEntity(XbraboView* parent, const QString description, const unsigned int id); // constructor
    virtual CommandSelectEntity* clone() const;  // virtual copy constructor

    ///// public member functions
    virtual bool initialRun();          // Selects the entity with the given ID

  private:
    unsigned int glID;                  // The selection ID as produced by a call to GLSimpleMoleculeView::selectEntity. Needed for the call to initialRun.
};

///// class CommandDisplayMode ////////////////////////////////////////////////
class CommandDisplayMode : public Command
{
  public:
    ///// constructor/destructor
    CommandDisplayMode(XbraboView* parent, const QString description); // constructor
    virtual CommandDisplayMode* clone() const;  // virtual copy constructor

    ///// public member functions
    bool execute(bool fromBackup = false);        // Executes the command
    bool revert();                      // Reverts the effects of executing the command.    

  private:
    ///// private member variables
    unsigned int oldStyleMolecule, newStyleMolecule;        // Holds the display style of the molecule
    unsigned int oldStyleForces, newStyleForces;            // Holds the display style of the forces
    bool oldShowElements, newShowElements;        // Holds whether elements are shown
    bool oldShowNumbers, newShowNumbers;          // Holds whether atomic numbers are shown
    unsigned int oldChargeType, newChargeType;    // Holds whether no/Mulliken/stockholder charges are shown
};

///// class CommandTranslation ////////////////////////////////////////////////
class CommandTranslation : public Command
{
  public:
    ///// constructor/destructor
    CommandTranslation(XbraboView* parent, const QString description);// constructor
    
    ///// public member functions
    bool execute(bool fromBackup = false);        // Executes the command
    bool revert();                      // Reverts the effects of executing the command.    
    virtual bool initialRun() = 0;      // Gets called in execute when fromBackup == false by subclasses

  private:
    ///// private member variables
    float oldX, newX, oldY, newY;       // values of the translation vector
};

///// class CommandTranslateXY ////////////////////////////////////////////////
class CommandTranslateXY : public CommandTranslation
{
  public:
    ///// constructor/destructor
    CommandTranslateXY(XbraboView* parent, const QString description, const int amountX, const int amountY);  // constructor
    virtual CommandTranslateXY* clone() const;    // virtual copy constructor

    ///// public member functions
    bool initialRun();                  // Translates the scene
    bool combine(Command* command);     // combines with another translation
  
  private:
    ///// private member variables
    int incX, incY;                       // amount of translation from input
};

///// class CommandCenterView /////////////////////////////////////////////////
class CommandCenterView : public CommandTranslation
{
  public:
    ///// constructor/destructor
    CommandCenterView(XbraboView* parent, const QString description); // constructor
    virtual CommandCenterView* clone() const;     // virtual copy constructor

    ///// public member functions
    bool initialRun();                  // Centers the view
};

///// class CommandZoom ///////////////////////////////////////////////////////
class CommandZoom : public Command
{
  public:
    ///// constructor/destructor
    CommandZoom(XbraboView* parent, const QString description);// constructor
    
    ///// public member functions
    bool execute(bool fromBackup = false);        // Executes the command
    bool revert();                      // Reverts the effects of executing the command.    
    virtual bool initialRun() = 0;      // Gets called in execute when fromBackup == false by subclasses

  private:
    ///// private member variables
    float oldZ, newZ;                   // values of the translation vector
};

///// class CommandTranslateZ /////////////////////////////////////////////////
class CommandTranslateZ : public CommandZoom
{
  public:
    ///// constructor/destructor
    CommandTranslateZ(XbraboView* parent, const QString description, const int amount);   // constructor
    virtual CommandTranslateZ* clone() const;    // virtual copy constructor

    ///// public member functions
    bool initialRun();                  // Executes the command 
    bool combine(Command* command);     // combines with another translation

  private:
    ///// private member variables
    int incZ;                           // amount of translation from input
};

///// class CommandZoomFit ////////////////////////////////////////////////////
class CommandZoomFit : public CommandZoom
{
  public:
    ///// constructor/destructor
    CommandZoomFit(XbraboView* parent, const QString description);    // constructor
    virtual CommandZoomFit* clone() const;    // virtual copy constructor

    ///// public member functions
    bool initialRun();                  // Executes the command 
};

///// class CommandRotation ///////////////////////////////////////////////////
class CommandRotation : public Command
{
  public:
    ///// constructor/destructor
    CommandRotation(XbraboView* parent, const QString description);// constructor
    
    ///// public member functions
    bool execute(bool fromBackup = false);        // Executes the command
    bool revert();                      // Reverts the effects of executing the command.    
    virtual bool initialRun() = 0;      // Gets called in execute when fromBackup == false by subclasses

  private:
    ///// private member variables
    Quaternion<float> oldRotation, newRotation;   // value of the rotation quaternion
};

///// class CommandRotate /////////////////////////////////////////////////////
class CommandRotate : public CommandRotation
{
  public:
    ///// constructor/destructor
    CommandRotate(XbraboView* parent, const QString description, const float amountX, const float amountY, const float amountZ);   // constructor
    virtual CommandRotate* clone() const;    // virtual copy constructor

    ///// public member functions
    bool initialRun();                  // Executes the command
    bool combine(Command* command);     // combines with another rotation

  private:
    ///// private member variables
    float incX, incY, incZ;              // amount of rotation from input
};

///// class CommandResetOrientation ///////////////////////////////////////////
class CommandResetOrientation : public CommandRotation
{
  public:
    ///// constructor/destructor
    CommandResetOrientation(XbraboView* parent, const QString description);   // constructor
    virtual CommandResetOrientation* clone() const;    // virtual copy constructor

    ///// public member functions
    bool initialRun();                  // Executes the command
};

///// class CommandResetView //////////////////////////////////////////////////
class CommandResetView : public Command
{
  public:
    ///// constructor/destructor
    CommandResetView(XbraboView* parent, const QString description);  // constructor
    virtual CommandResetView* clone() const;      // virtual copy constructor

    ///// public member functions
    bool execute(bool fromBackup = false);        // Executes the command
    bool revert();                      // Reverts the effects of executing the command.    

  private:
    ///// private member variables
    float oldX, newX, oldY, newY, oldZ, newZ;     // values of the translation vector
    Quaternion<float> oldRotation, newRotation;   // value of the rotation quaternion
};

///// class CommandSetupGlobal ////////////////////////////////////////////////
class CommandSetupGlobal : public Command
{
  public:
    ///// constructor/destructor
    CommandSetupGlobal(XbraboView* parent, const QString description);// constructor
    virtual CommandSetupGlobal* clone() const;    // virtual copy constructor

    ///// public member functions
    bool execute(bool fromBackup = false);        // Executes the command
    bool revert();                      // Reverts the effects of executing the command.    

  private:
    ///// private member variables
    GlobalBase::WidgetData oldData, newData;      // all data for the Global setup dialog
};

///// class CommandSetupBrabo /////////////////////////////////////////////////
class CommandSetupBrabo : public Command
{
  public:
    ///// constructor/destructor
    CommandSetupBrabo(XbraboView* parent, const QString description); // constructor
    virtual CommandSetupBrabo* clone() const;     // virtual copy constructor

    ///// public member functions
    bool execute(bool fromBackup = false);        // Executes the command
    bool revert();                      // Reverts the effects of executing the command.    

  private:
    ///// private member variables
    BraboBase::WidgetData oldData, newData;       // all data for the Brabo setup dialog
};

///// class CommandSetupRelax /////////////////////////////////////////////////
class CommandSetupRelax : public Command
{
  public:
    ///// constructor/destructor
    CommandSetupRelax(XbraboView* parent, const QString description); // constructor
    virtual CommandSetupRelax* clone() const;     // virtual copy constructor

    ///// public member functions
    bool execute(bool fromBackup = false);        // Executes the command
    bool revert();                      // Reverts the effects of executing the command.    

  private:
    ///// private member variables
    RelaxBase::WidgetData oldData, newData;       // all data for the Relax setup dialog
};

#endif
