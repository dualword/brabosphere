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
  present in this file. The implementation is such as to allow a stack for each 
  calculation.
  The list of classes: Command (abstract)
                         CommandCoordinates (abstract)
                           CommandReadCoordinates
                           CommandAddAtoms
                           CommandDeleteAtoms
                           CommandAlterCartesian
                           CommandAlterInternal
                           CommandTranslateSelectionXY
                           CommandTranslateSelectionZ
                           CommandRotateSelection
                           CommandChangeIC
                         CommandSelection (abstract)
                           CommandSelectAll
                           CommandSelectNone
                           CommandSelectEntity
                         CommandDisplayMode
                         CommandTranslate (abstract)
                           CommandCenterView
                           CommandTranslateXY
                         CommandRotate (abstract)
                           CommandResetOrientation
                           CommandRotateXYZ
                         CommandZoom (abstract)
                           CommandZoomFit
                           CommandTranslateZ
                         CommandResetView

*/
/// \file
/// Contains the implementation of the class Command and its subclasses.

///// Header files ////////////////////////////////////////////////////////////

// C++ header files
#include <cassert>

// Qt header files
#include <qdockwindow.h>
#include <qstatusbar.h>

// Xbrabo header files
#include "atomset.h"
#include "calculation.h"
#include "command.h"
#include "glmoleculeview.h"
#include "newatombase.h"
#include "point3d.h" // needed for the copy constructor of AtomSet (CommandReadCoordinates, CommandAddAtoms)
#include "xbraboview.h"

///////////////////////////////////////////////////////////////////////////////
///// Class Command                                                       /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
Command::Command(XbraboView* parent, const QString description) :
  view(parent),
  repeatable(false),
  desc(description)
