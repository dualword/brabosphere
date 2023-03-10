/***************************************************************************
                      glmoleculeview.cpp  -  description
                             -------------------
    begin                : Mon Jul 29 2002
    copyright            : (C) 2002-2006 by Ben Swerts
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
  \class GLMoleculeView
  \brief This class shows a molecule and various properties in 3D using OpenGL.

  It is a subclass of GLSimpleMoleculeView and additionally allows changing the
  molecule, visualisation of isodensity surfaces etc.

*/
/// \file
/// Contains the implementation of the class GLMoleculeView

///// Header files ////////////////////////////////////////////////////////////

// C++ header files
#include <cassert>
#include <cmath> // using abs() for floats/doubles doesn't work on e.g. GCC 3.3.3 so keep using fabs()
#include <cstring> // for memset

// STL header files
#include <algorithm>

// GLee header files
#include "GLee.h"

// Qt header files
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qfiledialog.h>
#include <qimage.h>
#include <qinputdialog.h>
#include <qmessagebox.h>
#include <qradiobutton.h>
#include <qslider.h>
#include <qstringlist.h>
#include <qtimer.h>
#include <qvalidator.h>

// Xbrabo header files
#include "atomset.h"
#include "colorbutton.h" // maybe this one shouldn't be here but provide getters for colors of DensityBase
#include "command.h"
#include "commandhistory.h"
#include "coordinateswidget.h"
#include "densitybase.h"
#include "glmoleculeview.h"
#include "densitygrid.h"
#include "newatombase.h"
#include "point3d.h"
#include "quaternion.h"
#include "vector3d.h"
#include "xbraboview.h"


///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// constructor /////////////////////////////////////////////////////////////
GLMoleculeView::GLMoleculeView(AtomSet* atomset, QWidget* parent, const char* name) : GLSimpleMoleculeView(atomset, parent, name),
  densityDialog(NULL),
  newAtomDialog(NULL),
  numVolumeObjects(0),
  textureID2D(NULL),
  textureID3D(0),
  sliceObject(0) 
/// The default constructor.
{
  densityGrid = new DensityGrid();
}

///// destructor //////////////////////////////////////////////////////////////
GLMoleculeView::~GLMoleculeView()
/// The default destructor.
{
  makeCurrent();
  for(unsigned int i = 0; i < glSurfaces.size(); i++)
    glDeleteLists(glSurfaces[i], 1);
  if(sliceObject != 0) 
    glDeleteLists(sliceObject, 1);
  delete densityGrid;
}

///// setAtomSet //////////////////////////////////////////////////////////////
void GLMoleculeView::setAtomSet(AtomSet* atomSet)
/// Updates the current AtomSet from a pointer.
{
  assert(atomSet);

  qDebug("entering GLMoleculeView::setAtomSet with atomSet->count() = %d, atoms->count() = %d", atomSet->count(), atoms->count());

  atoms = atomSet;

  qDebug(" after assignment atomSet->count() = %d, atoms->count() = %d", atomSet->count(), atoms->count());
  if(newAtomDialog != NULL)
    newAtomDialog->setAtomSet(atomSet);
  updateAtomSet(false); // don't reset the view or the selected atoms but do call updateGL

  qDebug(" after updateAtomSet atomSet->count() = %d, atoms->count() = %d", atomSet->count(), atoms->count());
}

///// alterCartesian //////////////////////////////////////////////////////////
bool GLMoleculeView::alterCartesian()
/// Changes the cartesian coordinates of the selection.
/// If one atom is selected, the absolute coordinates can be changed
/// If multiple atoms are selected, only relative changes can be given.
{
  if(selectionList.size() == 0)
    return false;

  ///// setup the dialog
  CoordinatesWidget* coords = new CoordinatesWidget(this, 0, true);
  QDoubleValidator* v = new QDoubleValidator(-9999.0,9999.,12,this);
  coords->LineEditX->setValidator(v);
  coords->LineEditY->setValidator(v);
  coords->LineEditZ->setValidator(v);
  v = 0;
  if(selectionList.size() == 1)
  {
    // use default absolute changes
    unsigned int atom = *selectionList.begin();
    coords->LineEditX->setText(QString::number(atoms->x(atom), 'f', 8));
    coords->LineEditY->setText(QString::number(atoms->y(atom), 'f', 8));
    coords->LineEditZ->setText(QString::number(atoms->z(atom), 'f', 8));
  }
  else
  {
    // only relative changes
    coords->RadioButtonAbsolute->setEnabled(false);
    coords->RadioButtonRelative->setChecked(true);
    coords->LineEditX->setText("0.0");
    coords->LineEditY->setText("0.0");
    coords->LineEditZ->setText("0.0");
  }

  ///// run the dialog
  if(coords->exec() == QDialog::Accepted)
  {
    if(coords->RadioButtonAbsolute->isChecked())
    {
      // absolute changes for one atom
      unsigned int atom = *selectionList.begin();
      bool ok;
      double newx = coords->LineEditX->text().stripWhiteSpace().toDouble(&ok);
      if(ok)
        atoms->setX(atom, newx);
      double newy = coords->LineEditY->text().stripWhiteSpace().toDouble(&ok);
      if(ok)
        atoms->setY(atom, newy);
      double newz = coords->LineEditZ->text().stripWhiteSpace().toDouble(&ok);
      if(ok)
        atoms->setZ(atom, newz);
    }
    else
    {
      // relative changes for one or more atoms
      bool ok;
      double deltax = coords->LineEditX->text().stripWhiteSpace().toDouble(&ok);
      if(!ok)
        deltax = 0.0;
      double deltay = coords->LineEditY->text().stripWhiteSpace().toDouble(&ok);
      if(!ok)
        deltay = 0.0;
      double deltaz = coords->LineEditZ->text().stripWhiteSpace().toDouble(&ok);
      if(!ok)
        deltaz = 0.0;
      std::list<unsigned int>::iterator it = selectionList.begin();
      while(it != selectionList.end())
      {
        atoms->setX(*it, atoms->x(*it) + deltax);
        atoms->setY(*it, atoms->y(*it) + deltay);
        atoms->setZ(*it, atoms->z(*it) + deltaz);
        it++;
      }
    }
    setModified();
    updateAtomSet();
    delete coords;
    return true;
  }
  else
  {
    delete coords;
    return false;
  }
}

///// alterInternal ///////////////////////////////////////////////////////////
bool GLMoleculeView::alterInternal()
/// Changes the cartesian coordinates of the selection.
/// If one atom is selected, the absolute coordinates can be changed
/// If multiple atoms are selected, only relative changes can be given
{
  switch(getSelectionType())
  {
    case SELECTION_BOND:
    {
      std::list<unsigned int>::iterator it = selectionList.begin();
      unsigned int atom1 = *it++;
      unsigned int atom2 = *it;
      ///// get the current bond length
      // <double> version
      Vector3D<double> bond(atoms->x(atom2), atoms->y(atom2), atoms->z(atom2), atoms->x(atom1), atoms->y(atom1), atoms->z(atom1));
      double bondLength = bond.length();

      bool ok;
      double newLength = QInputDialog::getDouble("Xbrabo", tr("Change the distance between atoms ")+QString::number(atom1+1)+" and "+QString::number(atom2+1), bondLength, -1000.0, 1000.0, 4, &ok, this);
      if(ok && fabs(newLength - bondLength) > 0.00001)
        atoms->changeBond(newLength - bondLength, atom1, atom2, true);
      else
        return false; // no new value was entered
      break;
    }

    case SELECTION_ANGLE:
    {
      std::list<unsigned int>::iterator it = selectionList.begin();
      unsigned int atom1 = *it++;
      unsigned int atom2 = *it++;
      unsigned int atom3 = *it;
      ///// get the current angle
      Vector3D<double> bond1(atoms->x(atom2), atoms->y(atom2), atoms->z(atom2), atoms->x(atom1), atoms->y(atom1), atoms->z(atom1));
      Vector3D<double> bond2(atoms->x(atom2), atoms->y(atom2), atoms->z(atom2), atoms->x(atom3), atoms->y(atom3), atoms->z(atom3));
      double angle = bond1.angle(bond2);
      bool ok;
      double newAngle = QInputDialog::getDouble("Xbrabo", tr("Change the angle ")+QString::number(atom1+1)+"-"+QString::number(atom2+1)+"-"+QString::number(atom3+1), angle, -1000.0, 1000.0, 2, &ok, this);
      if(ok && fabs(newAngle - angle) > 0.001)
        atoms->changeAngle(newAngle - angle, atom1, atom2, atom3, true);
      else
        return false; // no new value was entered
      break;
    }

    case SELECTION_TORSION:
    {
      std::list<unsigned int>::iterator it = selectionList.begin();
      unsigned int atom1 = *it++;
      unsigned int atom2 = *it++;
      unsigned int atom3 = *it++;
      unsigned int atom4 = *it;
      ///// get the current torsion angle
      Vector3D<double> bond1(atoms->x(atom2), atoms->y(atom2), atoms->z(atom2), atoms->x(atom1), atoms->y(atom1), atoms->z(atom1));
      Vector3D<double> centralbond(atoms->x(atom2), atoms->y(atom2), atoms->z(atom2), atoms->x(atom3), atoms->y(atom3), atoms->z(atom3));
      Vector3D<double> bond2(atoms->x(atom3), atoms->y(atom3), atoms->z(atom3), atoms->x(atom4), atoms->y(atom4), atoms->z(atom4));
      double torsion = bond1.torsion(bond2, centralbond);
      bool ok;
      double newTorsion = QInputDialog::getDouble("Xbrabo", tr("Change the torsion angle ")+QString::number(atom1+1)+"-"+QString::number(atom2+1)+"-"+QString::number(atom3+1)+"-"+QString::number(atom4+1), torsion, -1000.0, 1000.0, 2, &ok, this);
      if(ok && fabs(newTorsion - torsion) > 0.001)
        atoms->changeTorsion(torsion - newTorsion, atom1, atom2, atom3, atom4, true);
      else
        return false; // no new value was entered
      break;
    }
    default: return false;
  }
  setModified();
  updateAtomSet();
  return true;
}

///// deleteSelectedAtoms /////////////////////////////////////////////////////
bool GLMoleculeView::deleteSelectedAtoms()
/// Deletes all selected atoms. This function does the actual work and is called
/// only from CommandDeleteAtoms.
{
  if(selectionList.empty())
    return false;

  ///// delete the atoms from largest to smallest index
  // make a copy
  std::vector<unsigned int> sortedList;
  sortedList.reserve(selectionList.size());
  sortedList.assign(selectionList.begin(), selectionList.end());
  // sort it from largest to smallest
  std::sort(sortedList.begin(), sortedList.end(), std::greater<unsigned int>());
  // delete the atoms
  for(unsigned int i = 0; i < sortedList.size(); i++)
    atoms->removeAtom(sortedList[i]);
  // clear the selection
  unselectAll();
  if(newAtomDialog != 0)
    newAtomDialog->updateAtomLimits();
  updateAtomSet();
  setModified();
  emit atomsetChanged();
  return true;
}

///// vertexCount /////////////////////////////////////////////////////////////
unsigned int GLMoleculeView::vertexCount()
/// Returns the number of vertices in the scene. This is a value representing
/// the geometrical complexity of the scene.
/// TODO: forces
{
  unsigned int result = 0;
  unsigned int localMoleculeStyle = displayStyle(Molecule);
  if(localMoleculeStyle > SmoothLines && atoms->count() > moleculeParameters.fastRenderLimit)
    localMoleculeStyle = Lines;

  qDebug("result1 = %d", result);
  ///// the atoms
  if(localMoleculeStyle > SmoothLines) // Tubes, BallAndStick, VanderWaals, Cartoon or BlackAndWhite
    result = atoms->count() * moleculeParameters.quality*2 * moleculeParameters.quality*2;
  qDebug("result2 = %d", result);
  if(localMoleculeStyle == Cartoon || displayStyle(Molecule) == BlackAndWhite)
    result *= 2; // due to the outline
  qDebug("result3 = %d", result);
  result *= 4; // spheres are made from quads (assuming non-shared vertices of course)
  qDebug("result4 = %d", result);

  ///// the bonds
  vector<unsigned int>* firstAtom;
  vector<unsigned int>* secondAtom;
  atoms->bonds(firstAtom, secondAtom); // assigns both pointers
  unsigned int numBonds = firstAtom->size(); // single 1-color bonds
  // add double color bonds
  if(localMoleculeStyle == Lines)
  {
    vector<unsigned int>::iterator it2 = secondAtom->begin();
    for(vector<unsigned int>::iterator it1 = firstAtom->begin(); it1 != firstAtom->end(); ++it1, ++it2)
    {
      if(atoms->color(*it1) != atoms->color(*it2))
        ++numBonds;
    }
  }
  qDebug("numBonds = %d", numBonds);
  if(localMoleculeStyle > SmoothLines && displayStyle(Molecule) != VanDerWaals) // Tubes, BallAndStick, Cartoon or BlackAndWhite
  {
    result += 4 * numBonds * moleculeParameters.quality*2;
    if(localMoleculeStyle == Cartoon || localMoleculeStyle == BlackAndWhite)
      result += 4 * numBonds * moleculeParameters.quality*2; // due to the outline
  }
  else if(localMoleculeStyle == Lines || localMoleculeStyle == SmoothLines) // (Smooth)Lines
    result += 2 * numBonds;
  qDebug("result5 = %d", result);

  ///// selections
  if(selectionList.size() > 0)
  {
    if(localMoleculeStyle > SmoothLines) // Tubes, BallAndStick, VanderWaals, Cartoon or BlackAndWhite
      result += 4 * selectionList.size() * moleculeParameters.quality*2 * moleculeParameters.quality*2;
    else
      result += selectionList.size();
    if(selectionList.size() <= SELECTION_TORSION)
    {
      if(localMoleculeStyle > SmoothLines) // Tubes, BallAndStick, VanderWaals, Cartoon or BlackAndWhite
        result += 4 * (selectionList.size()-1) * moleculeParameters.quality*2;
      else
        result += 2 * (selectionList.size()-1);
    }
  }
  qDebug("result6 = %d", result);

  ///// text (each character is a textured quad)
  unsigned int numChars = 0;
  if(isShowingElements())
    numChars += atoms->count(); // some are 2 chars but lets ignore that
  if(isShowingNumbers())
  {
    numChars += atoms->count(); // some are 2 chars but lets ignore that
    if(atoms->count() > 10)
      numChars += (atoms->count() - 9);
    if(atoms->count() > 100)
      numChars += (atoms->count() - 99);
    if(atoms->count() > 1000)
      numChars += (atoms->count() - 999);
    if(atoms->count() > 10000)
      numChars += (atoms->count() - 9999);
  }
  if(isShowingCharges(AtomSet::Stockholder) || isShowingCharges(AtomSet::Mulliken))
    numChars += atoms->count() * 9;
  qDebug("numChars = %d", numChars);
  result += numChars * 4;
  qDebug("result7 = %d", result);

  ///// surfaces/volumes/slices
  if(densityDialog != NULL && densityDialog->visualizationType() == DensityBase::ISOSURFACES)
  {
    for(unsigned int i = 0; i < densityGrid->numSurfaces(); i++)
    {
      if(densityDialog->surfaceVisible(i))
        result += densityGrid->numVertices(i);
    }
  }
  ///// volumetric rendering
  else if(densityDialog != NULL && densityDialog->visualizationType() == DensityBase::VOLUME)
  {
    result += 4 * densityGrid->getNumPoints().z();
  }
  ///// slices
  else if(densityDialog != NULL && densityDialog->visualizationType() == DensityBase::SLICE)
  {
    result += 4;
    if(densityDialog->CheckBoxSliceTransparent->isChecked())
      result += 4;
  }
  qDebug("result8 = %d", result);

  return result;
}

///// toggleSelectionMode /////////////////////////////////////////////////////
void GLMoleculeView::toggleSelectionMode()
/// Toggles between manipulating the selected atoms and the entire system.
{
  manipulateSelection = !manipulateSelection;
}

///// setParameters ///////////////////////////////////////////////////////////
void GLMoleculeView::setParameters(GLTextureParameters params)
/// Updates the OpenGL texture parameters.
{
  textureParameters = params;
}


///////////////////////////////////////////////////////////////////////////////
///// Public Slots                                                        /////
///////////////////////////////////////////////////////////////////////////////

///// alterCartesianCommand ///////////////////////////////////////////////////
void GLMoleculeView::alterCartesianCommand()
/// Creates a Command to alter the cartesian coordinates of the selected atoms.
/// This Command will call alterCartesian.
{
  XbraboView* view = (XbraboView*)(parentWidget()->parentWidget());
  view->getCommandHistory()->addCommand(new CommandAlterCartesian(view, tr("Alter Cartesian Coordinates")));
}

///// alterInternalCommand ////////////////////////////////////////////////////
void GLMoleculeView::alterInternalCommand()
/// Creates a Command to alter the internal coordinate formed by the selected
/// atoms. This Command will call alterInternal.
{
  XbraboView* view = (XbraboView*)(parentWidget()->parentWidget());
  view->getCommandHistory()->addCommand(new CommandAlterInternal(view, tr("Alter Internal Coordinates")));
}

///// deleteSelectedAtomsCommand //////////////////////////////////////////////
void GLMoleculeView::deleteSelectedAtomsCommand()
/// Creates a Command to delete all selected atoms. This Command will call
/// deleteSelectedAtoms.
{
  XbraboView* view = (XbraboView*)(parentWidget()->parentWidget());
  view->getCommandHistory()->addCommand(new CommandDeleteAtoms(view, tr("Delete Selection")));
}

///// selectAllCommand ////////////////////////////////////////////////////////
void GLMoleculeView::selectAllCommand()
/// Creates a Command to select all atoms. This Command will call selectAll.
{
  XbraboView* view = (XbraboView*)(parentWidget()->parentWidget());
  view->getCommandHistory()->addCommand(new CommandSelectAll(view, tr("Select All Atoms")));
}

///// unselectAllCommand //////////////////////////////////////////////////////
void GLMoleculeView::unselectAllCommand()
/// Creates a Command to deselect all atoms. This Command will call unselectAll.
{
  XbraboView* view = (XbraboView*)(parentWidget()->parentWidget());
  view->getCommandHistory()->addCommand(new CommandSelectNone(view, tr("Deselect All Atoms")));
}

///// centerViewCommand ///////////////////////////////////////////////////////
void GLMoleculeView::centerViewCommand()
/// Creates a Command to center the view. This Command will call centerView.
{
  XbraboView* view = (XbraboView*)(parentWidget()->parentWidget());
  view->getCommandHistory()->addCommand(new CommandCenterView(view, tr("Reset Translation")));
}

///// resetOrientationCommand /////////////////////////////////////////////////
void GLMoleculeView::resetOrientationCommand()
/// Creates a Command to reset the orientation of the view. This Command will
/// call resetOrientation.
{
  XbraboView* view = (XbraboView*)(parentWidget()->parentWidget());
  view->getCommandHistory()->addCommand(new CommandResetOrientation(view, tr("Reset Orientation")));
}

///// zoomFitCommand //////////////////////////////////////////////////////////
void GLMoleculeView::zoomFitCommand()
/// Creates a Command to reset the zoom factor. This Command will call zoomFit.
{
  XbraboView* view = (XbraboView*)(parentWidget()->parentWidget());
  view->getCommandHistory()->addCommand(new CommandZoomFit(view, tr("Reset Zoom")));
}

///// resetViewCommand ///////////////////////////////////////////////////////
void GLMoleculeView::resetViewCommand()
/// Creates a Command to reset the view. This Command will call resetView.
{
  XbraboView* view = (XbraboView*)(parentWidget()->parentWidget());
  view->getCommandHistory()->addCommand(new CommandResetView(view, tr("Reset View")));
}

///// showDensity /////////////////////////////////////////////////////////////
void GLMoleculeView::showDensity()
/// Shows electron density isosurfaces from a Gaussian .cube file.
{
  if(densityDialog == NULL)
  {
    densityDialog = new DensityBase(densityGrid, this);
    connect(densityDialog, SIGNAL(newSurface(const unsigned int)), this, SLOT(addGLSurface(const unsigned int)));
    connect(densityDialog, SIGNAL(updatedSurface(const unsigned int)), this, SLOT(updateGLSurface(const unsigned int)));
    connect(densityDialog, SIGNAL(deletedSurface(const unsigned int)), this, SLOT(deleteGLSurface(const unsigned int)));
    connect(densityDialog, SIGNAL(updatedVolume()), this, SLOT(updateVolume()));
    connect(densityDialog, SIGNAL(updatedSlice()), this, SLOT(updateSlice()));
    connect(densityDialog, SIGNAL(redrawScene()), this, SLOT(updateScene()));
  }
  densityDialog->show();
  if(!densityGrid->densityPresent())
    densityDialog->loadDensityA();
}

///// addAtoms ////////////////////////////////////////////////////////////////
void GLMoleculeView::addAtoms()
/// Shows a dialog allowing the addition of atoms to the molecule.
{
  if(newAtomDialog == NULL)
  {
    newAtomDialog = new NewAtomBase(atoms, this);
    connect(newAtomDialog, SIGNAL(atomAdded()), this, SLOT(updateAtomSet()));
    connect(newAtomDialog, SIGNAL(atomAdded()), this, SLOT(setModified()));
    connect(newAtomDialog, SIGNAL(atomAdded()), this, SIGNAL(atomsetChanged()));
  }
  newAtomDialog->show();
}


///////////////////////////////////////////////////////////////////////////////
///// Protected Member Functions                                          /////
///////////////////////////////////////////////////////////////////////////////

/*//// initializeGL ////////////////////////////////////////////////////////////
void GLMoleculeView::initializeGL()
/// Overridden from GLSimpleMoleculeView::initializeGL(). Initializes 2D and
/// 3D texturing parameters
{
  GLSimpleMoleculeView::initializeGL();
}*/

///// boundingSphereRadius ////////////////////////////////////////////////////
float GLMoleculeView::boundingSphereRadius()
/// Calculates the radius of the bounding sphere. If atoms are present, the
/// radius of the base class is used. If no atoms are present but there is a
/// density loaded, its bounding box is used.
{
  float radius = GLSimpleMoleculeView::boundingSphereRadius();

  if(atoms->count() == 0 && densityGrid->densityPresent())
  {
    ///// get the boundaries of the box
    Point3D<float> origin = densityGrid->getOrigin();
    Point3D<float> delta = densityGrid->getDelta();
    Point3D<unsigned int> numPoints = densityGrid->getNumPoints();
    float x, y, z;
    std::vector<float> squaredR;
    // the current radius
    squaredR.push_back(static_cast<float>(radius*radius));
    // point 1: the origin
    x = origin.x() - centerX;
    y = origin.y() - centerY;
    z = origin.z() - centerZ;
    squaredR.push_back(x*x + y*y + z*z);
    // point 2: along the x-axis
    x = origin.x() + delta.x() * (numPoints.x() - 1) - centerX;
    y = origin.y() - centerY;
    z = origin.z() - centerZ;
    squaredR.push_back(x*x + y*y + z*z);
    // point 3: along the y-axis
    x = origin.x() - centerX;
    y = origin.y() + delta.y() * (numPoints.y() - 1) - centerY;
    z = origin.z() - centerZ;
    squaredR.push_back(x*x + y*y + z*z);
    // point 4: along the z-axis
    x = origin.x() - centerX;
    y = origin.y() - centerY;
    z = origin.z() + delta.z() * (numPoints.z() - 1) - centerZ;
    squaredR.push_back(x*x + y*y + z*z);
    // point 5: along the x and y axes
    x = origin.x() + delta.x() * (numPoints.x() - 1) - centerX;
    y = origin.y() + delta.y() * (numPoints.y() - 1) - centerY;
    z = origin.z() - centerZ;
    squaredR.push_back(x*x + y*y + z*z);
    // point 6: along the x and z axes
    x = origin.x() + delta.x() * (numPoints.x() - 1) - centerX;
    y = origin.y() - centerY;
    z = origin.z() + delta.z() * (numPoints.z() - 1) - centerZ;
    squaredR.push_back(x*x + y*y + z*z);
    // point 7: along the y and z axes
    x = origin.x() - centerX;
    y = origin.y() + delta.y() * (numPoints.y() - 1) - centerY;
    z = origin.z() + delta.z() * (numPoints.z() - 1) - centerZ;
    squaredR.push_back(x*x + y*y + z*z);
    // point 8: along all axes
    x = origin.x() + delta.x() * (numPoints.x() - 1) - centerX;
    y = origin.y() + delta.y() * (numPoints.y() - 1) - centerY;
    z = origin.z() + delta.z() * (numPoints.z() - 1) - centerZ;
    squaredR.push_back(x*x + y*y + z*z);

    float boxradius = sqrt(*(std::max_element(squaredR.begin(), squaredR.end())));
    qDebug("GLMoleculeView::boundingSphereRadius: boxradius = %f, radius = %f",boxradius, radius);
    if(boxradius > radius)
      radius = boxradius;
  }
  return radius;
}

///// mouseMoveEvent //////////////////////////////////////////////////////////
void GLMoleculeView::mouseMoveEvent(QMouseEvent* e)
/// Overridden from GLView::mouseMoveEvent.
/// Handles left mouse button drags.
{
  QPoint newPosition = e->pos();
  unsigned int selectionType = getSelectionType();
  if(selectionType != SELECTION_NONE && e->state() & Qt::LeftButton && (manipulateSelection || e->state() & Qt::AltButton) && !(e->state() & Qt::ShiftButton && e->state() & Qt::ControlButton))
  {
    ///// leftbutton mousemoves for manipulation of the selected atoms
    if(e->state() & Qt::ShiftButton)
    {
      ///// up/down movement: zooming (z-translation)
      ///// left/right movement: z-rotation
      if(abs(newPosition.y() - mousePosition.y()) > abs(newPosition.x() - mousePosition.x()))
        translateSelectionCommand(0, 0, newPosition.y() - mousePosition.y());
      else if(newPosition.x() != mousePosition.x())
        rotateSelectionCommand(0.0, 0.0, 180.0 * static_cast<double>(newPosition.x() - mousePosition.x()) / static_cast<double>(width()));
    }
    else if(e->state() & Qt::ControlButton)
      ///// up/down movement: y-translation
      ///// left/right movement: x-translation
      translateSelectionCommand(newPosition.x() - mousePosition.x(), newPosition.y() - mousePosition.y(), 0);
    else
      ///// up/down movement: x-rotation
      ///// left/right movement: y-rotation
      rotateSelectionCommand(-180.0 * static_cast<double>(newPosition.y() - mousePosition.y()) / static_cast<double>(height()),
                      -180.0 * static_cast<double>(newPosition.x() - mousePosition.x()) / static_cast<double>(width()), 0.0);
  }
  else if(selectionType >= SELECTION_BOND && selectionType <= SELECTION_TORSION && e->state() & Qt::LeftButton && e->state() & Qt::ShiftButton && e->state() & Qt::ControlButton)
    ///// LEFTBUTTON + SHIFT + CONTROL + horizontal movement: change selected internal coordinate
    changeSelectedICCommand(e->pos().x() - mousePosition.x());
  else
    GLView::mouseMoveEvent(e); // normal manipulation of entire system

  mousePosition = newPosition;
}