/// The default constructor.
{
  assert(view != NULL);
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

///// combine /////////////////////////////////////////////////////////////////
bool Command::combine(Command* command)
/// The default implementation of combining 2 Commands returns false so it does
/// not need to be reimplmented by all subclasses.
{
  return false;
}

///// ramSize /////////////////////////////////////////////////////////////////
unsigned int Command::ramSize() const
/// Returns the size needed for storing this class in RAM (approximately)
{
  return sizeof(this); // this value should change when called from a subclass
}

///// isRepeatable ////////////////////////////////////////////////////////////
bool Command::isRepeatable() const
/// Returns whether the command can be executed repeatedly. The default value is false.
{
  return repeatable;
}


/*
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
bool CommandNewCalculation::execute(bool)
/// Creates a new calculation. The procedure is the same whether it's executed 
/// for the first time or following an undo operation.
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
bool CommandOpenCalculation::execute(bool)
/// Opens an existing calculation. The procedure is the same whether it's executed 
/// for the first time or following an undo operation because 'fileName' is never
/// empty.
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
bool CommandPreferences::execute(bool fromBackup)
/// Changes the program's preferences and keeps a copy of the previous state.
/// When restoring the preferences, the Preferences dialog is not shown.
{
  if(!fromBackup)
  {
    // backup old data
    oldData = mainWindow->editPreferences->data;
    // show the dialog
    return mainWindow->changePreferences();
  }
  else
  {
    // re-apply new vales
    mainWindow->editPreferences->data = newData;
    mainWindow->editPreferences->restoreWidgets();
    mainWindow->editPreferences->applyChanges();
    return true;
  }
}

///// revert //////////////////////////////////////////////////////////////////
bool CommandPreferences::revert()
/// Reverts any changes made in the Preferences dialog.
{
  // backup new data
  newData = mainWindow->editPreferences->data;
  // revert to old data
  mainWindow->editPreferences->data = oldData;
  mainWindow->editPreferences->restoreWidgets(); // update the widgets from the data struct
  mainWindow->editPreferences->applyChanges(); // update the data struct from the widgets and update everything
  return true;
}


///////////////////////////////////////////////////////////////////////////////
///// Class CommandDockWindow                                             /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandDockWindow::CommandDockWindow(Xbrabo* parent, const QString description, QDockWindow* dock) : Command(parent, description),
  dockWindow(dock)
/// The default constructor.
{

}

///// copy constructor ////////////////////////////////////////////////////////
CommandDockWindow* CommandDockWindow::clone() const
/// The copy constructor using the 'virtual constructor idiom'
{
  return new CommandDockWindow(*this);
}

///// execute /////////////////////////////////////////////////////////////////
bool CommandDockWindow::execute(bool)
/// Toggles the visibility of the given QDockWindow. The procedure is the same
/// whether it's executed for the first time or following an undo operation.
{
  if(dockWindow->isVisibleTo(mainWindow))
    dockWindow->hide();
  else
    dockWindow->show();
  return true;
}

///// revert //////////////////////////////////////////////////////////////////
bool CommandDockWindow::revert()
/// Reverts the visibility.
{
  return execute();
}


///////////////////////////////////////////////////////////////////////////////
///// Class CommandStatusBar                                              /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandStatusBar::CommandStatusBar(Xbrabo* parent, const QString description) : Command(parent, description)
/// The default constructor.
{

}

///// copy constructor ////////////////////////////////////////////////////////
CommandStatusBar* CommandStatusBar::clone() const
/// The copy constructor using the 'virtual constructor idiom'
{
  return new CommandStatusBar(*this);
}

///// execute /////////////////////////////////////////////////////////////////
bool CommandStatusBar::execute(bool)
/// Toggles the visibility of the statusbar. The procedure is the same
/// whether it's executed for the first time or following an undo operation.
{
  if(mainWindow->statusBar()->isVisibleTo(mainWindow))
    mainWindow->statusBar()->hide();
  else
    mainWindow->statusBar()->show();
  mainWindow->fixToplevelModeHeight();
  return true;
}

///// revert //////////////////////////////////////////////////////////////////
bool CommandStatusBar::revert()
/// Reverts the visibility.
{
  return execute();
}
*/


///////////////////////////////////////////////////////////////////////////////
///// Class CommandCoordinates                                            /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandCoordinates::CommandCoordinates(XbraboView* parent, const QString description) : Command(parent, description),
  oldAtoms(NULL),
  newAtoms(NULL)
/// The default constructor. 
{
 
}

///// destructor //////////////////////////////////////////////////////////////
CommandCoordinates::~CommandCoordinates()
/// The default destructor.
{
  if(oldAtoms != NULL)
    delete oldAtoms;
  if(newAtoms != NULL)
    delete newAtoms;
}

///// execute /////////////////////////////////////////////////////////////////
bool CommandCoordinates::execute(bool fromBackup)
/// Changes the coordinate set in one way or the other
{
  if(view->isRunning())
    return false;

  qDebug("CommandCoordinates::execute");

  assert(oldAtoms == NULL); // should be zero at start and after a 'revert' operation.
  oldAtoms = new AtomSet(view->currentAtomSet()); // backup current situation
  oldSelectionList = view->moleculeView()->selectionList;

  if(!fromBackup)
  {
    // this is the first call of execute.
    assert(newAtoms == NULL);
    qDebug("count() before initialRun = %d", view->currentAtomSet()->count());
    return initialRun(); // the call differing between subclasses
  }
  else
  {
    assert(newAtoms != NULL); // fromBackup version is only called for 'redo' so revert should have been called 
    view->moleculeView()->selectionList = newSelectionList;
    view->setAtomSet(newAtoms);
    newAtoms = NULL; // ownership transfered to XbraboView
    return true;
  }
}

///// revert //////////////////////////////////////////////////////////////////
bool CommandCoordinates::revert()
/// Restores the previous set of atoms.
{
  if(view->isRunning())
    return false;

  qDebug("CommandCoordinates::revert: oldAtoms->count() = %d", oldAtoms->count());
  assert(oldAtoms != NULL); // execute should have been called
  assert(newAtoms == NULL); // always NULL after a run of execute and at start

  newAtoms = new AtomSet(view->currentAtomSet()); // backup current situation
  newSelectionList = view->moleculeView()->selectionList;
  qDebug("CommandCoordinates::revert: newAtoms->count() = %d", newAtoms->count());

  view->moleculeView()->selectionList = oldSelectionList;
  view->setAtomSet(oldAtoms);
  oldAtoms = NULL; // ownership transfered to XbraboView;
  return true;
}

///// combine /////////////////////////////////////////////////////////////////
bool CommandCoordinates::combine(Command* command)
/// The default implementation of combining 2 Commands returns false so it does
/// not need to be reimplmented by all subclasses.
{
  return false;
}

///// ramSize /////////////////////////////////////////////////////////////////
unsigned int CommandCoordinates::ramSize() const
/// Returns the size needed for storing this class in RAM (approximately). 
/// Overridden from Command::ramSize
{
  unsigned int result = sizeof(this); // not calling the base class version as I think it might return the size of the base class
  if(oldAtoms != NULL)
    result += oldAtoms->ramSize();
  if(newAtoms != NULL)
    result += newAtoms->ramSize();
  result += (oldSelectionList.size() + newSelectionList.size()) * sizeof(unsigned int);
  return result;
}


///////////////////////////////////////////////////////////////////////////////
///// Class CommandReadCoordinates                                        /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandReadCoordinates::CommandReadCoordinates(XbraboView* parent, const QString description) : CommandCoordinates(parent, description)
/// The default constructor. 
{

}

///// copy constructor ////////////////////////////////////////////////////////
CommandReadCoordinates* CommandReadCoordinates::clone() const
/// The copy constructor using the 'virtual constructor idiom'
{
  return new CommandReadCoordinates(*this);
}

///// initialRun //////////////////////////////////////////////////////////////
bool CommandReadCoordinates::initialRun()
/// Reads a new set of atoms for the given calculation.
{
  return view->moleculeReadCoordinates();
}

///// execute /////////////////////////////////////////////////////////////////
bool CommandReadCoordinates::execute(bool fromBackup)
/// Reads a new set of atoms for the given calculation. 
{
  if(view->isRunning())
    return false;

  assert(oldAtoms == NULL); // should be zero at start and after a 'revert' operation.
  oldAtoms = new AtomSet(view->currentAtomSet()); // backup current situation
  oldSelectionList = view->moleculeView()->selectionList;
  oldX = view->moleculeView()->xPos;
  oldY = view->moleculeView()->yPos;
  oldZ = view->moleculeView()->zPos;
  oldRotation = *(view->moleculeView()->orientationQuaternion);

  if(!fromBackup)
  {
    // this is the first call of execute.
    assert(newAtoms == NULL);
    return initialRun();
  }
  else
  {
    assert(newAtoms != NULL); // fromBackup version is only called for 'redo' so revert should have been called 
    view->setAtomSet(newAtoms);
    view->moleculeView()->resetView(); // this is also done by initialRun();
    newAtoms = NULL; // ownership transfered to XbraboView
    return true;
  }
}

///// revert //////////////////////////////////////////////////////////////////
bool CommandReadCoordinates::revert()
/// Restores the previous set of atoms.
{
  if(view->isRunning())
    return false;

  assert(oldAtoms != NULL); // execute should have been called
  assert(newAtoms == NULL); // always NULL after a run of execute and at start

  newAtoms = new AtomSet(view->currentAtomSet()); // backup current situation
  
  view->moleculeView()->selectionList = oldSelectionList;
  view->moleculeView()->xPos = oldX;
  view->moleculeView()->yPos = oldY;
  view->moleculeView()->zPos = oldZ;
  *(view->moleculeView()->orientationQuaternion) = oldRotation;
  view->setAtomSet(oldAtoms);
  oldAtoms = NULL; // ownership transfered to XbraboView;
  return true;
}


///////////////////////////////////////////////////////////////////////////////
///// Class CommandAddAtoms                                               /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandAddAtoms::CommandAddAtoms(XbraboView* parent, const QString description, NewAtomBase* newAtomDialog) : 
  CommandCoordinates(parent, description),
  newAtomBase(newAtomDialog)
/// The default constructor. 
{
  assert(newAtomBase != 0);
}

///// copy constructor ////////////////////////////////////////////////////////
CommandAddAtoms* CommandAddAtoms::clone() const
/// The copy constructor using the 'virtual constructor idiom'
{
  return new CommandAddAtoms(*this);
}

///// initialRun //////////////////////////////////////////////////////////////
bool CommandAddAtoms::initialRun()
/// Adds atoms to the current molecular system. 
{
  newAtomBase->addAtom();
  return true;
}

///////////////////////////////////////////////////////////////////////////////
///// Class CommandDeleteAtoms                                            /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandDeleteAtoms::CommandDeleteAtoms(XbraboView* parent, const QString description) : 
  CommandCoordinates(parent, description)
/// The default constructor. 
{
 
}

///// copy constructor ////////////////////////////////////////////////////////
CommandDeleteAtoms* CommandDeleteAtoms::clone() const
/// The copy constructor using the 'virtual constructor idiom'
{
  return new CommandDeleteAtoms(*this);
}

///// initialRun //////////////////////////////////////////////////////////////
bool CommandDeleteAtoms::initialRun()
/// Deletes the selected atoms from the current molecular system. 
{
  return view->moleculeView()->deleteSelectedAtoms();
}

///////////////////////////////////////////////////////////////////////////////
///// Class CommandAlterCartesian                                         /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandAlterCartesian::CommandAlterCartesian(XbraboView* parent, const QString description) : 
  CommandCoordinates(parent, description)
/// The default constructor. 
{
  
}

///// copy constructor ////////////////////////////////////////////////////////
CommandAlterCartesian* CommandAlterCartesian::clone() const
/// The copy constructor using the 'virtual constructor idiom'
{
  return new CommandAlterCartesian(*this);
}

///// initialRun /////////////////////////////////////////////////////////////////
bool CommandAlterCartesian::initialRun()
/// Changes the cartesian coordinate(s) of the current selection.
{
  return view->moleculeView()->alterCartesian();
}

///// combine /////////////////////////////////////////////////////////////////
bool CommandAlterCartesian::combine(Command* command)
/// Combines 2 changes of cartesian coordinates. 
{
  // as long as the other command is the same type nothing more has to be done.
  return dynamic_cast<CommandAlterCartesian*>(command) != NULL; 
}


///////////////////////////////////////////////////////////////////////////////
///// Class CommandAlterInternal                                          /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandAlterInternal::CommandAlterInternal(XbraboView* parent, const QString description) : 
  CommandCoordinates(parent, description)
/// The default constructor. 
{
  
}

///// copy constructor ////////////////////////////////////////////////////////
CommandAlterInternal* CommandAlterInternal::clone() const
/// The copy constructor using the 'virtual constructor idiom'
{
  return new CommandAlterInternal(*this);
}

///// initialRun /////////////////////////////////////////////////////////////////
bool CommandAlterInternal::initialRun()
/// Changes the internal coordinate formed by the current selection.
{
  return view->moleculeView()->alterInternal();
}

///// combine /////////////////////////////////////////////////////////////////
bool CommandAlterInternal::combine(Command* command)
/// Combines 2 changes of internal coordinates. 
{
  // as long as the other command is the same type nothing more has to be done.
  return dynamic_cast<CommandAlterInternal*>(command) != NULL; 
}

///////////////////////////////////////////////////////////////////////////////
///// Class CommandTranslateSelectionXY                                   /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandTranslateSelectionXY::CommandTranslateSelectionXY(XbraboView* parent, const QString description, const int amountX, const int amountY) : 
  CommandCoordinates(parent, description),
    incX(amountX),
    incY(amountY)
/// The default constructor. 
{
  repeatable = true;
}

///// copy constructor ////////////////////////////////////////////////////////
CommandTranslateSelectionXY* CommandTranslateSelectionXY::clone() const
/// The copy constructor using the 'virtual constructor idiom'
{
  return new CommandTranslateSelectionXY(*this);
}

///// initialRun /////////////////////////////////////////////////////////////////
bool CommandTranslateSelectionXY::initialRun()
/// Translates the cartesian coordinate(s) of the current selection.
{
  return view->moleculeView()->translateSelection(incX, incY, 0);
}

///// combine /////////////////////////////////////////////////////////////////
bool CommandTranslateSelectionXY::combine(Command* command)
/// Combines 2 translations of cartesian coordinates. 
{
  // as long as the other command is the same type nothing more has to be done.
  return dynamic_cast<CommandTranslateSelectionXY*>(command) != NULL; 
}


///////////////////////////////////////////////////////////////////////////////
///// Class CommandTranslateSelectionZ                                    /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandTranslateSelectionZ::CommandTranslateSelectionZ(XbraboView* parent, const QString description, const int amountZ) : 
  CommandCoordinates(parent, description),
    incZ(amountZ)
/// The default constructor. 
{
  repeatable = true;
}

///// copy constructor ////////////////////////////////////////////////////////
CommandTranslateSelectionZ* CommandTranslateSelectionZ::clone() const
/// The copy constructor using the 'virtual constructor idiom'
{
  return new CommandTranslateSelectionZ(*this);
}

///// initialRun /////////////////////////////////////////////////////////////////
bool CommandTranslateSelectionZ::initialRun()
/// Translates the cartesian coordinate(s) of the current selection.
{
  return view->moleculeView()->translateSelection(0, 0, incZ);
}

///// combine /////////////////////////////////////////////////////////////////
bool CommandTranslateSelectionZ::combine(Command* command)
/// Combines 2 translations of cartesian coordinates. 
{
  // as long as the other command is the same type nothing more has to be done.
  return dynamic_cast<CommandTranslateSelectionZ*>(command) != NULL; 
}


///////////////////////////////////////////////////////////////////////////////
///// Class CommandRotateSelection                                        /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandRotateSelection::CommandRotateSelection(XbraboView* parent, const QString description, const double amountX, const double amountY, const double amountZ) : 
  CommandCoordinates(parent, description),
    incX(amountX),
    incY(amountY),
    incZ(amountZ)
/// The default constructor. 
{
  repeatable = true;
}

///// copy constructor ////////////////////////////////////////////////////////
CommandRotateSelection* CommandRotateSelection::clone() const
/// The copy constructor using the 'virtual constructor idiom'
{
  return new CommandRotateSelection(*this);
}

///// initialRun /////////////////////////////////////////////////////////////////
bool CommandRotateSelection::initialRun()
/// Rotates the cartesian coordinate(s) of the current selection.
{
  return view->moleculeView()->rotateSelection(incX, incY, incZ);
}

///// combine /////////////////////////////////////////////////////////////////
bool CommandRotateSelection::combine(Command* command)
/// Combines 2 translations of cartesian coordinates. 
{
  // as long as the other command is the same type nothing more has to be done.
  return dynamic_cast<CommandRotateSelection*>(command) != NULL; 
}


///////////////////////////////////////////////////////////////////////////////
///// Class CommandChangeIC                                               /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandChangeIC::CommandChangeIC(XbraboView* parent, const QString description, const int range) : 
  CommandCoordinates(parent, description),
  amount(range)
/// The default constructor. 
{
  repeatable = true;
}

///// copy constructor ////////////////////////////////////////////////////////
CommandChangeIC* CommandChangeIC::clone() const
/// The copy constructor using the 'virtual constructor idiom'
{
  return new CommandChangeIC(*this);
}

///// initialRun /////////////////////////////////////////////////////////////////
bool CommandChangeIC::initialRun()
/// Changes the internal coordiante formed by the selected atoms
{
  return view->moleculeView()->changeSelectedIC(amount);
}

///// combine /////////////////////////////////////////////////////////////////
bool CommandChangeIC::combine(Command* command)
/// Combines 2 changes of the selected internal coordinate. 
{
  // as long as the other command is the same type nothing more has to be done.
  // -> maybe check for an identical selection... (not needed if all selections are also
  //    put in the undo/redo stack
  return dynamic_cast<CommandChangeIC*>(command) != NULL; 
}


///////////////////////////////////////////////////////////////////////////////
///// Class CommandSelection                                              /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandSelection::CommandSelection(XbraboView* parent, const QString description) : Command(parent, description)
/// The default constructor. 
{
 
}

///// execute /////////////////////////////////////////////////////////////////
bool CommandSelection::execute(bool fromBackup)
/// Changes the selection in one way or the other
{
  oldSelectionList = view->moleculeView()->selectionList;

  if(!fromBackup)
  {
    // this is the first call of execute.
    return initialRun(); // the call differing between subclasses
  }
  else
  {
    view->moleculeView()->selectionList = newSelectionList;
    view->moleculeView()->updateGL();
    return true;
  }
}

///// revert //////////////////////////////////////////////////////////////////
bool CommandSelection::revert()
/// Restores the previous selection.
{
  qDebug("entering CommandSelection::revert");
  newSelectionList = view->moleculeView()->selectionList;
  view->moleculeView()->selectionList = oldSelectionList;
  view->moleculeView()->updateGL();
  return true;
}

///// ramSize /////////////////////////////////////////////////////////////////
unsigned int CommandSelection::ramSize() const
/// Returns the size needed for storing this class in RAM (approximately)
{
  return sizeof(this) + (oldSelectionList.size() + newSelectionList.size()) * sizeof(unsigned int);
}


///////////////////////////////////////////////////////////////////////////////
///// Class CommandSelectAll                                              /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandSelectAll::CommandSelectAll(XbraboView* parent, const QString description) : CommandSelection(parent, description)
/// The default constructor. 
{
 
}

///// copy constructor ////////////////////////////////////////////////////////
CommandSelectAll* CommandSelectAll::clone() const
/// The copy constructor using the 'virtual constructor idiom'
{
  return new CommandSelectAll(*this);
}

///// initalRun /////////////////////////////////////////////////////////////////
bool CommandSelectAll::initialRun()
/// Selects all atoms.
{
  qDebug("entering CommandSelectAll::initialRun");
  view->moleculeView()->selectAll();
  return true;
}


///////////////////////////////////////////////////////////////////////////////
///// Class CommandSelectNone                                             /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandSelectNone::CommandSelectNone(XbraboView* parent, const QString description) : CommandSelection(parent, description)
/// The default constructor. 
{
 
}

///// copy constructor ////////////////////////////////////////////////////////
CommandSelectNone* CommandSelectNone::clone() const
/// The copy constructor using the 'virtual constructor idiom'
{
  return new CommandSelectNone(*this);
}

///// initalRun /////////////////////////////////////////////////////////////////
bool CommandSelectNone::initialRun()
/// Deselects all atoms.
{
  view->moleculeView()->unselectAll();
  return true;
}


///////////////////////////////////////////////////////////////////////////////
///// Class CommandSelectEntity                                           /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandSelectEntity::CommandSelectEntity(XbraboView* parent, const QString description, const unsigned int id) : CommandSelection(parent, description),
  glID(id)
/// The default constructor. 
{
 
}

///// copy constructor ////////////////////////////////////////////////////////
CommandSelectEntity* CommandSelectEntity::clone() const
/// The copy constructor using the 'virtual constructor idiom'
{
  return new CommandSelectEntity(*this);
}

///// initalRun /////////////////////////////////////////////////////////////////
bool CommandSelectEntity::initialRun()
/// selects or deselects the OpenGL entity with the given ID.
{
  view->moleculeView()->processSelection(glID);
  return true;
}


///////////////////////////////////////////////////////////////////////////////
///// Class CommandDisplayMode                                            /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandDisplayMode::CommandDisplayMode(XbraboView* parent, const QString description) : Command(parent, description)
/// The default constructor. 
{
  repeatable = true;
}

///// copy constructor ////////////////////////////////////////////////////////
CommandDisplayMode* CommandDisplayMode::clone() const
/// The copy constructor using the 'virtual constructor idiom'
{
  return new CommandDisplayMode(*this);
}

///// execute /////////////////////////////////////////////////////////////////
bool CommandDisplayMode::execute(bool fromBackup)
/// Changes the display mode of the molecule.
{
  oldStyleMolecule = view->moleculeView()->displayStyle(GLSimpleMoleculeView::Molecule);
  oldStyleForces = view->moleculeView()->displayStyle(GLSimpleMoleculeView::Forces);
  oldShowElements = view->moleculeView()->isShowingElements();
  oldShowNumbers = view->moleculeView()->isShowingNumbers();
  for(unsigned int type = AtomSet::None; type <= AtomSet::Stockholder; type++)
  {
    if(view->moleculeView()->isShowingCharges(type))
    {
      oldChargeType = type;
      break;
    }
  }

  if(!fromBackup)
  {
    // this is the first call of execute.
    return view->showProperties();
  }
  else
  {
    view->moleculeView()->setDisplayStyle(GLSimpleMoleculeView::Molecule, newStyleMolecule);
    view->moleculeView()->setDisplayStyle(GLSimpleMoleculeView::Forces, newStyleForces);
    view->moleculeView()->setLabels(newShowElements, newShowNumbers, newChargeType);    
    view->moleculeView()->updateGL();
    return true;
  }
}

///// revert //////////////////////////////////////////////////////////////////
bool CommandDisplayMode::revert()
/// Restores the previous selection.
{
  // backup current display mode
  newStyleMolecule = view->moleculeView()->displayStyle(GLSimpleMoleculeView::Molecule);
  newStyleForces = view->moleculeView()->displayStyle(GLSimpleMoleculeView::Forces);
  newShowElements = view->moleculeView()->isShowingElements();
  newShowNumbers = view->moleculeView()->isShowingNumbers();
  for(unsigned int type = AtomSet::None; type <= AtomSet::Stockholder; type++)
  {
    if(view->moleculeView()->isShowingCharges(type))
    {
      newChargeType = type;
      break;
    }
  }

  // revert to saved display mode
  view->moleculeView()->setDisplayStyle(GLSimpleMoleculeView::Molecule, oldStyleMolecule);
  view->moleculeView()->setDisplayStyle(GLSimpleMoleculeView::Forces, oldStyleForces);
  view->moleculeView()->setLabels(oldShowElements, oldShowNumbers, oldChargeType);
  view->moleculeView()->updateGL();
  return true;
}


///////////////////////////////////////////////////////////////////////////////
///// Class CommandTranslation                                             /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandTranslation::CommandTranslation(XbraboView* parent, const QString description) : Command(parent, description)
/// The default constructor. 
{
  
}

///// execute /////////////////////////////////////////////////////////////////
bool CommandTranslation::execute(bool fromBackup)
/// Changes the translation of the molecule in the plane of the screen (X & Y).
{
  oldX = view->moleculeView()->xPos;
  oldY = view->moleculeView()->yPos;

  if(!fromBackup)
  {
    return initialRun();
  }
  else
  {
    view->moleculeView()->xPos = newX;
    view->moleculeView()->yPos = newY;
    view->moleculeView()->updateGL();
    return true;
  }
}

///// revert //////////////////////////////////////////////////////////////////
bool CommandTranslation::revert()
/// Restores the previous translation.
{
  newX = view->moleculeView()->xPos;
  newY = view->moleculeView()->yPos;

  view->moleculeView()->xPos = oldX;
  view->moleculeView()->yPos = oldY;
  view->moleculeView()->updateGL();
  return true;
}


///////////////////////////////////////////////////////////////////////////////
///// Class CommandTranslateXY                                            /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandTranslateXY::CommandTranslateXY(XbraboView* parent, const QString description, const int amountX, const int amountY) : CommandTranslation(parent, description),
  incX(amountX),
  incY(amountY)
/// The default constructor. 
{
  repeatable = true;
}

///// copy constructor ////////////////////////////////////////////////////////
CommandTranslateXY* CommandTranslateXY::clone() const
/// The copy constructor using the 'virtual constructor idiom'
{
  return new CommandTranslateXY(*this);
}

///// initialRun //////////////////////////////////////////////////////////////
bool CommandTranslateXY::initialRun()
/// Changes the translation of the molecule in the plane of the screen (X & Y).
{
  if(incX == 0 && incY == 0)
    return false;
  view->moleculeView()->translateXY(incX, incY);
  return true;
}

///// combine /////////////////////////////////////////////////////////////////
bool CommandTranslateXY::combine(Command* command)
/// Combines 2 translations in the plane of the screen.
{
  // as long as the other command is the same type nothing more has to be done.
  return dynamic_cast<CommandTranslateXY*>(command) != NULL; 
}


///////////////////////////////////////////////////////////////////////////////
///// Class CommandCenterView                                             /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandCenterView::CommandCenterView(XbraboView* parent, const QString description) : CommandTranslation(parent, description)
/// The default constructor. 
{
  
}

///// copy constructor ////////////////////////////////////////////////////////
CommandCenterView* CommandCenterView::clone() const
/// The copy constructor using the 'virtual constructor idiom'
{
  return new CommandCenterView(*this);
}

///// initialRun //////////////////////////////////////////////////////////////
bool CommandCenterView::initialRun()
/// Resets the translation of the molecule in the plane of the screen (X & Y).
{
  view->moleculeView()->centerView();
  return true;
}

///////////////////////////////////////////////////////////////////////////////
///// Class CommandZoom                                                   /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandZoom::CommandZoom(XbraboView* parent, const QString description) : Command(parent, description)
/// The default constructor. 
{
  
}

///// execute /////////////////////////////////////////////////////////////////
bool CommandZoom::execute(bool fromBackup)
/// Changes the translation of the molecule out of the plane of the screen (Z).
{
  oldZ = view->moleculeView()->zPos;

  if(!fromBackup)
  {
    return initialRun();
  }
  else
  {
    view->moleculeView()->zPos = newZ;
    view->moleculeView()->updateGL();
    return true;
  }
}

///// revert //////////////////////////////////////////////////////////////////
bool CommandZoom::revert()
/// Restores the previous translation.
{
  newZ = view->moleculeView()->zPos;

  view->moleculeView()->zPos = oldZ;
  view->moleculeView()->updateGL();
  return true;
}


///////////////////////////////////////////////////////////////////////////////
///// Class CommandTranslateZ                                             /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandTranslateZ::CommandTranslateZ(XbraboView* parent, const QString description, const int amount) : CommandZoom(parent, description),
  incZ(amount)
/// The default constructor. 
{
  repeatable = true;
}

///// copy constructor ////////////////////////////////////////////////////////
CommandTranslateZ* CommandTranslateZ::clone() const
/// The copy constructor using the 'virtual constructor idiom'
{
  return new CommandTranslateZ(*this);
}

///// initialRun //////////////////////////////////////////////////////////////
bool CommandTranslateZ::initialRun()
/// Changes the translation of the molecule in the direction perpendicular to
/// the screen (essentially zooming)
{
    if(incZ == 0)
      return false;
    view->moleculeView()->translateZ(incZ);
  return true;
}

///// combine /////////////////////////////////////////////////////////////////
bool CommandTranslateZ::combine(Command* command)
/// Combines 2 zoom actions.
{
  // as long as the other command is the same type nothing more has to be done.
  return dynamic_cast<CommandTranslateZ*>(command) != NULL; 
}


///////////////////////////////////////////////////////////////////////////////
///// Class CommandZoomFit                                                /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandZoomFit::CommandZoomFit(XbraboView* parent, const QString description) : CommandZoom(parent, description)
/// The default constructor. 
{

}

///// copy constructor ////////////////////////////////////////////////////////
CommandZoomFit* CommandZoomFit::clone() const
/// The copy constructor using the 'virtual constructor idiom'
{
  return new CommandZoomFit(*this);
}

///// initialRun //////////////////////////////////////////////////////////////
bool CommandZoomFit::initialRun()
/// Zooms the molecule so it fits in the OpenGL window.
{
  view->moleculeView()->zoomFit();
  return true;
}


///////////////////////////////////////////////////////////////////////////////
///// Class CommandRotation                                                 /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandRotation::CommandRotation(XbraboView* parent, const QString description) : Command(parent, description)
/// The default constructor. 
{
  
}

///// execute /////////////////////////////////////////////////////////////////
bool CommandRotation::execute(bool fromBackup)
/// Changes the rotation of the molecule.
{
  oldRotation = *(view->moleculeView()->orientationQuaternion);

  if(!fromBackup)
    return initialRun();
  else
  {
    *(view->moleculeView()->orientationQuaternion) = newRotation;
    view->moleculeView()->updateGL();
    return true;
  }
}

///// revert //////////////////////////////////////////////////////////////////
bool CommandRotation::revert()
/// Restores the previous rotation.
{
  newRotation = *(view->moleculeView()->orientationQuaternion);
  
  *(view->moleculeView()->orientationQuaternion) = oldRotation;
  view->moleculeView()->updateGL();
  return true;
}


///////////////////////////////////////////////////////////////////////////////
///// Class CommandRotate                                                 /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandRotate::CommandRotate(XbraboView* parent, const QString description, const float amountX, const float amountY, const float amountZ) : 
  CommandRotation(parent, description),
  incX(amountX),
  incY(amountY),
  incZ(amountZ)
/// The default constructor. 
{
  repeatable = true;
}

///// copy constructor ////////////////////////////////////////////////////////
CommandRotate* CommandRotate::clone() const
/// The copy constructor using the 'virtual constructor idiom'
{
  return new CommandRotate(*this);
}

///// initialRun //////////////////////////////////////////////////////////////
bool CommandRotate::initialRun()
/// Changes the rotation of the molecule by settings the step rotation.
{
  view->moleculeView()->rotate(incX, incY, incZ);
  return true;
}

///// combine /////////////////////////////////////////////////////////////////
bool CommandRotate::combine(Command* command)
/// Combines 2 rotate actions.
{
  // as long as the other command is the same type nothing more has to be done.
  return dynamic_cast<CommandRotate*>(command) != NULL; 
}


///////////////////////////////////////////////////////////////////////////////
///// Class CommandResetOrientation                                       /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandResetOrientation::CommandResetOrientation(XbraboView* parent, const QString description) : CommandRotation(parent, description)
/// The default constructor. 
{
  
}

///// copy constructor ////////////////////////////////////////////////////////
CommandResetOrientation* CommandResetOrientation::clone() const
/// The copy constructor using the 'virtual constructor idiom'
{
  return new CommandResetOrientation(*this);
}

///// initialRun //////////////////////////////////////////////////////////////
bool CommandResetOrientation::initialRun()
/// Changes the rotation of the molecule by settings the step rotation.
{
  view->moleculeView()->resetOrientation();
  return true;
}

///////////////////////////////////////////////////////////////////////////////
///// Class CommandResetView                                              /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandResetView::CommandResetView(XbraboView* parent, const QString description) : Command(parent, description)
/// The default constructor. 
{
  
}

///// copy constructor ////////////////////////////////////////////////////////
CommandResetView* CommandResetView::clone() const
/// The copy constructor using the 'virtual constructor idiom'
{
  return new CommandResetView(*this);
}

///// execute /////////////////////////////////////////////////////////////////
bool CommandResetView::execute(bool fromBackup)
/// Resets the translation and rotation of te molecule.
{
  oldX = view->moleculeView()->xPos;
  oldY = view->moleculeView()->yPos;
  oldZ = view->moleculeView()->zPos;
  oldRotation = *(view->moleculeView()->orientationQuaternion);

  if(!fromBackup)
  {
    view->moleculeView()->resetView();
  }
  else
  {
    view->moleculeView()->xPos = newX;
    view->moleculeView()->yPos = newY;
    view->moleculeView()->zPos = newZ;
    *(view->moleculeView()->orientationQuaternion) = newRotation;
    view->moleculeView()->updateGL();
    return true;
  }
}

///// revert //////////////////////////////////////////////////////////////////
bool CommandResetView::revert()
/// Restores the previous translation and rotation.
{
  newX = view->moleculeView()->xPos;
  newY = view->moleculeView()->yPos;
  newZ = view->moleculeView()->zPos;
  newRotation = *(view->moleculeView()->orientationQuaternion);

  view->moleculeView()->xPos = oldX;
  view->moleculeView()->yPos = oldY;
  view->moleculeView()->zPos = oldZ;
  *(view->moleculeView()->orientationQuaternion) = oldRotation;
  view->moleculeView()->updateGL();
  return true;
}


///////////////////////////////////////////////////////////////////////////////
///// Class CommandSetupGlobal                                            /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandSetupGlobal::CommandSetupGlobal(XbraboView* parent, const QString description) : Command(parent, description)
/// The default constructor. 
{
  
}

///// copy constructor ////////////////////////////////////////////////////////
CommandSetupGlobal* CommandSetupGlobal::clone() const
/// The copy constructor using the 'virtual constructor idiom'
{
  return new CommandSetupGlobal(*this);
}

///// execute /////////////////////////////////////////////////////////////////
bool CommandSetupGlobal::execute(bool fromBackup)
/// Changes the Global setup options.
{
  if(view->globalSetup != 0)
    oldData = view->globalSetup->data;
  else
    oldData.type = GlobalBase::Frequencies + 10;

  if(!fromBackup)
    return view->setupGlobal();
  else
  {
    view->globalSetup->data = newData;
    view->globalSetup->restoreWidgets();
    return true;
  }
}

///// revert //////////////////////////////////////////////////////////////////
bool CommandSetupGlobal::revert()
/// Restores the previous translation and rotation.
{
  newData = view->globalSetup->data;

  if(oldData.type != GlobalBase::Frequencies + 10)
  {
    view->globalSetup->data = oldData;
    view->globalSetup->restoreWidgets();
  }
  else
    view->globalSetup->reset();
  return true;
}


///////////////////////////////////////////////////////////////////////////////
///// Class CommandSetupBrabo                                             /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandSetupBrabo::CommandSetupBrabo(XbraboView* parent, const QString description) : Command(parent, description)
/// The default constructor. 
{
  
}

///// copy constructor ////////////////////////////////////////////////////////
CommandSetupBrabo* CommandSetupBrabo::clone() const
/// The copy constructor using the 'virtual constructor idiom'
{
  return new CommandSetupBrabo(*this);
}

///// execute /////////////////////////////////////////////////////////////////
bool CommandSetupBrabo::execute(bool fromBackup)
/// Changes the Brabo setup options.
{
  if(view->braboSetup != 0)
    oldData = view->braboSetup->data;
  else
    oldData.SCFMethod = 10;

  if(!fromBackup)
    return view->setupBrabo();
  else
  {
    view->braboSetup->data = newData;
    view->braboSetup->restoreWidgets();
    return true;
  }
}

///// revert //////////////////////////////////////////////////////////////////
bool CommandSetupBrabo::revert()
/// Restores the previous set of Energy & Forces options.
{
  newData = view->braboSetup->data;

  if(oldData.SCFMethod != 10)
  {
    view->braboSetup->data = oldData;
    view->braboSetup->restoreWidgets();
  }
  else
    view->braboSetup->reset();
  return true;
}


///////////////////////////////////////////////////////////////////////////////
///// Class CommandSetupRelax                                             /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
CommandSetupRelax::CommandSetupRelax(XbraboView* parent, const QString description) : Command(parent, description)
/// The default constructor. 
{
  
}

///// copy constructor ////////////////////////////////////////////////////////
CommandSetupRelax* CommandSetupRelax::clone() const
/// The copy constructor using the 'virtual constructor idiom'
{
  return new CommandSetupRelax(*this);
}

///// execute /////////////////////////////////////////////////////////////////
bool CommandSetupRelax::execute(bool fromBackup)
/// Changes the Relax setup options.
{
  if(view->relaxSetup != 0)
    oldData = view->relaxSetup->data;
  else
    oldData.type = 10;

  if(!fromBackup)
    return view->setupRelax();
  else
  {
    view->relaxSetup->data = newData;
    view->relaxSetup->restoreWidgets();
    return true;
  }
}

///// revert //////////////////////////////////////////////////////////////////
bool CommandSetupRelax::revert()
/// Restores the previous translation and rotation.
{
  newData = view->relaxSetup->data;

  if(oldData.type != 10)
  {
    view->relaxSetup->data = oldData;
    view->relaxSetup->restoreWidgets();
  }
  else
    view->relaxSetup->reset();
  return true;
}