///// keyPressEvent ///////////////////////////////////////////////////////////
void GLMoleculeView::keyPressEvent(QKeyEvent* e)
/// Overridden from GLSimpleMoleculeView::keyPressEvent. Handles key presses for manipulating
/// selections.
/// \arg <ctrl>+<shift>+<left>: change internal coordinate of selection (smaller).
/// \arg <ctrl>+<shift>+<right>: change internal coordinate of selection (larger).
{
  unsigned int selectionType = getSelectionType();
  if(selectionType != SELECTION_NONE && (manipulateSelection || e->state() & Qt::AltButton) && !(e->state() & Qt::ShiftButton && e->state() & Qt::ControlButton))
  {
    switch(e->key())
    {
      case Qt::Key_Left : if(e->state() & Qt::ShiftButton)
                            rotateSelectionCommand(0.0, 0.0, -5.0);
                          else if(e->state() & Qt::ControlButton)
                            translateSelectionCommand(-5, 0, 0);
                          else
                            rotateSelectionCommand(0.0, 5.0, 0.0);
                          break;

      case Qt::Key_Up   : if(e->state() & Qt::ShiftButton)
                            translateSelectionCommand(0, 0, -5);
                          else if(e->state() & Qt::ControlButton)
                            translateSelectionCommand(0, -5, 0);
                          else
                            rotateSelectionCommand(5.0, 0.0, 0.0);
                          break;

      case Qt::Key_Right: if(e->state() & Qt::ShiftButton)
                            rotateSelectionCommand(0.0, 0.0, 5.0);
                          else if(e->state() & Qt::ControlButton)
                            translateSelectionCommand(5, 0, 0);
                          else
                            rotateSelectionCommand(0.0, -5.0, 0.0);
                          break;

      case Qt::Key_Down : if(e->state() & Qt::ShiftButton)
                            translateSelectionCommand(0, 0, 5);
                          else if(e->state() & Qt::ControlButton)
                            translateSelectionCommand(0, 5, 0);
                          else
                            rotateSelectionCommand(-5.0, 0.0, 0.0);
                          break;

      //default:            e->ignore();
      //                    return;
    }
  }
  else if(selectionType >= SELECTION_BOND && selectionType <= SELECTION_TORSION && e->state() & Qt::ShiftButton && e->state() & Qt::ControlButton)
  {
    switch(e->key())
    {
      case Qt::Key_Left : changeSelectedICCommand(-1);
                          break;
      case Qt::Key_Right: changeSelectedICCommand(1);
                          break;
      //default:            e->ignore();
      //                    return;
    }
  }
  else
    GLSimpleMoleculeView::keyPressEvent(e);
}

///// updateShapes ///////////////////////////////////////////////////////////
void GLMoleculeView::updateShapes()
/// Updates the contents of the shapes vector.
/// Overridden from GLSimpleMoleculeView::updateShapes().
{
  GLSimpleMoleculeView::updateShapes(); // first the shapes of the base class

  ShapeProperties prop;

  ///// surfaces
  if(densityDialog != NULL && densityDialog->visualizationType() == DensityBase::ISOSURFACES)
  {
    for(unsigned int i = 0; i < densityGrid->numSurfaces(); i++)
    {
      prop.id = i;
      if(densityDialog->surfaceType(i) == 0) // solid
        prop.opacity = densityDialog->surfaceOpacity(i);
      else if(baseParameters.antialias)
        prop.opacity = 99; // antialiased lines/dots need blending enabled
      else
        prop.opacity = 100;
      prop.type = SHAPE_SURFACE;
      shapes.push_back(prop);
    }
  }
  ///// volumetric rendering
  else if(densityDialog != NULL && densityDialog->visualizationType() == DensityBase::VOLUME)
  {
    prop.id = 0; // not used
    prop.opacity = 0; // always draw the volume last
    prop.type = SHAPE_VOLUME;
    shapes.push_back(prop);
  }
  ///// slices
  else if(densityDialog != NULL && densityDialog->visualizationType() == DensityBase::SLICE)
  {
    prop.id = 0; // not used
    prop.opacity = 0; // always needs blending turned on, regardless of the slice's background setting
    prop.type = SHAPE_SLICE;
    shapes.push_back(prop);
  }
}

///// updateGLSettings ////////////////////////////////////////////////////////
void GLMoleculeView::updateGLSettings()
/// Updates the contents of the shapes vector.
/// Overridden from GLSimpleMoleculeView::updateShapes().
{
  GLSimpleMoleculeView::updateGLSettings(); // update the settings of the base class

  // possibly new texture size and 2D/3D texturing switch
  if(densityDialog != NULL && densityDialog->visualizationType() == DensityBase::VOLUME)
    updateVolume();
  else if(densityDialog != NULL && densityDialog->visualizationType() == DensityBase::SLICE)
    updateSlice();
}

///// processSelectionCommand /////////////////////////////////////////////////
void GLMoleculeView::processSelectionCommand(const unsigned int id)
/// Creates a Command to call processSelection. Overridden from
/// GLSimpleMoleculeView::processSelectionCommand.
{
  XbraboView* view = (XbraboView*)(parentWidget()->parentWidget());
  view->getCommandHistory()->addCommand(new CommandSelectEntity(view, "Change selection", id));
}

///// translateCommand ////////////////////////////////////////////////////////
void GLMoleculeView::translateCommand(const int amountX, const int amountY, const int amountZ)
/// Creates a Command to translate the scene in the X, Y or Z direction.
/// Overridden from GLView::translateCommand.
{
  XbraboView* view = (XbraboView*)(parentWidget()->parentWidget());
  if(amountZ != 0)
    view->getCommandHistory()->addCommand(new CommandTranslateZ(view, "Zoom", amountZ));
  else
    view->getCommandHistory()->addCommand(new CommandTranslateXY(view, "Translate", amountX, amountY));
}

///// rotateCommand ///////////////////////////////////////////////////////////
void GLMoleculeView::rotateCommand(const float amountX, const float amountY, const float amountZ)
/// Creates a Command to rotate the scene in the X, Y or Z direction.
/// Overridden from GLView::rotateCommand.
{
  XbraboView* view = (XbraboView*)(parentWidget()->parentWidget());
  view->getCommandHistory()->addCommand(new CommandRotate(view, "Rotate", amountX, amountY, amountZ));
}

///// translateSelectionCommand /////////////////////////////////////////////////////////
void GLMoleculeView::translateSelectionCommand(const int amountX, const int amountY, const int amountZ)
/// Creates a Command to translate the selected atoms in the X, Y or Z direction.
{
  XbraboView* view = (XbraboView*)(parentWidget()->parentWidget());
  if(amountZ != 0)
    view->getCommandHistory()->addCommand(new CommandTranslateSelectionZ(view, "Zoom Selection", amountZ));
  else
    view->getCommandHistory()->addCommand(new CommandTranslateSelectionXY(view, "Translate Selection", amountX, amountY));
}

///// rotateSelectionCommand //////////////////////////////////////////////////
void GLMoleculeView::rotateSelectionCommand(const double amountX, const double amountY, const double amountZ)
/// Creates a Command to rotate the selected atoms in the X, Y or Z direction.
{
  XbraboView* view = (XbraboView*)(parentWidget()->parentWidget());
  view->getCommandHistory()->addCommand(new CommandRotateSelection(view, "Rotate Selection", amountX, amountY, amountZ));
}

///// changeSelectedICCommand /////////////////////////////////////////////////
void GLMoleculeView::changeSelectedICCommand(const int range)
/// Creates a Command to change the selected IC.
{
  XbraboView* view = (XbraboView*)(parentWidget()->parentWidget());
  view->getCommandHistory()->addCommand(new CommandChangeIC(view, "Change Internal Coordinate", range));
}


///////////////////////////////////////////////////////////////////////////////
///// Private Slots                                                       /////
///////////////////////////////////////////////////////////////////////////////

///// addGLSurface ////////////////////////////////////////////////////////////
void GLMoleculeView::addGLSurface(const unsigned int index)
/// Creates a display list for a new surface.
{
  ///// generate a new display list and save it
  makeCurrent();
  GLuint newList = glGenLists(1);
  glSurfaces.push_back(newList);

  ///// if this is the only surface and no atoms are present: zoomFit
  if(glSurfaces.size() == 1 && atoms->count() == 0)
    zoomFit(false);

  qDebug("creating surface %d", index);
  ///// populate the display list
  updateGLSurface(index);
}

///// updateGLSurface /////////////////////////////////////////////////////////
void GLMoleculeView::updateGLSurface(const unsigned int index)
/// Updates the display list for an existing surface.
{
  makeCurrent();
  QColor surfaceColor = densityDialog->surfaceColor(index);
  bool usesMapping = densityDialog->surfaceMapping();
  unsigned int surfaceOpacity = densityDialog->surfaceOpacity(index);
  Point3D<float> point1, point2, point3, normal1, normal2, normal3;
  QColor color1, color2, color3;

  //qDebug("updating surface %d", index);
  //qDebug(" which consists of %d vertices and %d triangles",densityGrid->numVertices(index),densityGrid->numTriangles(index));
  //qDebug(" with color %d, %d, %d and opacity %d", surfaceColor.red(), surfaceColor.green(), surfaceColor.blue(), surfaceOpacity);
  glNewList(glSurfaces[index], GL_COMPILE);
    switch(densityDialog->surfaceType(index))
    {
      case 0: // Solid surface
        glBegin(GL_TRIANGLES);
          if(!usesMapping)
            glColor4d(surfaceColor.red()/255.0, surfaceColor.green()/255.0, surfaceColor.blue()/255, surfaceOpacity/100.0);
	        for(unsigned int i = 0; i < densityGrid->numTriangles(index); i++)
	        {
            densityGrid->getTriangle(index, i, point1, point2, point3, normal1, normal2, normal3);
            if(usesMapping)
            {
              color1 = densityGrid->getMappingColor(point1);
              glColor4d(color1.red()/255.0, color1.green()/255.0, color1.blue()/255.0, surfaceOpacity/100.0);
            }
    		    glNormal3f(normal1.x(), normal1.y(), normal1.z());
            glVertex3f(point1.x(), point1.y(), point1.z());
            if(usesMapping)
            {
              color1 = densityGrid->getMappingColor(point2);
              glColor4d(color1.red()/255.0, color1.green()/255.0, color1.blue()/255.0, surfaceOpacity/100.0);
            }
		        glNormal3f(normal2.x(), normal2.y(), normal2.z());
            glVertex3f(point2.x(), point2.y(), point2.z());
            if(usesMapping)
            {
              color1 = densityGrid->getMappingColor(point3);
              glColor4d(color1.red()/255.0, color1.green()/255.0, color1.blue()/255.0, surfaceOpacity/100.0);
            }
		        glNormal3f(normal3.x(), normal3.y(), normal3.z());
            glVertex3f(point3.x(), point3.y(), point3.z());
	        }
        glEnd();
        break;
      case 1: // Wireframe
        //glLineWidth(1.0);
        {
          double lw, ps;
          glGetDoublev(GL_LINE_WIDTH, &lw);
          glGetDoublev(GL_POINT_SIZE, &ps);
          qDebug("linewidth and pointsize used for generating: %f and %f", lw, ps);
        }
        glBegin(GL_LINES);
          if(!usesMapping)
            qglColor(surfaceColor);
	        for(unsigned int i = 0; i < densityGrid->numTriangles(index); i++)
	        {
            densityGrid->getTriangle(index, i, point1, point2, point3, normal1, normal2, normal3);
            if(usesMapping)
            {
              color1 = densityGrid->getMappingColor(point1);
              color2 = densityGrid->getMappingColor(point2);
              color3 = densityGrid->getMappingColor(point3);
              qglColor(color1);
            }
            glVertex3f(point1.x(), point1.y(), point1.z());
            if(usesMapping)
              qglColor(color2);
		        glVertex3f(point2.x(), point2.y(), point2.z());
            if(usesMapping)
              qglColor(color1);
            glVertex3f(point1.x(), point1.y(), point1.z());
            if(usesMapping)
              qglColor(color3);
            glVertex3f(point3.x(), point3.y(), point3.z());
            if(usesMapping)
              qglColor(color2);
		        glVertex3f(point2.x(), point2.y(), point2.z());
            if(usesMapping)
              qglColor(color3);
            glVertex3f(point3.x(), point3.y(), point3.z());
	        }
        glEnd();
        break;
      case 2: // Dots
        glPointSize(1.0);
        glBegin(GL_POINTS);
          if(!usesMapping)
            qglColor(surfaceColor);
	        for(unsigned int i = 0; i < densityGrid->numVertices(index); i++)
	        {
            point1 = densityGrid->getPoint(index, i);
            if(usesMapping)
              qglColor(densityGrid->getMappingColor(point1));
            glVertex3f(point1.x(), point1.y(), point1.z());
          }
        glEnd();
    }
  glEndList();
  reorderShapes();
}

///// deleteGLSurface /////////////////////////////////////////////////////////
void GLMoleculeView::deleteGLSurface(const unsigned int index)
/// Deletes the display list for an existing surface.
{
  makeCurrent();
  glDeleteLists(glSurfaces[index], 1);
  std::vector<GLuint>::iterator it = glSurfaces.begin();
  it += index;
  glSurfaces.erase(it);
  reorderShapes();
}

///// updateScene /////////////////////////////////////////////////////////////
void GLMoleculeView::updateScene()
/// Does the necessary updating when something changed in DensityBase (like e.g.
/// the visualization type.
{
  makeCurrent();
  reorderShapes();

  // set the correct texturing mode for volume rendering and slices
  switch(densityDialog->ComboBoxVisualizationType->currentItem())
  {
    case DensityBase::VOLUME: glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); // transparent textures
                              break;
    case DensityBase::SLICE:  if(densityDialog->PushButtonSingleColor->isOn() && densityDialog->CheckBoxSliceTransparent->isChecked())
                                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE); // transparent textures
                              else
                                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL); // textures blended with the current background color
  }

  updateGL();
}

///// updateVolume ////////////////////////////////////////////////////////////
void GLMoleculeView::updateVolume()
/// Recalculates the stack of slices for viewing the volumetric rendering of
/// the active density grid or updates the 3D texture depending on the settings.
{
  qDebug("calling updateVolume");
  if(textureParameters.use3DTextures)
    updateVolume3D();
  else
    updateVolume2D();
}

///// updateSlice /////////////////////////////////////////////////////////////
void GLMoleculeView::updateSlice()
/// Updates the active slice to be displayed
{
  if(densityDialog->ComboBoxVisualizationType->currentItem() != DensityBase::SLICE)
    return;

  makeCurrent();

  // densityGrid data
  Point3D<float> origin = densityGrid->getOrigin();
  Point3D<float> delta = densityGrid->getDelta();
  // visualization settings from DensityBase
  QColor positiveColor = densityDialog->ColorButtonSlicePos->color();
  QColor negativeColor = densityDialog->ColorButtonSliceNeg->color();
  double maxPlotValue = densityDialog->LineEditSlicePos->text().stripWhiteSpace().toDouble();
  double minPlotValue = densityDialog->LineEditSliceNeg->text().stripWhiteSpace().toDouble();
  unsigned int colorMap = densityDialog->ComboBoxSliceMap->currentItem();
  if(densityDialog->PushButtonSingleColor->isOn())
    colorMap = DensityGrid::MAP_LAST; // no mapping
  GLuint textureID;


  qglColor(densityDialog->ColorButtonSliceBack->color());

  if(sliceObject == 0)
    sliceObject = glGenLists(1); // can't create it in the constructor

  const unsigned int index = densityDialog->SliderSlice->value();
  const Point3D<unsigned int> numPoints = densityGrid->getNumPoints();
  if(index < numPoints.x())
  {
    qDebug("calling updateSlice for YZ-plane (varying X)");
    // a slice in the YZ-plane
    const unsigned int x = index;
    QImage image = densityGrid->getSlice(DensityGrid::PLANE_YZ, x, positiveColor, negativeColor, maxPlotValue, minPlotValue, colorMap);
    QImage glImage = glSlice(image);

    // create a texture from it
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glImage.width(), glImage.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, glImage.bits());

    // create a display list
    glNewList(sliceObject, GL_COMPILE);
      // the texture
      glBindTexture(GL_TEXTURE_2D, textureID);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(origin.x() + x * delta.x(), origin.y(),                                 origin.z());
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(origin.x() + x * delta.x(), origin.y() + (numPoints.y()-1) * delta.y(), origin.z());
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(origin.x() + x * delta.x(), origin.y() + (numPoints.y()-1) * delta.y(), origin.z() + (numPoints.z()-1) * delta.z());
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(origin.x() + x * delta.x(), origin.y(),                                 origin.z() + (numPoints.z()-1) * delta.z());
      glEnd();

      // a rectangle in case of transparent textures
      if(densityDialog->PushButtonSingleColor->isOn() && densityDialog->CheckBoxSliceTransparent->isChecked())
      {
        qglColor(densityDialog->ColorButtonSliceBack->color());
        glBindTexture(GL_TEXTURE_2D, 0); // don't use texturing for this
        glBegin(GL_LINE_LOOP);
          glVertex3f(origin.x() + x * delta.x(), origin.y(),                                 origin.z());
          glVertex3f(origin.x() + x * delta.x(), origin.y() + (numPoints.y()-1) * delta.y(), origin.z());
          glVertex3f(origin.x() + x * delta.x(), origin.y() + (numPoints.y()-1) * delta.y(), origin.z() + (numPoints.z()-1) * delta.z());
          glVertex3f(origin.x() + x * delta.x(), origin.y(),                                 origin.z() + (numPoints.z()-1) * delta.z());
        glEnd();
      }
    glEndList();
  }
  else if(index < numPoints.x() + numPoints.y())
  {
    qDebug("calling updateSlice for XZ-plane (varying Y)");
    // a slice in the XZ-plane
    unsigned int y = index - numPoints.x();
    QImage image = densityGrid->getSlice(DensityGrid::PLANE_XZ, y, positiveColor, negativeColor, maxPlotValue, minPlotValue, colorMap);
    QImage glImage = glSlice(image);

    // create a texture from it
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glImage.width(), glImage.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, glImage.bits());

    // create a display list
    glNewList(sliceObject, GL_COMPILE);
      glBindTexture(GL_TEXTURE_2D, textureID);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(origin.x(),                                 origin.y() + y * delta.y(), origin.z());
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(origin.x(),                                 origin.y() + y * delta.y(), origin.z() + (numPoints.z()-1) * delta.z());
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(origin.x() + (numPoints.x()-1) * delta.x(), origin.y() + y * delta.y(), origin.z() + (numPoints.z()-1) * delta.z());
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(origin.x() + (numPoints.x()-1) * delta.x(), origin.y() + y * delta.y(), origin.z());
      glEnd();
      // a rectangle in case of transparent textures
      if(densityDialog->PushButtonSingleColor->isOn() && densityDialog->CheckBoxSliceTransparent->isChecked())
      {
        qglColor(densityDialog->ColorButtonSliceBack->color());
        glBindTexture(GL_TEXTURE_2D, 0); // don't use texturing for this
        glBegin(GL_LINE_LOOP);
          glVertex3f(origin.x(),                                 origin.y() + y * delta.y(), origin.z());
          glVertex3f(origin.x(),                                 origin.y() + y * delta.y(), origin.z() + (numPoints.z()-1) * delta.z());
          glVertex3f(origin.x() + (numPoints.x()-1) * delta.x(), origin.y() + y * delta.y(), origin.z() + (numPoints.z()-1) * delta.z());
          glVertex3f(origin.x() + (numPoints.x()-1) * delta.x(), origin.y() + y * delta.y(), origin.z());
        glEnd();
      }
    glEndList();
  }
  else
  {
    qDebug("calling updateSlice for XY-plane (varying Z)");
    // a slice in the XY-plane
    unsigned int z = index - numPoints.x() - numPoints.y();
    QImage image = densityGrid->getSlice(DensityGrid::PLANE_XY, z, positiveColor, negativeColor, maxPlotValue, minPlotValue, colorMap);
    QImage glImage = glSlice(image);

    // create a texture from it
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glImage.width(), glImage.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, glImage.bits());

    // create a display list
    glNewList(sliceObject, GL_COMPILE);
      glBindTexture(GL_TEXTURE_2D, textureID);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(origin.x(),                                 origin.y(),                                 origin.z() + z * delta.z());
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(origin.x() + (numPoints.x()-1) * delta.x(), origin.y(),                                 origin.z() + z * delta.z());
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(origin.x() + (numPoints.x()-1) * delta.x(), origin.y() + (numPoints.y()-1) * delta.y(), origin.z() + z * delta.z());
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(origin.x(),                                 origin.y() + (numPoints.y()-1) * delta.y(), origin.z() + z * delta.z());
      glEnd();
      // a rectangle in case of transparent textures
      if(densityDialog->PushButtonSingleColor->isOn() && densityDialog->CheckBoxSliceTransparent->isChecked())
      {
        qglColor(densityDialog->ColorButtonSliceBack->color());
        glBindTexture(GL_TEXTURE_2D, 0); // don't use texturing for this
        glBegin(GL_LINE_LOOP);
          glVertex3f(origin.x(),                                 origin.y(),                                 origin.z() + z * delta.z());
          glVertex3f(origin.x() + (numPoints.x()-1) * delta.x(), origin.y(),                                 origin.z() + z * delta.z());
          glVertex3f(origin.x() + (numPoints.x()-1) * delta.x(), origin.y() + (numPoints.y()-1) * delta.y(), origin.z() + z * delta.z());
          glVertex3f(origin.x(),                                 origin.y() + (numPoints.y()-1) * delta.y(), origin.z() + z * delta.z());
        glEnd();
      }
    glEndList();
  }

  reorderShapes();
}


///////////////////////////////////////////////////////////////////////////////
///// Private Member Functions                                            /////
///////////////////////////////////////////////////////////////////////////////

///// translateSelection //////////////////////////////////////////////////////
bool GLMoleculeView::translateSelection(const int xRange, const int yRange, const int zRange)
/// Translates the selected atoms according to the screen.
{
  if(selectionList.empty())
    return false;

  makeCurrent();
  // needed variables
  GLdouble modelview[16];
  GLdouble projection[16];
  GLint viewport[4];

  // set up the modelview matrix to be the same as in paintGL
  Vector3D<float> axis;
  float angle;
  orientationQuaternion->getAxisAngle(axis, angle);
  glPushMatrix();
  glTranslatef(xPos, yPos, 0.0f);
  glRotatef(angle, axis.x(), axis.y(), axis.z());
  glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
  glPopMatrix();

  // get the other matrices
  glGetDoublev(GL_PROJECTION_MATRIX, projection);
  glGetIntegerv(GL_VIEWPORT, viewport);

  ///// project and unproject the first selected atom (maybe change this tothe center of mass for larger selections...)
  GLdouble x, y, z, xwin, ywin, zwin;
  std::list<unsigned int>::iterator it = selectionList.begin();

  // get the data
  x = atoms->x(*it);
  y = atoms->y(*it);
  z = atoms->z(*it);

  // map the atom coordinates to the window coordinates with gluProject
  if(gluProject(x, y, z, modelview, projection, viewport, &xwin, &ywin, &zwin) == GL_FALSE)
    return false;

  // do the translation in window coordinates
  xwin += static_cast<double>(xRange);
  ywin -= static_cast<double>(yRange); // OpenGL coordinate system inverts the y-axis
  if(baseParameters.perspectiveProjection)
    zwin += static_cast<double>(zRange)/10000.0; // (10000 = far clipping distance/near clipping distance)
  else
    zwin += static_cast<double>(zRange)/100.0;
    //zwin += static_cast<double>(zRange)/(2.0f*boundingSphereRadius()); // is very slow!!!

  // map the new window coordinates to the atom coordinates with gluUnproject
  if(gluUnProject(xwin, ywin, zwin, modelview, projection, viewport, &x, &y, &z) == GL_FALSE)
    return false;

  // determine the translation vector
  const double dx = x - atoms->x(*it);
  const double dy = y - atoms->y(*it);
  const double dz = z - atoms->z(*it);

  // apply this translation vector to all selected atoms
  while(it != selectionList.end())
  {
    atoms->setX(*it, atoms->x(*it) + dx);
    atoms->setY(*it, atoms->y(*it) + dy);
    atoms->setZ(*it, atoms->z(*it) + dz);
    it++;
  }

  updateAtomSet();
  setModified();
  return true;
}

///// rotateSelection /////////////////////////////////////////////////////////
bool GLMoleculeView::rotateSelection(const double angleX, const double angleY, const double angleZ)
/// Rotates the selected atoms around their local center of mass.
{
  if(selectionList.empty())
    return false;

  ///// determine the axis around which to rotate all atoms + the amount of rotation
  ///// first the system should be backrotated from the scene's rotation (orientationQuaternion)
  ///// then the system should be rotated as desired
  ///// finally the system should be rotated to the scene's rotation again
  //* original version working with 3 successive axis/angle rotations
  Vector3D<double> axis;
  double angle;
  Quaternion<double> q(angleX, angleY, angleZ);
  q.getAxisAngle(axis, angle);
  Vector3D<float> axis2;
  float angle2;
  orientationQuaternion->getAxisAngle(axis2, angle2);
  double backAngle = - angle2;
  Vector3D<double> backAxis(axis2.x(), axis2.y(), axis2.z());
  // */
  /* construct the desired quaternions in double precision -> doesn't seem to work correctly
  Quaternion<double> orientation(static_cast<double>(orientationQuaternion->w()), static_cast<double>(orientationQuaternion->x()),
                               static_cast<double>(orientationQuaternion->y()), static_cast<double>(orientationQuaternion->z()));
  Quaternion<double> backOrientation(orientation);
  backOrientation.inverse();
  Quaternion<double> rotation(angleX, angleY, angleZ);
  Quaternion<double> totalRotation = backOrientation * rotation * orientation;
  // get the axis/angle representation
  Vector3D<double> axis;
  double angle;
  totalRotation.getAxisAngle(axis, angle);
  */
  if(fabs(angle) < Point3D<double>::TOLERANCE)
    return false;

  ///// determine the local center of mass (all masses are taken equal)
  Point3D<double> centerOfMass(0.0, 0.0, 0.0);
  std::list<unsigned int>::iterator it = selectionList.begin();
  while(it != selectionList.end())
    centerOfMass.add(atoms->coordinates(*it++));
  centerOfMass.setValues(centerOfMass.x()/selectionList.size(), centerOfMass.y()/selectionList.size(), centerOfMass.z()/selectionList.size());
  //qDebug("centerOfMass = %f, %f, %f", centerOfMass.x(), centerOfMass.y(), centerOfMass.z());

  ///// rotate the atoms around this center
  it = selectionList.begin();
  while(it != selectionList.end())
  {
    Vector3D<double> v(centerOfMass, atoms->coordinates(*it));
    v.rotate(backAxis, backAngle);
    v.rotate(axis, angle);
    v.rotate(backAxis, -backAngle);
    atoms->setX(*it, centerOfMass.x() + v.x());
    atoms->setY(*it, centerOfMass.y() + v.y());
    atoms->setZ(*it, centerOfMass.z() + v.z());
    it++;
  }
  ///// TEMP HACK: if all atoms are selected, also rotate the point charges
  if(selectionList.size() == atoms->count())
  {
    vector<Point3D<double> > newPCcoords;
    vector<double> newPCcharges;
    newPCcoords.reserve(atoms->countPointCharges());
    newPCcharges.reserve(atoms->countPointCharges());
    for(unsigned int i = 0; i < atoms->countPointCharges(); i++)
    {
      Vector3D<double> v(centerOfMass, atoms->pointChargeCoordinates(i));
      v.rotate(backAxis, backAngle);
      v.rotate(axis, angle);
      v.rotate(backAxis, -backAngle);
      Point3D<double> point(centerOfMass.x() + v.x(), centerOfMass.y() + v.y(), centerOfMass.z() + v.z());
      point.setID(atoms->pointChargeCoordinates(i).id());
      newPCcoords.push_back(point);
      newPCcharges.push_back(atoms->pointCharge(i));
    }
    atoms->removePointCharges();
    vector<Point3D<double> >::iterator it1 = newPCcoords.begin();
    vector<double>::iterator it2 = newPCcharges.begin();
    for(; it1 != newPCcoords.end(); it1++, it2++)
      atoms->addPointCharge((*it1).x(), (*it1).y(), (*it1).z(), *it2, (*it1).id());
  }
  updateAtomSet();
  setModified();
  return true;
}

///// changeSelectedIC ////////////////////////////////////////////////////////
bool GLMoleculeView::changeSelectedIC(const int range)
/// Changes the selected internal coordinate
/// according to the magnitude and direction of the range.
{
  if(range == 0)
    return false;

  unsigned int atom1, atom2, atom3, atom4;
  std::list<unsigned int>::iterator it = selectionList.begin();
  switch(getSelectionType())
  {
    case SELECTION_BOND:    atom1 = *it++;
                            atom2 = *it;
                            atoms->changeBond(static_cast<double>(range) * 0.1, atom1, atom2, true);
                            break;

    case SELECTION_ANGLE:   atom1 = *it++;
                            atom2 = *it++;
                            atom3 = *it;
                            atoms->changeAngle(180.0 * static_cast<double>(range) / static_cast<double>(width()), atom1, atom2, atom3, true);
                            break;
    case SELECTION_TORSION: atom1 = *it++;
                            atom2 = *it++;
                            atom3 = *it++;
                            atom4 = *it;
                            atoms->changeTorsion(-180.0 * static_cast<double>(range) / static_cast<double>(width()), atom1, atom2, atom3, atom4, true);
                            break;
  }
  updateAtomSet();
  setModified();
  return true;
}

///// drawItem ////////////////////////////////////////////////////////////////
void GLMoleculeView::drawItem(const unsigned int index)
/// Draws the item shapes[index].
{
  switch(shapes[index].type)
  {
    case SHAPE_SURFACE: drawSurface(index);
                        break;
    case SHAPE_VOLUME:  drawVolume();
                        break;
    case SHAPE_SLICE:   drawSlice();
  }
}

///// drawSurface /////////////////////////////////////////////////////////////
void GLMoleculeView::drawSurface(const unsigned int index)
/// Draws the surface shapes[index].
{
  assert(shapes[index].type == SHAPE_SURFACE);
  assert(shapes[index].id < densityGrid->numSurfaces());

  const unsigned int currentSurface = shapes[index].id;
  if(densityDialog->surfaceVisible(currentSurface))
  {
    if(densityDialog->surfaceType(currentSurface) == 0) // solid
      glCallList(glSurfaces[currentSurface]);
    else // wireframe or dots
    {
      glDisable(GL_LIGHTING);
      glCallList(glSurfaces[currentSurface]);
      glEnable(GL_LIGHTING);
    }
  }
}

///// drawVolume //////////////////////////////////////////////////////////////
void GLMoleculeView::drawVolume()
/// Draws a density grid with volumetric rendering.
{
  glColor3f(1.0f, 1.0f, 1.0f); // needed so the colors are blended with white
  glDisable(GL_LIGHTING);

  if(textureParameters.use3DTextures)
    drawVolume3D();
  else
    drawVolume2D();

  glEnable(GL_LIGHTING);
}

///// drawVolume2D ////////////////////////////////////////////////////////////
void GLMoleculeView::drawVolume2D()
/// Draws a density grid with volumetric rendering using a stack of 2D textures
{
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_CULL_FACE); // now the backsides can be visible

  ///// The display lists are updated so now just call them from back to front
  Point3D<unsigned int> numPoints = densityGrid->getNumPoints();
  switch(getDirection())
  {
    case DIRECTION_POSX: for(unsigned int x = 0; x < numPoints.x(); x++)
                           glCallList(volumeObjects + x);
                         break;

    case DIRECTION_NEGX: for(unsigned int x = 0; x < numPoints.x(); x++)
                           glCallList(volumeObjects + numPoints.x()-1 - x);
                         break;

    case DIRECTION_POSY: for(unsigned int y = 0; y < numPoints.y(); y++)
                           glCallList(volumeObjects + numPoints.x() + y);
                         break;

    case DIRECTION_NEGY: for(unsigned int y = 0; y < numPoints.y(); y++)
                           glCallList(volumeObjects + numPoints.x() + numPoints.y()-1 - y);
                         break;

    case DIRECTION_POSZ: for(unsigned int z = 0; z < numPoints.z(); z++)
                           glCallList(volumeObjects + numPoints.x() + numPoints.y() + z);
                         break;

    case DIRECTION_NEGZ: for(unsigned int z = 0; z < numPoints.z(); z++)
                           glCallList(volumeObjects + numPoints.x() + numPoints.y() + numPoints.z()
                           -1 - z);
                         break;
  }
  glDisable(GL_TEXTURE_2D);
  glEnable(GL_CULL_FACE);

  ///// New technique for volume rendering: each gridelement is rendered as a quad
  ///// oriented in the XY-plane the size of the gridelement (delta.x() * delta.y())
  ///// with a transparency value related to the density value.
  ///// On rotation all quads are rotated around their own axes to always face the viewer.
  ///// => first attempt is waaay too slow (less than 2 FPS) but kinda works (still needs Z-ordering)
  /*
  std::vector<double>* values = &(densityDialog->densityPointsA); // the grid
  Point3D<unsigned int> numPoints = densityGrid->getNumPoints();
  double minDensity = densityGrid->getMinimumDensity();
  double maxDensity = densityGrid->getMaximumDensity();
  Point3D<float> origin = densityGrid->getOrigin();
  Point3D<float> delta = densityGrid->getDelta();
  double opacity;

  Vector3D<float> axis;
  float angle;
  orientationQuaternion->getAxisAngle(axis, angle);

  std::vector<double>::iterator itValues = values->begin();
  glDisable(GL_LIGHTING);
  for(unsigned int x = 0; x < numPoints.x(); ++x)
  {
    for(unsigned int y = 0; y < numPoints.y(); ++y)
    {
      for(unsigned int z = 0; z < numPoints.z(); ++z)
      {
        // position
        glPushMatrix();
        glTranslatef(origin.x() + x*delta.x(), origin.y() + y*delta.y(), origin.z() + z*delta.z());
        glRotatef(-angle, axis.x(), axis.y(), axis.z()); // backrotation

        // set the color
        if((*itValues) > 0.0)
        {
          opacity = *itValues/maxDensity;
          if(opacity > 1.0)
            opacity = 1.0;
          glColor4d(0.0, 0.0, 1.0, opacity);
        }
        else
        {
          opacity = *itValues/minDensity;
          if(opacity > 1.0)
            opacity = 1.0;
          glColor4d(1.0, 0.0, 0.0, opacity);
        }

        // draw the quad
        glBegin(GL_QUADS);
          glVertex3f(0.0f, 0.0f, 0.0f);
          glVertex3f(1.0f, 0.0f, 0.0f);
          glVertex3f(1.0f, 1.0f, 0.0f);
          glVertex3f(0.0f, 1.0f, 0.0f);
        glEnd();
        ++itValues;
        glPopMatrix();
      }
    }
  }
  glEnable(GL_LIGHTING);
  return;
  */

  ///// Yet another technique for volume rendering: each gridelement is rendered
  ///// by 3 quads: the bottom XZ-plane, the left YZ-plane and the back XY-plane.
  ///// This doesn't need back-rotation of the voxels and doesn't produce any gaps.
  ///// => also very slow and doesn't really work atm due to missing ordering
  /*
  std::vector<double>* values = &(densityDialog->densityPointsA); // the grid
  Point3D<unsigned int> numPoints = densityGrid->getNumPoints();
  double minDensity = densityGrid->getMinimumDensity();
  double maxDensity = densityGrid->getMaximumDensity();
  Point3D<float> origin = densityGrid->getOrigin();
  Point3D<float> delta = densityGrid->getDelta();
  double opacity;

  std::vector<double>::iterator itValues = values->begin();
  glDisable(GL_LIGHTING);
  double posx = origin.x(), posy, posz;
  for(unsigned int x = 0; x < numPoints.x(); ++x)
  {
    posy = origin.y();
    for(unsigned int y = 0; y < numPoints.y(); ++y)
    {
      posz = origin.z();
      for(unsigned int z = 0; z < numPoints.z(); ++z)
      {
        // set the color
        if((*itValues) > 0.0)
        {
          opacity = *itValues/maxDensity;
          if(opacity > 1.0)
            opacity = 1.0;
          opacity = 1.0;
          glColor4d(0.0, 0.0, 1.0, opacity);
        }
        else
        {
          opacity = *itValues/minDensity;
          if(opacity > 1.0)
            opacity = 1.0;
          opacity = 1.0;
          glColor4d(1.0, 0.0, 0.0, opacity);
        }

        glBegin(GL_QUADS);
          // draw the XY-quad
          glVertex3f(posx,             posy,             posz);
          glVertex3f(posx + delta.x(), posy,             posz);
          glVertex3f(posx + delta.x(), posy + delta.y(), posz);
          glVertex3f(posx,             posy + delta.y(), posz);
          // draw the XZ-quad
          glVertex3f(posx,             posy, posz);
          glVertex3f(posx + delta.x(), posy, posz);
          glVertex3f(posx + delta.x(), posy, posz + delta.z());
          glVertex3f(posx,             posy, posz + delta.z());
          // draw the YZ-quad
          glVertex3f(posx, posy,             posz);
          glVertex3f(posx, posy + delta.y(), posz);
          glVertex3f(posx, posy + delta.y(), posz + delta.z());
          glVertex3f(posx, posy,             posz + delta.z());
        glEnd();
        ++itValues;
        glPopMatrix();
        posz += delta.z();
      }
      posy += delta.y();
    }
    posx += delta.x();
  }
  glEnable(GL_LIGHTING);
  return;
  */

  /*
  ///// And another technique for volume rendering, based on the current implementation.
  ///// All sets of textures are shown but are given a global alpha value depending
  ///// on the amount of orientation into the viewing direction.
  ///// => gives exactly the same problems when crossing boundaries and is a bit slower
  /////    it also fades too much along the boundaries

  // first get the coincidence of the primary axes with the viewing direction
  // (similar to getDirection()) -> get the angles

  // get the axis/angle representation of the current scene orientation
  Vector3D<float> orientationVector, axis;
  float orientationAngle;
  orientationQuaternion->getAxisAngle(orientationVector, orientationAngle);
  orientationAngle = -orientationAngle; // we need opposite rotation from the OpenGL one.

  // the view vector (not needed anymore because of the shortcut in calculating the dot product,
  // namely a dot product with (0, 0, 1) is always equal to the Z-component of the other argument)
  //Vector3D<float> view(0.0f, 0.0f, 1.0f); // we're looking into the negative Z-direction so the normal
                                          // of the plane we want to see points towards us into the positive Z-direction

  // rotate a unit vector along the positive X-axis
  axis.setValues(1.0f, 0.0f, 0.0f);
  axis.rotate(orientationVector, orientationAngle);
  // get the dot product with the view vector
  // as the view vector is (0, 0, 1), the dot product is always equal to the Z-component of the axis
  float dotX = axis.z(); // = axis.dot(view); with view = Vector3D(0.0f, 0.0f, 1.0f);

  // rotate a unit vector along the positive Y-axis
  axis.setValues(0.0f, 1.0f, 0.0f);
  axis.rotate(orientationVector, orientationAngle);
  // get the dot product with the view vector
  float dotY = axis.z();

  // rotate a unit vector along the positive Z-axis
  axis.setValues(0.0f, 0.0f, 1.0f);
  axis.rotate(orientationVector, orientationAngle);
  // get the dot product with the view vector
  float dotZ = axis.z();

  // the dot products are cosines of the angles with the viewing vector
  float xAngle = Point3D<float>::PI/2.0f - acos(fabs(dotX));
  float yAngle = Point3D<float>::PI/2.0f - acos(fabs(dotY));
  float zAngle = Point3D<float>::PI/2.0f - acos(fabs(dotZ));
  qDebug("dots:   %f, %f, %f", dotX, dotY, dotZ);
  qDebug("angles: %f, %f, %f (sum = %f)", xAngle*180.0f/Point3D<float>::PI, yAngle*180.0f/Point3D<float>::PI, zAngle*180.0f/Point3D<float>::PI, (xAngle+yAngle+zAngle)*180.0f/Point3D<float>::PI);

  float xAlpha = fabs(xAngle)/(xAngle+yAngle+zAngle); // an angle of zero is fully aligned
  float yAlpha = fabs(yAngle)/(xAngle+yAngle+zAngle);
  float zAlpha = fabs(zAngle)/(xAngle+yAngle+zAngle);

  ///// The display lists are updated so now just call them from back to front
  glDisable(GL_LIGHTING);
  glDisable(GL_CULL_FACE); // now the backsides can be visible
  //glDepthMask(GL_FALSE);

  unsigned int numX = densityGrid->getNumPoints().x(), numY = densityGrid->getNumPoints().y(), numZ = densityGrid->getNumPoints().z();

  glColor3f(1.0f, 1.0f, 1.0f);
  if(xAlpha > yAlpha && xAlpha > zAlpha)
  {
    glColor4f(1.0f, 1.0f, 1.0f, xAlpha);
    if(dotX > 0.0f)
    {
      for(unsigned int x = 0; x < numX; x++)
        glCallList(volumeObjects + x);
    }
    else
    {
      for(unsigned int x = 0; x < numX; x++)
        glCallList(volumeObjects + numX-1 - x);
    }

    if(yAlpha > zAlpha)
    {
      glColor4f(1.0f, 1.0f, 1.0f, yAlpha);
      if(dotY > 0.0f)
      {
        for(unsigned int y = 0; y < numY; y++)
          glCallList(volumeObjects + numX + y);
      }
      else
      {
        for(unsigned int y = 0; y < numY; y++)
          glCallList(volumeObjects + numX + numY-1 - y);
      }
      glColor4f(1.0f, 1.0f, 1.0f, zAlpha);
      if(dotZ > 0.0f)
      {
        for(unsigned int z = 0; z < numZ; z++)
          glCallList(volumeObjects + numX + numY + z);
      }
      else
      {
        for(unsigned int z = 0; z < numZ; z++)
          glCallList(volumeObjects + numX + numY + numZ-1 - z);
      }
    }
    else
    {
      glColor4f(1.0f, 1.0f, 1.0f, zAlpha);
      if(dotZ > 0.0f)
      {
        for(unsigned int z = 0; z < numZ; z++)
          glCallList(volumeObjects + numX + numY + z);
      }
      else
      {
        for(unsigned int z = 0; z < numZ; z++)
          glCallList(volumeObjects + numX + numY + numZ-1 - z);
      }
      glColor4f(1.0f, 1.0f, 1.0f, yAlpha);
      if(dotY > 0.0f)
      {
        for(unsigned int y = 0; y < numY; y++)
          glCallList(volumeObjects + numX + y);
      }
      else
      {
        for(unsigned int y = 0; y < numY; y++)
          glCallList(volumeObjects + numX + numY-1 - y);
      }
    }
  }
  else if(yAlpha > xAlpha && yAlpha > zAlpha)
  {
    glColor4f(1.0f, 1.0f, 1.0f, yAlpha);
    if(dotY > 0.0f)
    {
      for(unsigned int y = 0; y < numY; y++)
        glCallList(volumeObjects + numX + y);
    }
    else
    {
      for(unsigned int y = 0; y < numY; y++)
        glCallList(volumeObjects + numX + numY-1 - y);
    }

    if(xAlpha > zAlpha)
    {
      glColor4f(1.0f, 1.0f, 1.0f, xAlpha);
      if(dotX > 0.0f)
      {
        for(unsigned int x = 0; x < numX; x++)
          glCallList(volumeObjects + x);
      }
      else
      {
        for(unsigned int x = 0; x < numX; x++)
          glCallList(volumeObjects + numX-1 - x);
       }
      glColor4f(1.0f, 1.0f, 1.0f, zAlpha);
      if(dotZ > 0.0f)
      {
        for(unsigned int z = 0; z < numZ; z++)
          glCallList(volumeObjects + numX + numY + z);
      }
      else
      {
        for(unsigned int z = 0; z < numZ; z++)
          glCallList(volumeObjects + numX + numY + numZ-1 - z);
      }
    }
    else
    {
      glColor4f(1.0f, 1.0f, 1.0f, zAlpha);
      if(dotZ > 0.0f)
      {
        for(unsigned int z = 0; z < numZ; z++)
          glCallList(volumeObjects + numX + numY + z);
      }
      else
      {
        for(unsigned int z = 0; z < numZ; z++)
          glCallList(volumeObjects + numX + numY + numZ-1 - z);
      }
      glColor4f(1.0f, 1.0f, 1.0f, xAlpha);
      if(dotX > 0.0f)
      {
        for(unsigned int x = 0; x < numX; x++)
          glCallList(volumeObjects + x);
      }
      else
      {
        for(unsigned int x = 0; x < numX; x++)
          glCallList(volumeObjects + numX-1 - x);
      }
    }
  }
  else
  {
    glColor4f(1.0f, 1.0f, 1.0f, zAlpha);
    if(dotZ > 0.0f)
    {
      for(unsigned int z = 0; z < numZ; z++)
        glCallList(volumeObjects + numX + numY + z);
    }
    else
    {
      for(unsigned int z = 0; z < numZ; z++)
        glCallList(volumeObjects + numX + numY + numZ-1 - z);
    }

    if(xAlpha > yAlpha)
    {
      glColor4f(1.0f, 1.0f, 1.0f, xAlpha);
      if(dotX > 0.0f)
      {
        for(unsigned int x = 0; x < numX; x++)
          glCallList(volumeObjects + x);
      }
      else
      {
        for(unsigned int x = 0; x < numX; x++)
          glCallList(volumeObjects + numX-1 - x);
      }
      glColor4f(1.0f, 1.0f, 1.0f, yAlpha);
      if(dotY > 0.0f)
      {
        for(unsigned int y = 0; y < numY; y++)
          glCallList(volumeObjects + numX + y);
      }
      else
      {
        for(unsigned int y = 0; y < numY; y++)
          glCallList(volumeObjects + numX + numY-1 - y);
      }
    }
    else
    {
      glColor4f(1.0f, 1.0f, 1.0f, yAlpha);
      if(dotY > 0.0f)
      {
        for(unsigned int y = 0; y < numY; y++)
          glCallList(volumeObjects + numX + y);
      }
      else
      {
        for(unsigned int y = 0; y < numY; y++)
          glCallList(volumeObjects + numX + numY-1 - y);
      }
      glColor4f(1.0f, 1.0f, 1.0f, xAlpha);
      if(dotX > 0.0f)
      {
        for(unsigned int x = 0; x < numX; x++)
          glCallList(volumeObjects + x);
      }
      else
      {
        for(unsigned int x = 0; x < numX; x++)
          glCallList(volumeObjects + numX-1 - x);
      }
    }
  }
  */
}

///// drawVolume3D ////////////////////////////////////////////////////////////
void GLMoleculeView::drawVolume3D()
/// Draws a density grid with volumetric rendering using a 3D texture. This 3D 
/// texture is created in updateVolume3D.
{
  assert(GLEE_VERSION_1_2);

  // data regarding the density grid
  const Point3D<float> origin = densityGrid->getOrigin();
  const Point3D<float> delta = densityGrid->getDelta();
  const Point3D<unsigned int> numPoints = densityGrid->getNumPoints();
  const Point3D<float> extent(delta.x()*(numPoints.x() - 1),
                              delta.y()*(numPoints.y() - 1),
                              delta.z()*(numPoints.z() - 1));

  glEnable(GL_TEXTURE_3D);
  glBindTexture(GL_TEXTURE_3D, textureID3D); // needed here?

  ///// The 3D texture itself has to be translated/scaled/rotated. The effect
  ///// on the texture coordinates will be the inverse, so all operations will be done
  ///// to gain the proper texture coordinates
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();

  ///// Scale the texture
  /// translate so the scaling will be symmetrical
  glTranslatef(0.5f, 0.5f, 0.5f); // STR
  // scale the texture such that the R-direction is uncompressed (remove transparent slices)
  // and the texture is exactly contained within the texture coordinates (-0.5-0.5, -0.5-0.5, -0.5-0.5)
  if(numPoints.y() > textureSize(numPoints.y()))
  {
    unsigned int texSizeY = textureSize(numPoints.y());
    unsigned int inc = 1;
    unsigned int numStacksY = numPoints.y();
    while(numStacksY > texSizeY)
      numStacksY = numPoints.y()/(++inc);
    glScalef(1.0f, 1.0f, static_cast<float>(numStacksY)/texSizeY);
  }
  else
    glScalef(1.0f, 1.0f, static_cast<float>(numPoints.y())/textureSize(numPoints.y())); // str
  // scale the texture such that the sizes in each direction are proportional to the actual
  // sizes, taking the largest size as a reference
  float maxExtent = extent.x() > extent.y() ? extent.x() : extent.y();
  maxExtent = extent.z() > maxExtent ? extent.z() : maxExtent;
  glScalef(maxExtent/extent.z(), maxExtent/extent.x(), maxExtent/extent.y()); // STR
  // backtranslate after scaling
  glTranslatef(-0.5f, -0.5f, -0.5f);

  ///// Rotate the texture
  // translate the texture so the rotation center coincides with the actual rotation
  // center of the grid
  const Point3D<float> newOrigin(origin.x() - (maxExtent - extent.x())/2.0f, 
                                 origin.y() - (maxExtent - extent.y())/2.0f,
                                 origin.z() - (maxExtent - extent.z())/2.0f); // xyz
  const Point3D<float> trans(-newOrigin.z()/maxExtent,
                             -newOrigin.x()/maxExtent,
                             -newOrigin.y()/maxExtent); // str
  glTranslatef(trans.x(), trans.y(), trans.z()); // str
  // rotate the texture 
  Vector3D<float> axis;
  float angle;
  orientationQuaternion->getAxisAngle(axis, angle);
  glRotatef(-angle, axis.z(), axis.x(), axis.y()); // str
  // backtranslate after rotation
  glTranslatef(-trans.x(), -trans.y(), -trans.z());

  ///// setup the model view as in GLView::paintGL(), but don't rotate as the texture is already rotated
  // rotating the textured quads would stop them from being view-aligned
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  if(baseParameters.perspectiveProjection)
    gluLookAt(0.0f, 0.0f, zPos, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
  else
    resizeGL(width(), height());
  glTranslatef(xPos, yPos, 0.0f);

  ///// Draw the slices from back to front 
  // find out the maximum granularity for the Z-stacking of the slices
  unsigned int maxSize = numPoints.x() > numPoints.y() ? numPoints.x() : numPoints.y();
  if(maxSize < numPoints.z())
    maxSize = numPoints.z();
  maxSize = textureSize(maxSize);
  // draw the slices
  glBegin(GL_QUADS);
    for(float z = 0.0f; z < 1.0f; z += 1.0f/maxSize)
    {
      // texture coordinates = STR = ZXY
      // CCW allows the use of culling
      glTexCoord3f(z, 0.0f, 0.0f); glVertex3f(newOrigin.x(),             newOrigin.y(),             newOrigin.z() + z * maxExtent);
      glTexCoord3f(z, 1.0f, 0.0f); glVertex3f(newOrigin.x() + maxExtent, newOrigin.y(),             newOrigin.z() + z * maxExtent);
      glTexCoord3f(z, 1.0f, 1.0f); glVertex3f(newOrigin.x() + maxExtent, newOrigin.y() + maxExtent, newOrigin.z() + z * maxExtent);
      glTexCoord3f(z, 0.0f, 1.0f); glVertex3f(newOrigin.x(),             newOrigin.y() + maxExtent, newOrigin.z() + z * maxExtent);
    }  
  glEnd();
  
  glPopMatrix();
  glDisable(GL_TEXTURE_3D);
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
}

///// updateVolume2D //////////////////////////////////////////////////////////
void GLMoleculeView::updateVolume2D()
/// Recalculates the stack of slices in each direction for viewing the 
/// volumetric rendering of the active density grid using 2D textures.
{
  qDebug("calling updateVolume2D");

  // data regarding the density grid
  Point3D<float> origin = densityGrid->getOrigin();
  Point3D<float> delta = densityGrid->getDelta();
  Point3D<unsigned int> numPoints = densityGrid->getNumPoints();

  // visualization settings from DensityBase
  QColor positiveColor = densityDialog->ColorButtonVolumePos->color();
  QColor negativeColor = densityDialog->ColorButtonVolumeNeg->color();
  double maxPlotValue = densityDialog->LineEditVolumePos->text().stripWhiteSpace().toDouble();
  double minPlotValue = densityDialog->LineEditVolumeNeg->text().stripWhiteSpace().toDouble();

  glEnable(GL_TEXTURE_2D);

  ///// Allocate enough display lists and texture names to hold all textured quads
  if(numVolumeObjects < numPoints.x() + numPoints.y() + numPoints.z())
  {
    qDebug("updating 2D texture data");
    clearVolumeTextures();
    numVolumeObjects = numPoints.x() + numPoints.y() + numPoints.z();
    volumeObjects = glGenLists(numVolumeObjects);
    textureID2D = new GLuint[numVolumeObjects];
    glGenTextures(numVolumeObjects, textureID2D);
  }

  glColor3f(1.0f, 1.0f, 1.0f); // needed so the colors are blended with white

  ///// Create the textured quads for the X-direction
  QImage glImage;
  for(unsigned int x = 0; x < numPoints.x(); x++)
  {
    // get the image
    glImage = glSlice(densityGrid->getSlice(DensityGrid::PLANE_YZ, x, positiveColor, negativeColor, maxPlotValue, minPlotValue));

    // create a texture from it
    glBindTexture(GL_TEXTURE_2D, textureID2D[x]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glImage.width(), glImage.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, glImage.bits());

    // create a display list
    glNewList(volumeObjects + x, GL_COMPILE);
      glBindTexture(GL_TEXTURE_2D, textureID2D[x]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(origin.x() + x * delta.x(), origin.y(),                                 origin.z());
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(origin.x() + x * delta.x(), origin.y() + (numPoints.y()-1) * delta.y(), origin.z());
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(origin.x() + x * delta.x(), origin.y() + (numPoints.y()-1) * delta.y(), origin.z() + (numPoints.z()-1) * delta.z());
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(origin.x() + x * delta.x(), origin.y(),                                 origin.z() + (numPoints.z()-1) * delta.z());
      glEnd();
    glEndList();
  }

  ///// Create the textured quads for the Y-direction
  for(unsigned int y = 0; y < numPoints.y(); y++)
  {
    // get the image
    glImage = glSlice(densityGrid->getSlice(DensityGrid::PLANE_XZ, y, positiveColor, negativeColor, maxPlotValue, minPlotValue));

    // create a texture from it
    glBindTexture(GL_TEXTURE_2D, textureID2D[numPoints.x() + y]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glImage.width(), glImage.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, glImage.bits());

    // create a display list
    glNewList(volumeObjects + numPoints.x() + y, GL_COMPILE);
      glBindTexture(GL_TEXTURE_2D, textureID2D[numPoints.x() + y]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(origin.x(),                                 origin.y() + y * delta.y(), origin.z());
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(origin.x() + (numPoints.x()-1) * delta.x(), origin.y() + y * delta.y(), origin.z());
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(origin.x() + (numPoints.x()-1) * delta.x(), origin.y() + y * delta.y(), origin.z() + (numPoints.z()-1) * delta.z());
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(origin.x(),                                 origin.y() + y * delta.y(), origin.z() + (numPoints.z()-1) * delta.z());
      glEnd();
    glEndList();
  }

  ///// Create the textured quads for the Z-direction
  for(unsigned int z = 0; z < numPoints.z(); z++)
  {
    // get the image
    glImage = glSlice(densityGrid->getSlice(DensityGrid::PLANE_XY, z, positiveColor, negativeColor, maxPlotValue, minPlotValue));

    // create a texture from it
    glBindTexture(GL_TEXTURE_2D, textureID2D[numPoints.x() + numPoints.y() + z]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glImage.width(), glImage.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, glImage.bits());

    // create a display list
    glNewList(volumeObjects + numPoints.x() + numPoints.y() + z, GL_COMPILE);
      glBindTexture(GL_TEXTURE_2D, textureID2D[numPoints.x() + numPoints.y() + z]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(origin.x(),                                 origin.y(),                                 origin.z() + z * delta.z());
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(origin.x() + (numPoints.x()-1) * delta.x(), origin.y(),                                 origin.z() + z * delta.z());
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(origin.x() + (numPoints.x()-1) * delta.x(), origin.y() + (numPoints.y()-1) * delta.y(), origin.z() + z * delta.z());
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(origin.x(),                                 origin.y() + (numPoints.y()-1) * delta.y(), origin.z() + z * delta.z());
      glEnd();
    glEndList();
  }
  reorderShapes();
  glDisable(GL_TEXTURE_2D);

  ///// old code which reloaded textures upon changes in the viewing direction
  ///// -> noticable interruptions when changing directions
  /*
  // data regarding the density grid
  Point3D<float> origin = densityGrid->getOrigin();
  Point3D<float> delta = densityGrid->getDelta();
  Point3D<unsigned int> numPoints = densityGrid->getNumPoints();

  // visluaization settings from DensityBase
  QColor positiveColor = densityDialog->ColorButtonVolumePos->color();
  QColor negativeColor = densityDialog->ColorButtonVolumeNeg->color();
  double maxPlotValue = densityDialog->LineEditVolumePos->text().stripWhiteSpace().toDouble();
  double minPlotValue = densityDialog->LineEditVolumeNeg->text().stripWhiteSpace().toDouble();

  // rest
  QColor backgroundColor(qRgba(255, 255, 255, 0)); // fully transparant
  QImage image, glImage;
  GLuint textureID;

  // update according to the orientation of the scene
  switch(getDirection())
  {
    case DIRECTION_POSX:
    {
      // make sure a sufficient number of OpenGL display lists are available
      if(numVolumeObjects < numPoints.x())
      {
        if(numVolumeObjects > 0)
          glDeleteLists(volumeObjects, numVolumeObjects);
        numVolumeObjects = numPoints.x();
        volumeObjects = glGenLists(numVolumeObjects);
      }
      // get the QImages, bind them to textures and create OpenGL display lists for showing them
      for(unsigned int x = 0; x < numPoints.x(); x++)
      {
        // get the image
        image = densityGrid->getSlice(DensityGrid::PLANE_YZ, x, positiveColor, negativeColor, backgroundColor, maxPlotValue, minPlotValue);
        glImage = QGLWidget::convertToGLFormat(image.smoothScale(64, 64));

        // create a texture from it
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glImage.width(), glImage.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, glImage.bits());

        // create a display list
        glNewList(volumeObjects + x, GL_COMPILE);
          glBindTexture(GL_TEXTURE_2D, textureID);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
          glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f);
            glVertex3f(origin.x() + x * delta.x(), origin.y(),                                 origin.z());
            glTexCoord2f(1.0f, 0.0f);
            glVertex3f(origin.x() + x * delta.x(), origin.y() + (numPoints.y()-1) * delta.y(), origin.z());
            glTexCoord2f(1.0f, 1.0f);
            glVertex3f(origin.x() + x * delta.x(), origin.y() + (numPoints.y()-1) * delta.y(), origin.z() + (numPoints.z()-1) * delta.z());
            glTexCoord2f(0.0f, 1.0f);
            glVertex3f(origin.x() + x * delta.x(), origin.y(),                                 origin.z() + (numPoints.z()-1) * delta.z());
          glEnd();
        glEndList();
      }
      break;
    }

    case DIRECTION_NEGX:
    {
      // make sure a sufficient number of OpenGL display lists are available
      if(numVolumeObjects < numPoints.x())
      {
        if(numVolumeObjects > 0)
          glDeleteLists(volumeObjects, numVolumeObjects);
        numVolumeObjects = numPoints.x();
        volumeObjects = glGenLists(numVolumeObjects);
      }
      // get the QImages, bind them to textures and create OpenGL display lists for showing them
      for(unsigned int x = 0; x < numPoints.x(); x++)
      {
        // get the image
        image = densityGrid->getSlice(DensityGrid::PLANE_YZ, x, positiveColor, negativeColor, backgroundColor, maxPlotValue, minPlotValue);
        glImage = QGLWidget::convertToGLFormat(image.smoothScale(64, 64));

        // create a texture from it
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glImage.width(), glImage.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, glImage.bits());

        // create a display list
        glNewList(volumeObjects + x, GL_COMPILE);
          glBindTexture(GL_TEXTURE_2D, textureID);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
          glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f);
            glVertex3f(origin.x() + x * delta.x(), origin.y(),                                 origin.z());
            glTexCoord2f(0.0f, 1.0f);
            glVertex3f(origin.x() + x * delta.x(), origin.y(),                                 origin.z() + (numPoints.z()-1) * delta.z());
            glTexCoord2f(1.0f, 1.0f);
            glVertex3f(origin.x() + x * delta.x(), origin.y() + (numPoints.y()-1) * delta.y(), origin.z() + (numPoints.z()-1) * delta.z());
            glTexCoord2f(1.0f, 0.0f);
            glVertex3f(origin.x() + x * delta.x(), origin.y() + (numPoints.y()-1) * delta.y(), origin.z());
          glEnd();
        glEndList();
      }
      break;
    }

    case DIRECTION_POSY:
    {
      // make sure a sufficient number of OpenGL display lists are available
      if(numVolumeObjects < numPoints.y())
      {
        if(numVolumeObjects > 0)
          glDeleteLists(volumeObjects, numVolumeObjects);
        numVolumeObjects = numPoints.y();
        volumeObjects = glGenLists(numVolumeObjects);
      }
      // get the QImages, bind them to textures and create OpenGL display lists for showing them
      for(unsigned int y = 0; y < numPoints.y(); y++)
      {
        // get the image
        image = densityGrid->getSlice(DensityGrid::PLANE_XZ, y, positiveColor, negativeColor, backgroundColor, maxPlotValue, minPlotValue);
        glImage = QGLWidget::convertToGLFormat(image.smoothScale(64, 64));

        // create a texture from it
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glImage.width(), glImage.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, glImage.bits());

        // create a display list
        glNewList(volumeObjects + y, GL_COMPILE);
          glBindTexture(GL_TEXTURE_2D, textureID);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
          glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f);
            glVertex3f(origin.x(),                                 origin.y() + y * delta.y(), origin.z());
            glTexCoord2f(1.0f, 0.0f);
            glVertex3f(origin.x(),                                 origin.y() + y * delta.y(), origin.z() + (numPoints.z()-1) * delta.z());
            glTexCoord2f(1.0f, 1.0f);
            glVertex3f(origin.x() + (numPoints.x()-1) * delta.x(), origin.y() + y * delta.y(), origin.z() + (numPoints.z()-1) * delta.z());
            glTexCoord2f(0.0f, 1.0f);
            glVertex3f(origin.x() + (numPoints.x()-1) * delta.x(), origin.y() + y * delta.y(), origin.z());
          glEnd();
        glEndList();
      }
      break;
    }

    case DIRECTION_NEGY:
    {
      // make sure a sufficient number of OpenGL display lists are available
      if(numVolumeObjects < numPoints.y())
      {
        if(numVolumeObjects > 0)
          glDeleteLists(volumeObjects, numVolumeObjects);
        numVolumeObjects = numPoints.y();
        volumeObjects = glGenLists(numVolumeObjects);
      }
      // get the QImages, bind them to textures and create OpenGL display lists for showing them
      for(unsigned int y = 0; y < numPoints.y(); y++)
      {
        // get the image
        image = densityGrid->getSlice(DensityGrid::PLANE_XZ, y, positiveColor, negativeColor, backgroundColor, maxPlotValue, minPlotValue);
        glImage = QGLWidget::convertToGLFormat(image.smoothScale(64, 64));

        // create a texture from it
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glImage.width(), glImage.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, glImage.bits());

        // create a display list
        glNewList(volumeObjects + y, GL_COMPILE);
          glBindTexture(GL_TEXTURE_2D, textureID);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
          glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f);
            glVertex3f(origin.x(),                                 origin.y() + y * delta.y(), origin.z());
            glTexCoord2f(0.0f, 1.0f);
            glVertex3f(origin.x() + (numPoints.x()-1) * delta.x(), origin.y() + y * delta.y(), origin.z());
            glTexCoord2f(1.0f, 1.0f);
            glVertex3f(origin.x() + (numPoints.x()-1) * delta.x(), origin.y() + y * delta.y(), origin.z() + (numPoints.z()-1) * delta.z());
            glTexCoord2f(1.0f, 0.0f);
            glVertex3f(origin.x(),                                 origin.y() + y * delta.y(), origin.z() + (numPoints.z()-1) * delta.z());
          glEnd();
        glEndList();
      }
      break;
    }

    case DIRECTION_POSZ:
    {
      // make sure a sufficient number of OpenGL display lists are available
      if(numVolumeObjects < numPoints.z())
      {
        if(numVolumeObjects > 0)
          glDeleteLists(volumeObjects, numVolumeObjects);
        numVolumeObjects = numPoints.z();
        volumeObjects = glGenLists(numVolumeObjects);
      }
      // get the QImages, bind them to textures and create OpenGL display lists for showing them
      for(unsigned int z = 0; z < numPoints.z(); z++)
      {
        // get the image
        image = densityGrid->getSlice(DensityGrid::PLANE_XY, z, positiveColor, negativeColor, backgroundColor, maxPlotValue, minPlotValue);
        glImage = QGLWidget::convertToGLFormat(image.smoothScale(64, 64));

        // create a texture from it
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glImage.width(), glImage.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, glImage.bits());

        // create a display list
        glNewList(volumeObjects + z, GL_COMPILE);
          glBindTexture(GL_TEXTURE_2D, textureID);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
          glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f);
            glVertex3f(origin.x(),                                 origin.y(),                                 origin.z() + z * delta.z());
            glTexCoord2f(1.0f, 0.0f);
            glVertex3f(origin.x() + (numPoints.x()-1) * delta.x(), origin.y(),                                 origin.z() + z * delta.z());
            glTexCoord2f(1.0f, 1.0f);
            glVertex3f(origin.x() + (numPoints.x()-1) * delta.x(), origin.y() + (numPoints.y()-1) * delta.y(), origin.z() + z * delta.z());
            glTexCoord2f(0.0f, 1.0f);
            glVertex3f(origin.x(),                                 origin.y() + (numPoints.y()-1) * delta.y(), origin.z() + z * delta.z());
          glEnd();
        glEndList();
      }
      break;
    }

    case DIRECTION_NEGZ:
    {
      // make sure a sufficient number of OpenGL display lists are available
      if(numVolumeObjects < numPoints.z())
      {
        if(numVolumeObjects > 0)
          glDeleteLists(volumeObjects, numVolumeObjects);
        numVolumeObjects = numPoints.z();
        volumeObjects = glGenLists(numVolumeObjects);
      }
      // get the QImages, bind them to textures and create OpenGL display lists for showing them
      for(unsigned int z = 0; z < numPoints.z(); z++)
      {
        // get the image
        image = densityGrid->getSlice(DensityGrid::PLANE_XY, z, positiveColor, negativeColor, backgroundColor, maxPlotValue, minPlotValue);
        glImage = QGLWidget::convertToGLFormat(image.smoothScale(64, 64));

        // create a texture from it
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glImage.width(), glImage.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, glImage.bits());

        // create a display list
        glNewList(volumeObjects + z, GL_COMPILE);
          glBindTexture(GL_TEXTURE_2D, textureID);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
          glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f);
            glVertex3f(origin.x(),                                 origin.y(),                                 origin.z() + z * delta.z());
            glTexCoord2f(0.0f, 1.0f);
            glVertex3f(origin.x(),                                 origin.y() + (numPoints.y()-1) * delta.y(), origin.z() + z * delta.z());
            glTexCoord2f(1.0f, 1.0f);
            glVertex3f(origin.x() + (numPoints.x()-1) * delta.x(), origin.y() + (numPoints.y()-1) * delta.y(), origin.z() + z * delta.z());
            glTexCoord2f(1.0f, 0.0f);
            glVertex3f(origin.x() + (numPoints.x()-1) * delta.x(), origin.y(),                                 origin.z() + z * delta.z());
          glEnd();
        glEndList();
      }
      break;
    }
  }
  */
}

///// updateVolume3D //////////////////////////////////////////////////////////
void GLMoleculeView::updateVolume3D()
/// Recalculates the 3D texture for viewing the volumetric rendering of the 
/// active density grid. The mapping between texture coordinates and regular
/// coordinates is (S, T, R) -> (Z, X, Y)
/// Currently a 32bit RGBA texture is used although only 2 colors are ever
/// present in the texture. This can be optimized by using 2 Alpha textures 
/// and blending them with their respective colors. This would cut memory costs
/// in half.
{
  qDebug("calling updateVolume3D");

  // data regarding the density grid
  const Point3D<unsigned int> numPoints = densityGrid->getNumPoints();

  // visualization settings from DensityBase
  const QColor positiveColor = densityDialog->ColorButtonVolumePos->color();
  const QColor negativeColor = densityDialog->ColorButtonVolumeNeg->color();
  const double maxPlotValue = densityDialog->LineEditVolumePos->text().stripWhiteSpace().toDouble();
  const double minPlotValue = densityDialog->LineEditVolumeNeg->text().stripWhiteSpace().toDouble();

  // get the dimensions of the 3D texture

  const Point3D<unsigned int> textureSize3D(textureSize(numPoints.x()), textureSize(numPoints.y()), textureSize(numPoints.z()));
  const unsigned int planeXZ = 4 * textureSize3D.x() * textureSize3D.z(); // size of the data needed to store an XZ plane
  // the X and Z dimensions are correctly scaled by the getSlice function, but the Y dimension isn't
  unsigned int incValue = 1; // by default don't skip XZ-planes while reading values
  unsigned int numStacksY = numPoints.y(); // the actual number of XZ-planes to read
  while(numStacksY > textureSize3D.y())
    numStacksY = numPoints.y()/(++incValue); // start by skipping 1 plane and increasing linearly

  ///// store the texture values in planes of XZ and loop over Y
  unsigned char* gridData = new unsigned char[4 * planeXZ * textureSize3D.y()]; // 4 because of RGBA / 32 bit
   
  // store transparent slices for the first half of unused slices
  /*
  const QImage glImageT = glSlice(densityGrid->getSlice(DensityGrid::PLANE_ZX, 0, positiveColor, negativeColor, 1000.0, -1000.0)); // a fully transparent slice
  for(unsigned int y = 0; y < (textureSize3D.y() - numStacksY)/2; ++y)
    memcpy((void*)(gridData + y * planeXZ), glImageT.bits(), planeXZ); // can this be changed to just store a lot of 'FF FF FF 00' sequences?  
  */
  memset(gridData, '\0', (textureSize3D.y() - numStacksY)/2 * planeXZ);

  // store the central numPoints.y() used slices
  QImage glImage;
  for(unsigned int y = (textureSize3D.y() - numStacksY)/2; y < (textureSize3D.y() - numStacksY)/2 + numStacksY; ++y)
  {
    glImage = glSlice(densityGrid->getSlice(DensityGrid::PLANE_ZX, (y - (textureSize3D.y() - numStacksY)/2)*incValue, positiveColor, negativeColor, maxPlotValue, minPlotValue));
    memcpy((void*)(gridData + y * planeXZ), glImage.bits(), planeXZ);
  }
  // store again transparent slices for the rest
  /*
  for(unsigned int y = (textureSize3D.y() - numStacksY)/2 + numStacksY; y < textureSize3D.y(); ++y)
    memcpy((void*)(gridData + y * planeXZ), glImageT.bits(), planeXZ);
  */
  memset(&gridData[((textureSize3D.y() - numStacksY)/2 + numStacksY)*planeXZ], '\0', 
         (textureSize3D.y() - ((textureSize3D.y() - numStacksY)/2 + numStacksY))*planeXZ);

  ///// create a texture from it

  glEnable(GL_TEXTURE_3D); // needed here?
  // Allocate a texture name if needed
  if(textureID3D == 0)
  {
    qDebug("updating 3D texture data");
    clearVolumeTextures(); // clear any 2D data
    glGenTextures(1, &textureID3D);
  }

  glBindTexture(GL_TEXTURE_3D, textureID3D);
  glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, textureSize3D.z(), textureSize3D.x(), textureSize3D.y(), 0, GL_RGBA, GL_UNSIGNED_BYTE, gridData);
  delete [] gridData; gridData = 0;
  // no texture repeating
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP); 
  // texture interpolation
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glDisable(GL_TEXTURE_3D);
  reorderShapes();
}

///// drawSlice ///////////////////////////////////////////////////////////////
void GLMoleculeView::drawSlice()
/// Draws a 2D slice
{
  qDebug("calling drawSlice");
  assert(densityDialog != NULL);

  if(densityDialog->CheckBoxSliceTransparent->isOn())
    glColor3f(1.0f, 1.0f, 1.0f); // for tranparent slices blend the color with white
  else
    qglColor(densityDialog->ColorButtonSliceBack->color());

  glDisable(GL_CULL_FACE);
  glDisable(GL_LIGHTING);
  glCallList(sliceObject);
  glEnable(GL_LIGHTING);
  glEnable(GL_CULL_FACE);
}

///// getDirection ////////////////////////////////////////////////////////////
unsigned int GLMoleculeView::getDirection() const
/// Calculates the face of the scene (along the axes) currently most visible.
/// This is done by calculating the dot product of the view vector with the normal
/// of each of the six faces. The result is the cosine of the angle between the
/// 2 vectors. A value of 1 indicates vectors pointing in the same direction,
/// a value of -1 indicates opposite direction.
{
  // get the axis/angle representation of the current scene orientation
  Vector3D<float> orientationVector, axis;
  float orientationAngle;
  orientationQuaternion->getAxisAngle(orientationVector, orientationAngle);
  orientationAngle = -orientationAngle; // we need opposite rotation from the OpenGL one.

  // the view vector (not needed anymore because of the shortcut in calculating the dot product,
  // namely a dot product with (0, 0, 1) is always equal to the Z-component of the other argument)
  //Vector3D<float> view(0.0f, 0.0f, 1.0f); // we're looking into the negative Z-direction so the normal
                                          // of the plane we want to see points towards us into the positive Z-direction

  // rotate a unit vector along the positive X-axis
  axis.setValues(1.0f, 0.0f, 0.0f);
  axis.rotate(orientationVector, orientationAngle);
  // get the dot product with the view vector
  // as the view vector is (0, 0, 1), the dot product is always equal to the Z-component of the axis
  float dotX = axis.z(); // = axis.dot(view); with view = Vector3D(0.0f, 0.0f, 1.0f);

  // rotate a unit vector along the positive Y-axis
  axis.setValues(0.0f, 1.0f, 0.0f);
  axis.rotate(orientationVector, orientationAngle);
  // get the dot product with the view vector
  float dotY = axis.z();

  // rotate a unit vector along the positive Z-axis
  axis.setValues(0.0f, 0.0f, 1.0f);
  axis.rotate(orientationVector, orientationAngle);
  // get the dot product with the view vector
  float dotZ = axis.z();

  // determine the general direction
  unsigned int direction = DIRECTION_NONE;
  if(fabs(dotX) > fabs(dotY))
  {
    if(fabs(dotX) > fabs(dotZ))
      direction = DIRECTION_POSX;
    else
      direction = DIRECTION_POSZ;
  }
  else
  {
    if(fabs(dotY) > fabs(dotZ))
      direction = DIRECTION_POSY;
    else
      direction = DIRECTION_POSZ;
  }

  // determine the specific direction
  if(direction == DIRECTION_POSX && dotX < 0.0f)
    direction = DIRECTION_NEGX;
  else if(direction == DIRECTION_POSY && dotY < 0.0f)
    direction = DIRECTION_NEGY;
  else if(direction == DIRECTION_POSZ && dotZ < 0.0f)
    direction = DIRECTION_NEGZ;

  //qDebug("direction %d has been chosen", direction);

  return direction;
}

///// textureSize /////////////////////////////////////////////////////////////
unsigned int GLMoleculeView::textureSize(const unsigned int size) const
/// Returns the power-of-two size of the given size according to the maximum 
/// texture size settings and OpenGL limitations. The minimum size will be 16.
{
  unsigned int result = 16;
  if(size > static_cast<unsigned int>(textureParameters.maximumSize))
    result = textureParameters.maximumSize;
  else if(size > 16)
  {
    // round to the nearest higher power of 2
    int exp; frexp(static_cast<double>(size), &exp); // frexp(x, e) = m so that x = m * 2^e with m in [0.5,1.0[
    result = 1 << exp; // = 2^exp
  }
  return result;
}

///// glSlice /////////////////////////////////////////////////////////////////
QImage GLMoleculeView::glSlice(const QImage& image) const
/// Resizes the given image according to the maximum texture size (so it shows
/// maximum detail when shown as an OpenGL texture) and returns it in OpenGL format.
{
  return QGLWidget::convertToGLFormat(image.smoothScale(textureSize(image.width()), textureSize(image.height())));
}

///// clearVolumeTextures /////////////////////////////////////////////////////
void GLMoleculeView::clearVolumeTextures()
/// Clears any data allocated for volume textures.
{
  // 2D textures  
  if(numVolumeObjects > 0)
  {
    glDeleteLists(volumeObjects, numVolumeObjects);
    glDeleteTextures(numVolumeObjects, textureID2D);
    delete[] textureID2D;
    numVolumeObjects = 0;
    textureID2D = NULL;
  }
  // 3D texture
  if(textureID3D != 0)
  {
    glDeleteTextures(1, &textureID3D);
    textureID3D = 0;
  }
}

///////////////////////////////////////////////////////////////////////////////
///// Static Variables                                                    /////
///////////////////////////////////////////////////////////////////////////////

bool GLMoleculeView::manipulateSelection = false;
GLMoleculeView::GLTextureParameters GLMoleculeView::textureParameters = {128, false};
