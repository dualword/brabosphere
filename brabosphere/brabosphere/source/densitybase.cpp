/***************************************************************************
                        densitybase.cpp  -  description
                             -------------------
    begin                : Thu Mar 17 2005
    copyright            : (C) 2005-2006 by Ben Swerts
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
  \class DensityBase
  \brief This class allows changing the representation of density isosurfaces.

  It allows adding/deleting and changing the visual representation of isodensity
  surfaces. Up to 2 source densities can be loaded and combined (added/substracted).
  It provides the functionality for the DensityWidget class (base class).
*/
/// \file
/// Contains the implementation of the class DensityBase

///// Header files ////////////////////////////////////////////////////////////

// C++ header files
#include <cassert>
#include <cmath>

// STL header files
#include <algorithm>
#include <functional>

// Qt header files
#include <qapplication.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qdatetime.h>
#include <qfile.h>
#include <qfiledialog.h>
#include <qgroupbox.h>
#include <qinputdialog.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qlistview.h>
#include <qmessagebox.h>
#include <qprogressbar.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qslider.h>
#include <qstring.h>
#include <qtextstream.h>
#include <qvalidator.h>
#include <qwidgetstack.h>

// Xbrabo header files
#include "colorbutton.h"
#include "densitybase.h"
#include "densitygrid.h"
#include "loadcubethread.h"
#include "loadpltthread.h"
#include "mappedsurfacewidget.h"

///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// Constructor /////////////////////////////////////////////////////////////
DensityBase::DensityBase(DensityGrid* grid, QWidget* parent, const char* name, bool modal, WFlags fl) : DensityWidget(parent, name, modal, fl),
  densityGrid(grid),
  loadingThread(0),
  columnColourWidth(-1),
  oldVisualizationType(-1)
/// The defaults constructor.
{
  assert(densityGrid != NULL);
  // validators
  QDoubleValidator* v = new QDoubleValidator(-100.0,100.0,3,this);
  LineEditLevel->setValidator(v);
  LineEditVolumePos->setValidator(v);
  LineEditVolumeNeg->setValidator(v);
  LineEditSlicePos->setValidator(v);
  LineEditSliceNeg->setValidator(v);
  v = 0;
  // Isosurfaces
  ListViewParameters->setSorting(-1);
  ListViewParameters->setColumnWidthMode(COLUMN_ID,QListView::Manual);
  ListViewParameters->setColumnWidth(COLUMN_ID,0);
  ListViewParameters->setColumnWidthMode(COLUMN_RGB,QListView::Manual);
  ListViewParameters->setColumnWidth(COLUMN_RGB,0);
  ProgressBarA->hide();
  ProgressBarB->hide();
  // Volume
  ColorButtonVolumePos->setColor(QColor(0, 0, 255));
  ColorButtonVolumeNeg->setColor(QColor(255, 0, 0));
  // Slice
  ColorButtonSlicePos->setColor(QColor(0, 0, 255));
  ColorButtonSliceNeg->setColor(QColor(255, 0, 0));
  ColorButtonSliceBack->setColor(QColor(255, 255, 255));
  LabelSliceMap1->hide();
  LabelSliceMap2->hide();
  LabelSliceMap3->hide();
  ComboBoxSliceMap->hide();
  // Density mapping
  mappingWidget = new MappedSurfaceWidget(this, 0, true);

  enableWidgets();
  makeConnections();
}

///// Destructor //////////////////////////////////////////////////////////////
DensityBase::~DensityBase()
///The default destructor.
{
  if(loadingThread != 0)
  {
    if(loadingThread->running())
    {
      loadingThread->stop();
      loadingThread->wait();
    }
    delete loadingThread;
  }
}

///// visualizationType ///////////////////////////////////////////////////////
unsigned int DensityBase::visualizationType() const
/// Returns the active visualization type.
{
  return ComboBoxVisualizationType->currentItem();
}

///// surfaceVisible //////////////////////////////////////////////////////////
bool DensityBase::surfaceVisible(const unsigned int surface) const
/// Returns whether a surface is visible.
{
  assert(surface < surfaceProperties.size());

  return surfaceProperties[surface].visible;
}

///// surfaceMapping //////////////////////////////////////////////////////////
bool DensityBase::surfaceMapping() const
/// Returns whether surfaces are using a color map.
{
  return PushButtonMapped->isOn();
}

///// surfaceColor ////////////////////////////////////////////////////////////
QColor DensityBase::surfaceColor(const unsigned int surface) const
/// Returns the color of a surface.
{
  assert(surface < surfaceProperties.size());

  return QColor(surfaceProperties[surface].colour);
}

///// surfaceOpacity //////////////////////////////////////////////////////////
unsigned int DensityBase::surfaceOpacity(const unsigned int surface) const
/// Returns the opacity of a surface.
{
  assert(surface < surfaceProperties.size());

  return surfaceProperties[surface].opacity;
}

///// surfaceType /////////////////////////////////////////////////////////////
unsigned int DensityBase::surfaceType(const unsigned int surface) const
/// Returns the drawing type of a surface.
{
  assert(surface < surfaceProperties.size());

  return surfaceProperties[surface].type;
}

///////////////////////////////////////////////////////////////////////////////
///// Public Slots                                                        /////
///////////////////////////////////////////////////////////////////////////////

///// loadDensityA ////////////////////////////////////////////////////////////
void DensityBase::loadDensityA()
/// Loads the contents of a cube file for density A.
{
  loadDensity(true);
}

///// loadDensityB ////////////////////////////////////////////////////////////
void DensityBase::loadDensityB()
/// Loads the contents of a cube file for density B.
{
  loadDensity(false);
}

///// addSurface //////////////////////////////////////////////////////////////
void DensityBase::addSurface()
/// Generates a new surface with default parameters.
{
  ///// add a listview item with the parameters from GroupBoxSettings
  QCheckListItem* item = new QCheckListItem(ListViewParameters, ListViewParameters->lastItem(), QString::null, QCheckListItem::CheckBox);
  item->setOn(true);
  ListViewParameters->blockSignals(true);
  ListViewParameters->setSelected(item, true);
  ListViewParameters->blockSignals(false);
  updateListView();
  item->setText(COLUMN_ID, QString::number(++idCounter));

  ///// save the data
  SurfaceProperties newSurface;
  newSurface.visible = true;
  newSurface.level = LineEditLevel->text().stripWhiteSpace().toDouble();
  newSurface.colour = ColorButtonLevel->color().rgb();
  newSurface.opacity = SliderOpacity->value();
  newSurface.type = ComboBoxType->currentItem();
  newSurface.deleted = false;
  newSurface.isNew = true;
  newSurface.ID = idCounter;
  surfaceProperties.push_back(newSurface);

  checkUpdate();
  enableWidgets();
}

///// addSurfacePair //////////////////////////////////////////////////////////
void DensityBase::addSurfacePair()
/// Generates 2 new surfaces with opposite sign. The parameters are not based
/// on the currently selected surface.
{
  ///// add 2 listview item with the parameters from GroupBoxSettings

  ///// positive blue with level = 0.05 (or max density)
  ListViewParameters->blockSignals(true);
  QCheckListItem* item = new QCheckListItem(ListViewParameters, ListViewParameters->lastItem(), QString::null, QCheckListItem::CheckBox);
  item->setOn(true);
  item->setText(COLUMN_ID, QString::number(++idCounter));
  QColor blue(0, 0, 255);
  item->setText(COLUMN_RGB, QString::number(blue.rgb()));
  if(0.05 > LabelMax->text().stripWhiteSpace().toDouble())
    item->setText(COLUMN_LEVEL, LabelMax->text());
  else
    item->setText(COLUMN_LEVEL, "0.05");
  const int columnColourWidth = ListViewParameters->columnWidth(COLUMN_COLOUR);
  QPixmap pm(ListViewParameters->width(), item->height() - 2);
  pm.fill(blue);
  item->setPixmap(COLUMN_COLOUR,pm);
  item->setText(COLUMN_OPACITY, QString::number(SliderOpacity->value()));
  item->setText(COLUMN_TYPE, ComboBoxType->currentText());
  ///// save the data
  SurfaceProperties newSurface;
  newSurface.visible = true;
  newSurface.level = LineEditLevel->text().stripWhiteSpace().toDouble();
  newSurface.colour = blue.rgb();
  newSurface.opacity = SliderOpacity->value();
  newSurface.type = ComboBoxType->currentItem();
  newSurface.deleted = false;
  newSurface.isNew = true;
  newSurface.ID = idCounter;
  surfaceProperties.push_back(newSurface);

  ///// negative red with level = -pos (or min density)
  QCheckListItem* item2 = new QCheckListItem(ListViewParameters, item, QString::null, QCheckListItem::CheckBox);
  item2->setOn(true);
  item2->setText(COLUMN_ID, QString::number(++idCounter));
  QColor red(255, 0, 0);
  item2->setText(COLUMN_RGB, QString::number(red.rgb()));
  if(-item->text(COLUMN_LEVEL).stripWhiteSpace().toDouble() < LabelMin->text().stripWhiteSpace().toDouble())
    item2->setText(COLUMN_LEVEL, LabelMin->text());
  else
    item2->setText(COLUMN_LEVEL, "-" + item->text(COLUMN_LEVEL));
  pm.fill(red);
  item2->setPixmap(COLUMN_COLOUR,pm);
  ListViewParameters->setColumnWidth(COLUMN_COLOUR, columnColourWidth);
  item2->setText(COLUMN_OPACITY, QString::number(SliderOpacity->value()));
  item2->setText(COLUMN_TYPE, ComboBoxType->currentText());
  ListViewParameters->blockSignals(false);

  ///// save the data
  newSurface.level = LineEditLevel->text().stripWhiteSpace().toDouble();
  newSurface.colour = red.rgb();
  newSurface.ID = idCounter;
  surfaceProperties.push_back(newSurface);

  ListViewParameters->setSelected(item2, true);
  updateSettings();

  qDebug("RGB values for blue: " + item->text(COLUMN_RGB));
  qDebug("RGB values for red:  " + item2->text(COLUMN_RGB));

  checkUpdate();
  enableWidgets();
}

///// deleteSurface ///////////////////////////////////////////////////////////
void DensityBase::deleteSurface()
/// Deletes an existing surface. The currently selected surface will be removed.
{
  ///// get the currently selected item
  QListViewItem* item = ListViewParameters->selectedItem();
  if(item == NULL)
    return;

  ///// set the deleted flag in the corresponding struct
  unsigned int itemID = item->text(COLUMN_ID).toUInt();
  std::vector<SurfaceProperties>::iterator it = surfaceProperties.begin();
  while(it != surfaceProperties.end())
  {
    if((*it).ID == itemID)
      (*it).deleted = true;
    it++;
  }

  ///// delete the item and select the first item
  delete item;
  if(ListViewParameters->childCount() != 0)
  {
    ListViewParameters->setSelected(ListViewParameters->firstChild(), true);
    updateSettings();
  }

  checkUpdate();
  enableWidgets();
}

///// updateAll ///////////////////////////////////////////////////////////////
void DensityBase::updateAll()
/// Updates all changes.
{
  int newVisualizationType = ComboBoxVisualizationType->currentItem();
  bool changed = oldVisualizationType != newVisualizationType;

  switch(newVisualizationType)
  {
    case 0: // Isosurfaces
            changed = updateIsoSurfaces() || changed; // have to do it in this order as otherwise the function might never be called
            break;
    case 1: // Volumetric rendering
            changed = updateVolume() || changed;
            break;
    case 2: // Slice
            changed = updateSlice() || changed;
            break;
  }
  oldVisualizationType = newVisualizationType;
  if(changed)
    emit redrawScene();
  //qDebug("at the end of updateAll, oldVisualizationType = %d", oldVisualizationType);
}


///////////////////////////////////////////////////////////////////////////////
///// Protected Member Functions                                          /////
///////////////////////////////////////////////////////////////////////////////

///// customEvent /////////////////////////////////////////////////////////////
void DensityBase::customEvent(QCustomEvent* e)
/// Handles custom events originating from loadingThread
{
  ///// update the loading progress
  if(e->type() == 1001)
    updateProgress(*(static_cast<unsigned int*>(e->data())));
  ///// finish up after the thread has ended
  else if(e->type() == 1002)
    updateDensity();
}

///// showEvent /////////////////////////////////////////////////////////////
void DensityBase::showEvent(QShowEvent* e)
/// Restores the correct column width for the colour column after a hide/show
/// cycle.
{
  if(columnColourWidth != -1)
    ListViewParameters->setColumnWidth(COLUMN_COLOUR, columnColourWidth);
  DensityWidget::showEvent(e);
}

///// hideEvent /////////////////////////////////////////////////////////////
void DensityBase::hideEvent(QHideEvent* e)
/// Saves the correct column width for the colour column before a hide/show
/// cycle.
{
  columnColourWidth = ListViewParameters->columnWidth(COLUMN_COLOUR);
  DensityWidget::hideEvent(e);
}

///////////////////////////////////////////////////////////////////////////////
///// Private Slots                                                       /////
///////////////////////////////////////////////////////////////////////////////

///// updateVisualizationType /////////////////////////////////////////////////
void DensityBase::updateVisualizationType()
/// Does the necessary updates when a new visualization type was chosen.
{
  const int currentType = ComboBoxVisualizationType->currentItem();
  // show the new page
  WidgetStack->raiseWidget(currentType);
  enableWidgets();

  // automatic update?
  checkUpdate();
}

///// updateSliderLevel ///////////////////////////////////////////////////////
void DensityBase::updateSliderLevel()
/// Updates SliderLevel with the contents of LineEditLevel.
{
  bool convOK;
  double level = LineEditLevel->text().stripWhiteSpace().toDouble(&convOK);
  if(!convOK)
    return;
  SliderLevel->blockSignals(true);
  SliderLevel->setValue(static_cast<int>(level/deltaLevel));
  SliderLevel->blockSignals(false);
}

///// updateLineEditLevel /////////////////////////////////////////////////////
void DensityBase::updateLineEditLevel()
/// Updates LineEditLevel with the contents of SliderLevel.
{
  LineEditLevel->setText(QString::number(SliderLevel->value()*deltaLevel,'f',3));
}

///// updateListView //////////////////////////////////////////////////////////
void DensityBase::updateListView()
/// Updates ListViewParameters with the contents of the settings.
{
  //qDebug("calling updateListView");
  QListViewItem* item = ListViewParameters->selectedItem();
  if(item == 0)
    return;
  ListViewParameters->blockSignals(true);
  item->setText(COLUMN_LEVEL, LineEditLevel->text());
  ///// only update the pixmap if needed
  if(item->pixmap(COLUMN_COLOUR) == 0 || item->text(COLUMN_RGB).toUInt() != ColorButtonLevel->color().rgb())
  {
    columnColourWidth = ListViewParameters->columnWidth(COLUMN_COLOUR);
    QPixmap pm(ListViewParameters->width(), item->height() - 2);
    pm.fill(ColorButtonLevel->color());
    item->setPixmap(COLUMN_COLOUR,pm);
    ListViewParameters->setColumnWidth(COLUMN_COLOUR, columnColourWidth);
  }
  item->setText(COLUMN_RGB, QString::number(ColorButtonLevel->color().rgb()));
  item->setText(COLUMN_OPACITY, QString::number(SliderOpacity->value()));
  item->setText(COLUMN_TYPE, ComboBoxType->currentText());
  ListViewParameters->blockSignals(false);

  checkUpdate();
}

///// updateSettings //////////////////////////////////////////////////////////
void DensityBase::updateSettings()
/// Updates the settings with the contents of the currently
/// selected item in ListViewParameters.
{
  QListViewItem* item = ListViewParameters->selectedItem();
  if(item == 0)
    return;
  //qDebug("calling updateSettings");
  LineEditLevel->blockSignals(true);
  LineEditLevel->setText(item->text(COLUMN_LEVEL));
  LineEditLevel->blockSignals(false);
  SliderLevel->blockSignals(true);
  updateSliderLevel();
  SliderLevel->blockSignals(false);
  ColorButtonLevel->blockSignals(true);
  ColorButtonLevel->setColor(QColor(item->text(COLUMN_RGB).toUInt()));
  ColorButtonLevel->blockSignals(false);
  SliderOpacity->blockSignals(true);
  SliderOpacity->setValue(item->text(COLUMN_OPACITY).toUInt());
  SliderOpacity->blockSignals(false);
  updateOpacity();
  ComboBoxType->blockSignals(true);
  ComboBoxType->setCurrentText(item->text(COLUMN_TYPE));
  ComboBoxType->blockSignals(false);
}

///// updateVisibility ////////////////////////////////////////////////////////
void DensityBase::updateVisibility(QListViewItem* item, const QPoint&, int column)
/// Updates the visibility of a surface when a Checkbox is clicked.
{
  if(item == 0)
    return;
  if(column != 0)
    return;

  ///// the visibility of a surface was changed
  checkUpdate();
}

///// updateOperation /////////////////////////////////////////////////////////
void DensityBase::updateOperation(const unsigned int op)
/// Updates the possible operations and the DensityGrid.
/// Possible values of \c op :
/// \arg 0 : no density has changed, just the current item of ComboBoxOperation.
/// \arg 1 : a new density was read into densityPointsA.
/// \arg 2 : a new density was read into densityPointsB.
{
  double maxDensity = 0.0, minDensity = 0.0;

  ///// op = 0 => both densities are available and of the same size if currentItem > 1
  if(op == 0)
  {
    switch(ComboBoxOperation->currentItem())
    {
      case 0: // density A
              {
                std::vector<double>::iterator it = std::max_element(densityPointsA.begin(), densityPointsA.end());
                maxDensity = *it;
                it = std::min_element(densityPointsA.begin(), densityPointsA.end());
                minDensity = *it;
              }
              densityGrid->setParameters(&densityPointsA, numPointsA, deltaA, originA);
              break;
      case 1: // density B
              {
                std::vector<double>::iterator it = std::max_element(densityPointsB.begin(), densityPointsB.end());
                maxDensity = *it;
                it = std::min_element(densityPointsB.begin(), densityPointsB.end());
                minDensity = *it;
              }
              densityGrid->setParameters(&densityPointsB, numPointsB, deltaB, originB);
              break;
      case 2: // A + B
              {
                std::vector<double> densitySum(densityPointsA.size());
                std::transform(densityPointsA.begin(), densityPointsA.end(), densityPointsB.begin(), densitySum.begin(), std::plus<double>());
                std::vector<double>::iterator it = std::max_element(densitySum.begin(), densitySum.end());
                maxDensity = *it;
                it = std::min_element(densitySum.begin(), densitySum.end());
                minDensity = *it;
                densityGrid->setParameters(&densitySum, numPointsA, deltaA, originA);
              }
              break;
      case 3: // A - B
              {
                std::vector<double> densityDiff(densityPointsA.size());
                std::transform(densityPointsA.begin(), densityPointsA.end(), densityPointsB.begin(), densityDiff.begin(), std::minus<double>());
                std::vector<double>::iterator it = std::max_element(densityDiff.begin(), densityDiff.end());
                maxDensity = *it;
                it = std::min_element(densityDiff.begin(), densityDiff.end());
                minDensity = *it;
                densityGrid->setParameters(&densityDiff, numPointsA, deltaA, originA);
              }
              break;
      case 4: // B - A
              {
                std::vector<double> densityDiff(densityPointsA.size());
                std::transform(densityPointsB.begin(), densityPointsB.end(), densityPointsA.begin(), densityDiff.begin(), std::minus<double>());
                std::vector<double>::iterator it = std::max_element(densityDiff.begin(), densityDiff.end());
                maxDensity = *it;
                it = std::min_element(densityDiff.begin(), densityDiff.end());
                minDensity = *it;
                densityGrid->setParameters(&densityDiff, numPointsA, deltaA, originA);
              }
              break;
    }
  }
  ///// op = 1
  else if(op == 1)
  {
    if(densityPointsB.empty())
    {
      ///// no other density present
      ///// just update the first density
      ComboBoxOperation->setCurrentItem(0);
      updateOperation();
      return;
    }
    else if(identicalGrids())
    {
      ///// both densities are present and have the same size
      if(ComboBoxOperation->count() == 2)
      {
        ComboBoxOperation->insertItem(tr("Add densities (A + B)"));
        ComboBoxOperation->insertItem(tr("Substract densities (A - B)"));
        ComboBoxOperation->insertItem(tr("Substract densities (B - A)"));
      }
      ///// do not update if the current operation is the other density
      if(ComboBoxOperation->currentItem() != 1)
        updateOperation();
      return;
    }
    else
    {
      ///// both densities are present but with different grid sizes
      ///// if a combined operation was chosen, revert to the new density
      if(ComboBoxOperation->currentItem() > 2)
        ComboBoxOperation->setCurrentItem(0);
      ///// adjust the available items in the combobox
      while(ComboBoxOperation->count() > 2)
        ComboBoxOperation->removeItem(2);
      ///// do not update if the current operation is the other density
      if(ComboBoxOperation->currentItem() != 1)
        updateOperation();
      setSingleColor(); // can't use mapping when only a single density is present
      return;
    }
  }
  ///// op = 2
  else if(op == 2)
  {
    if(densityPointsA.empty())
    {
      ///// no other density present
      ///// just update the second density
      ComboBoxOperation->setCurrentItem(1);
      updateOperation();
      return;
    }
    else if(identicalGrids())
    {
      ///// both densities are present and have identical grids
      if(ComboBoxOperation->count() == 2)
      {
        ComboBoxOperation->insertItem(tr("Add densities (A + B)"));
        ComboBoxOperation->insertItem(tr("Substract densities (A - B)"));
        ComboBoxOperation->insertItem(tr("Substract densities (B - A)"));
        // Qt does not recompute the optimal horizontal size for ComboBoxOperation
        // when items are added or removed. That's why the following 2 lines are added as a hack
        // (from http://lists.trolltech.com/qt-interest/2002-05/thread00289-0.html)
        // -> only do this for enlarging, never for shrinking again
        ComboBoxOperation->setFont(ComboBoxOperation->font()); // invalidates sizeHint
        ComboBoxOperation->updateGeometry(); // recalculates sizeHint and re-layouts this widget
      }
      ///// do not update if the current operation is the other density
      if(ComboBoxOperation->currentItem() != 0)
        updateOperation();
      return;
    }
    else
    {
      ///// both densities are present but with different grids
      ///// if a combined operation was chosen, revert to the new density
      if(ComboBoxOperation->currentItem() > 2)
        ComboBoxOperation->setCurrentItem(1);
      ///// adjust the available items in the combobox
      while(ComboBoxOperation->count() > 2)
        ComboBoxOperation->removeItem(2);
      ///// do not update if the current operation is the other density
      if(ComboBoxOperation->currentItem() != 0)
        updateOperation();
      setSingleColor(); // can't use mapping when only a single density is present
      return;
    }
  }

  ///// arrived here, stuff needs to be updated
  ///// and the surfaces need to be reset

  ///// clear any defined surfaces with isoLevels outside minDensity and maxDensity
  //idCounter = 0;
  //ListViewParameters->clear();
  for(unsigned int i = surfaceProperties.size(); i > 0; i--)
  {
    if(surfaceProperties[i-1].level > maxDensity || surfaceProperties[i-1].level < minDensity)
    {
      if(!surfaceProperties[i-1].isNew)
        emit deletedSurface(i-1);
      // delete the corresponding listview item
      QListViewItem* item = ListViewParameters->firstChild();
      while(item)
      {
        if(item->text(COLUMN_ID).toUInt() == surfaceProperties[i-1].ID)
        {
          delete item;
          break;
        }
        item = item->nextSibling();
      }
      std::vector<SurfaceProperties>::iterator it = surfaceProperties.begin();
      it += i - 1;
      surfaceProperties.erase(it);
    }
  }
  ///// flag all remaining surfaces as new so they will get updated on the next update
  for(unsigned int i = 0; i < surfaceProperties.size(); i++)
    surfaceProperties[i].isNew = true;

  ///// update if requested
  checkUpdate();

  ///// update the type of density
  LabelMax->setText(QString::number(maxDensity,'f'));
  LabelMin->setText(QString::number(minDensity,'f'));
  ///// update the default settings
  SliderLevel->setMaxValue(static_cast<int>(maxDensity/deltaLevel));
  SliderLevel->setMinValue(static_cast<int>(minDensity/deltaLevel));
  ///// update the settings for defaults if no surfaces are already defined
  if(ListViewParameters->childCount() == 0)
  {
    if(maxDensity > 0.0)
    {
      const double defaultLevel = maxDensity < 0.05 ? maxDensity : 0.05;
      LineEditLevel->setText(QString::number(defaultLevel,'f',3));
      SliderLevel->setValue(static_cast<int>(defaultLevel/deltaLevel));
      ColorButtonLevel->setColor(QColor(0, 0, 255));
    }
    else
    {
      const double defaultLevel = minDensity > -0.05 ? minDensity : -0.05;
      LineEditLevel->setText(QString::number(defaultLevel,'f',3));
      SliderLevel->setValue(static_cast<int>(defaultLevel/deltaLevel));
      ColorButtonLevel->setColor(QColor(255, 0, 0));
    }
  }
  ///// update the maxima for volume rendering and slices
  resetVolumeMaxima();
  resetSliceMaxima();
  SliderSlice->setMaxValue(densityGrid->getNumPoints().x() + densityGrid->getNumPoints().y() + densityGrid->getNumPoints().z() - 1);
  SliderSlice->setTickInterval(SliderSlice->maxValue()/3);
  enableWidgets();
}

///// updateOpacity ///////////////////////////////////////////////////////////
void DensityBase::updateOpacity()
/// Updates LabelOpacity so its value corresponds with the position of
/// SliderOpacity.
{
  if(SliderOpacity->value() == 100)
    LabelOpacity->setText(QString::number(SliderOpacity->value()) + " %");
  else
    LabelOpacity->setText(" " + QString::number(SliderOpacity->value()) + " %");
}

///// setSingleColor //////////////////////////////////////////////////////////
void DensityBase::setSingleColor()
/// Sets the use of single colors for the visualization of the isosurfaces and
/// slices.
{
  // common stuff
  PushButtonSingleColor->setOn(true);
  PushButtonMapped->setOn(false);

  switch(ComboBoxVisualizationType->currentItem())
  {
    case ISOSURFACES:
      ColorButtonLevel->setEnabled(true);
      break;
    case SLICE:
      // show the single color widgets
      LabelSlicePos->show();
      LabelSliceNeg->show();
      LabelSliceBack->show();
      ColorButtonSlicePos->show();
      ColorButtonSliceNeg->show();
      ColorButtonSliceBack->show();
      CheckBoxSliceTransparent->show();
      // hide the  mapping widgets
      LabelSliceMap1->hide();
      LabelSliceMap2->hide();
      LabelSliceMap3->hide();
      ComboBoxSliceMap->hide();
      break;
  }
  checkUpdate();
}

///// setMapping //////////////////////////////////////////////////////////////
void DensityBase::setMapping()
/// Sets the use of another density for color mapping the isosurfaces or changes
/// the mapping options for slices.
{
  switch(ComboBoxVisualizationType->currentItem())
  {
    case ISOSURFACES:
    {
      // the source density
      QString currentDensity = mappingWidget->ComboBoxSource->currentText();
      mappingWidget->ComboBoxSource->clear();
      if(!densityPointsA.empty())
        mappingWidget->ComboBoxSource->insertItem(tr("Density A"));
      if(!densityPointsB.empty())
        mappingWidget->ComboBoxSource->insertItem(tr("Density B"));
      mappingWidget->ComboBoxSource->setCurrentItem(0);
      bool noChange = false;
      for(int i = 0; i < mappingWidget->ComboBoxSource->count(); i++)
      {
        if(currentDensity == mappingWidget->ComboBoxSource->text(i))
        {
          mappingWidget->ComboBoxSource->setCurrentItem(i);
          noChange = true;
        }
      }

      // the maximum plot values
      if(!noChange)
      {
        bool update = CheckBoxUpdate->isOn();
        CheckBoxUpdate->setChecked(false); // prevent automatic update
        resetMappedMaxima(); // the current source density has changed so reset to the maximum values
        CheckBoxUpdate->setChecked(update);
      }

      // save the original values
      const int oldDensity = mappingWidget->ComboBoxSource->currentItem();
      const QString oldMax = mappingWidget->LineEditMaxPos->text();
      const QString oldMin = mappingWidget->LineEditMaxNeg->text();
      const int oldMap = mappingWidget->ComboBoxMap->currentItem();

      // show it
      if(mappingWidget->exec() == QDialog::Rejected)
      {
        // reset to the old values and return
        mappingWidget->ComboBoxSource->setCurrentItem(oldDensity);
        mappingWidget->LineEditMaxPos->setText(oldMax);
        mappingWidget->LineEditMaxNeg->setText(oldMin);
        mappingWidget->ComboBoxMap->setCurrentItem(oldMap);
        if(PushButtonSingleColor->isOn())
          PushButtonMapped->setOn(false);
        return;
      }

      // update the DensityGrid
      std::vector<double>* points;
      if(mappingWidget->ComboBoxSource->currentText() == tr("Density A"))
        points = &densityPointsA;
      else
        points = &densityPointsB;
      densityGrid->setMappingParameters(points, mappingWidget->ComboBoxMap->currentItem(),
                                        mappingWidget->LineEditMaxPos->text().toFloat(),
                                        mappingWidget->LineEditMaxNeg->text().toFloat());
      mappingChanged = true;

      // update the isosurface's colors in the ListView
      columnColourWidth = ListViewParameters->columnWidth(COLUMN_COLOUR);
      QPixmap pm(*(mappingWidget->ComboBoxMap->pixmap(mappingWidget->ComboBoxMap->currentItem())));
      QListViewItemIterator it(ListViewParameters);
      if(it.current() && pm.height() > it.current()->height() - 2)
        pm.resize(pm.width(), it.current()->height() - 2); // assume all rows have equal height
      while(it.current())
      {
        it.current()->setPixmap(COLUMN_COLOUR, pm);
        it++;
      }
      ListViewParameters->setColumnWidth(COLUMN_COLOUR, columnColourWidth); // reset the column width

      // fix relevant button
      ColorButtonLevel->setEnabled(false);
    }

    case SLICE:
    {
      // hide the single color widgets
      LabelSlicePos->hide();
      LabelSliceNeg->hide();
      LabelSliceBack->hide();
      ColorButtonSlicePos->hide();
      ColorButtonSliceNeg->hide();
      ColorButtonSliceBack->hide();
      CheckBoxSliceTransparent->hide();
      // show mapping widgets
      LabelSliceMap1->show();
      LabelSliceMap2->show();
      LabelSliceMap3->show();
      ComboBoxSliceMap->show();
    }
  }

  ///// fix relevant buttons
  PushButtonMapped->setOn(true);
  PushButtonSingleColor->setOn(false);
  checkUpdate();
}

///// resetMappedMaxima ///////////////////////////////////////////////////////
void DensityBase::resetMappedMaxima()
/// Resets the given maxima in MappedSurfaceWidget to their original values.
{
  if(mappingWidget->ComboBoxSource->currentText() == tr("Density A"))
  {
    std::vector<double>::iterator it = std::max_element(densityPointsA.begin(), densityPointsA.end());
    mappingWidget->LineEditMaxPos->setText(QString::number(*it, 'f'));
    it = std::min_element(densityPointsA.begin(), densityPointsA.end());
    mappingWidget->LineEditMaxNeg->setText(QString::number(*it, 'f'));
  }
  else if(mappingWidget->ComboBoxSource->currentText() == tr("Density B"))
  {
    std::vector<double>::iterator it = std::max_element(densityPointsB.begin(), densityPointsB.end());
    mappingWidget->LineEditMaxPos->setText(QString::number(*it, 'f'));
    it = std::min_element(densityPointsB.begin(), densityPointsB.end());
    mappingWidget->LineEditMaxNeg->setText(QString::number(*it, 'f'));
  }
  checkUpdate();
}

///// resetVolumeMaxima ///////////////////////////////////////////////////////
void DensityBase::resetVolumeMaxima()
/// Resets the given maxima for Volume rendering
{
  LineEditVolumePos->setText(LabelMax->text());
  LineEditVolumeNeg->setText(LabelMin->text());
}

///// resetSliceMaxima ////////////////////////////////////////////////////////
void DensityBase::resetSliceMaxima()
/// Resets the given maxima for slices.
{
  LineEditSlicePos->setText(LabelMax->text());
  LineEditSliceNeg->setText(LabelMin->text());
}

///// checkUpdate /////////////////////////////////////////////////////////////
void DensityBase::checkUpdate()
/// Does in update only if automatic updates are enabled.
{
  if(CheckBoxUpdate->isOn())
    updateAll();
}

///////////////////////////////////////////////////////////////////////////////
///// Private Member Functions                                            /////
///////////////////////////////////////////////////////////////////////////////

///// makeConnections /////////////////////////////////////////////////////////
void DensityBase::makeConnections()
/// Sets up the permanent connections. Called once from the constructor.
{
  ///// connections for buttons
  connect(PushButtonLoadA, SIGNAL(clicked()), this, SLOT(loadDensityA()));
  connect(PushButtonLoadB, SIGNAL(clicked()), this, SLOT(loadDensityB()));
  connect(PushButtonSingleColor, SIGNAL(clicked()), this, SLOT(setSingleColor()));
  connect(PushButtonMapped, SIGNAL(clicked()), this, SLOT(setMapping()));
  connect(PushButtonAdd, SIGNAL(clicked()), this, SLOT(addSurface()));
  connect(PushButtonAdd2, SIGNAL(clicked()), this, SLOT(addSurfacePair()));
  connect(PushButtonDelete, SIGNAL(clicked()), this, SLOT(deleteSurface()));
  connect(PushButtonUpdate, SIGNAL(clicked()), this, SLOT(updateAll()));
  connect(PushButtonOK, SIGNAL(clicked()), this, SLOT(accept()));
  connect(PushButtonCancel, SIGNAL(clicked()), this, SLOT(reject()));

  ///// connections for settings
  connect(LineEditLevel, SIGNAL(textChanged(const QString&)), this, SLOT(updateSliderLevel()));
  connect(SliderLevel, SIGNAL(valueChanged(int)), this, SLOT(updateLineEditLevel()));
  connect(SliderOpacity, SIGNAL(valueChanged(int)), this, SLOT(updateOpacity()));

  ///// connections for ListViewParameters
  connect(LineEditLevel, SIGNAL(textChanged(const QString&)), this, SLOT(updateListView()));
  connect(ColorButtonLevel, SIGNAL(newColor(QColor*)), this, SLOT(updateListView()));
  connect(SliderOpacity, SIGNAL(valueChanged(int)), this, SLOT(updateListView()));
  connect(ComboBoxType, SIGNAL(activated(int)), this, SLOT(updateListView()));
  connect(ListViewParameters, SIGNAL(clicked(QListViewItem*)), this, SLOT(updateSettings()));
  connect(ListViewParameters, SIGNAL(clicked(QListViewItem*, const QPoint&, int)), this, SLOT(updateVisibility(QListViewItem*, const QPoint&, int)));

  ///// connections for ComboBoxOperation
  connect(ComboBoxOperation, SIGNAL(activated(int)), this, SLOT(updateOperation()));

  ///// connections for ComboBoxVisualizationType
  connect(ComboBoxVisualizationType, SIGNAL(activated(int)), this, SLOT(updateVisualizationType()));

  ///// connections for MappedSurfaceWidget
  connect(mappingWidget->PushButtonReset, SIGNAL(clicked()), this, SLOT(resetMappedMaxima()));
  connect(mappingWidget->PushButtonCancel, SIGNAL(clicked()), mappingWidget, SLOT(reject()));
  connect(mappingWidget->PushButtonOK, SIGNAL(clicked()), mappingWidget, SLOT(accept()));

  ///// connections for Volume
  connect(PushButtonVolumeReset, SIGNAL(clicked()), this, SLOT(resetVolumeMaxima()));
  connect(ColorButtonVolumePos, SIGNAL(newColor(QColor*)), this, SLOT(checkUpdate()));
  connect(ColorButtonVolumeNeg, SIGNAL(newColor(QColor*)), this, SLOT(checkUpdate()));
  connect(LineEditVolumePos, SIGNAL(textChanged(const QString&)), this, SLOT(checkUpdate()));
  connect(LineEditVolumeNeg, SIGNAL(textChanged(const QString&)), this, SLOT(checkUpdate()));

  ///// connections for Slice
  connect(PushButtonSliceReset, SIGNAL(clicked()), this, SLOT(resetSliceMaxima()));
  connect(ColorButtonSlicePos, SIGNAL(newColor(QColor*)), this, SLOT(checkUpdate()));
  connect(ColorButtonSliceNeg, SIGNAL(newColor(QColor*)), this, SLOT(checkUpdate()));
  connect(ColorButtonSliceBack, SIGNAL(newColor(QColor*)), this, SLOT(checkUpdate()));
  connect(CheckBoxSliceTransparent, SIGNAL(clicked()), this, SLOT(checkUpdate()));
  connect(ComboBoxSliceMap, SIGNAL(activated(int)), this, SLOT(checkUpdate()));
  connect(LineEditSlicePos, SIGNAL(textChanged(const QString&)), this, SLOT(checkUpdate()));
  connect(LineEditSliceNeg, SIGNAL(textChanged(const QString&)), this, SLOT(checkUpdate()));
  connect(SliderSlice, SIGNAL(valueChanged(int)), this, SLOT(checkUpdate()));
}

///// loadDensity /////////////////////////////////////////////////////////////
void DensityBase::loadDensity(const bool densityA)
/// Loads a new density into density A or B.
/// This depends on the value of \c densityA:
/// \arg true : density A.
/// \arg false : density B.
{
  loadingDensityA = densityA;

  ///// get the filename of a cube file to open
  QString dialogText = tr("Select an electron density grid file for density ");
  if(densityA)
    dialogText += "A";
  else
    dialogText += "B";
  QStringList filters = tr("All supported file types") + " (*.cube *.cub *.plt)";
  filters += "Potdicht/Gaussian CUBE (*.cube *.cub)";
  filters += "gOpenMol PLT (*.plt)";
  QString filename = QFileDialog::getOpenFileName(QString::null, filters.join(";;"), this, 0, dialogText);
  if(filename.isEmpty())
    return;
  QFile* file = new QFile(filename);
  if(!file->open(IO_ReadOnly))
  {
    delete file;
    QMessageBox::warning(this, tr("Load Density"), tr("Unable to open the grid file"));
    return;
  }
  ///// get the header information (MO to read, number of density points, origin and extents)
  const QString extension = filename.section(".",-1).lower();
  if(extension == "cube" || extension == "cub")
  {
    if(!loadCube(file))
    {
      delete file;
      QMessageBox::warning(this, tr("Load Density"), tr("An error occured reading the cube file"));
      return;
    } // if loading is succesful, deletion of file is taken care of by loadCubeThread
  }
  else
  {
    if(!loadPLT(file))
    {
      delete file;
      QMessageBox::warning(this, tr("Load Density"), tr("An error occured reading the PLT file"));
      return;
    } // if loading is succesful, deletion of file is taken care of by loadPLTThread
  }

  enableWidgets();
}

///// loadCube ////////////////////////////////////////////////////////////////
bool DensityBase::loadCube(QFile* file)
/// Reads and processes a Potdicht/Gaussian CUBE file.
{
  const double AUTOANG = 1.0/1.889726342;
  QTextStream stream(file);
  stream.readLine(); // ignore the first line
  newDescription = stream.readLine(); // the description of the type of density
  QString line = stream.readLine();
  const int numAtoms = line.mid(0,5).toInt();
  const float originX = line.mid(5,12).toFloat() * AUTOANG;
  const float originY = line.mid(17,12).toFloat() * AUTOANG;
  const float originZ = line.mid(29,12).toFloat() * AUTOANG;
  line = stream.readLine();
  const unsigned int numPointsX = line.mid(0,5).toUInt();
  const float deltaX = line.mid(5,12).toFloat() * AUTOANG;
  line = stream.readLine();
  const unsigned int numPointsY = line.mid(0,5).toUInt();
  const float deltaY = line.mid(17,12).toFloat() * AUTOANG;
  line = stream.readLine();
  const unsigned int numPointsZ = line.mid(0,5).toUInt();
  const float deltaZ = line.mid(29,12).toFloat() * AUTOANG;
  if(numPointsX == 0 || numPointsY == 0 || numPointsZ == 0 || stream.atEnd())
    return false;

  ///// skip the lines containing the coordinates
  for(int i = 0; i < abs(numAtoms); i++)
    stream.readLine();
  ///// read the list of MO's if numAtoms < 0
  QStringList listMO;
  if(numAtoms < 0)
  {
    unsigned int numMO, mo;
    stream >> numMO;
    qDebug("number of MO's present: %d", numMO);
	  for(unsigned int i = 0; i < numMO; i++)
    {
	    stream >> mo;
      listMO << QString::number(mo);
    }
  }
  if(stream.atEnd())
    return false;

  ///// ask which MO should be read and skip the initial values
  unsigned int numSkipValues = 0;
  if(listMO.size() > 1)
  {
    bool ok;
    QString result = QInputDialog::getItem(tr("Select the desired MO"), tr("The file contains multiple entries for\n")+newDescription+"\nSelect the desired molecular orbital", listMO,0,false,&ok,this);
    if(!ok)
      return false; // cancelled. Only when returning false, the QFile is deleted properly.
    newDescription += QString(" for MO " + result);
    double skipValue;
    for(QStringList::iterator it = listMO.begin(); it != listMO.end(); ++it)
    {
      // exit if the right MO is found
      if(*it == result)
        break;
      stream >> skipValue;
    }
    numSkipValues = listMO.size() - 1;
  }
  else if(listMO.size() == 1)
    newDescription += QString(" for MO " + listMO[0]);

  ///// read all density points in a LoadCubeThread (this class takes ownership of the opened file pointer)
  const unsigned int totalPoints = numPointsX * numPointsY * numPointsZ;
  if(loadingDensityA)
  {
    numPointsA.setValues(numPointsX, numPointsY, numPointsZ);
    originA.setValues(originX, originY, originZ);
    deltaA.setValues(deltaX, deltaY, deltaZ);
    ProgressBarA->setTotalSteps(totalPoints);
    ProgressBarA->setProgress(0);
    ProgressBarA->show();
    LabelDensityA->hide();
    loadingThread = new LoadCubeThread(&densityPointsA, file, this, totalPoints, numSkipValues);
  }
  else
  {
    numPointsB.setValues(numPointsX, numPointsY, numPointsZ);
    originB.setValues(originX, originY, originZ);
    deltaB.setValues(deltaX, deltaY, deltaZ);
    ProgressBarB->setTotalSteps(totalPoints);
    ProgressBarB->setProgress(0);
    ProgressBarB->show();
    LabelDensityB->hide();
    loadingThread = new LoadCubeThread(&densityPointsB, file, this, totalPoints, numSkipValues);
  }
  loadingThread->start(QThread::LowPriority);
  return true;
}

///// loadPLT /////////////////////////////////////////////////////////////////
bool DensityBase::loadPLT(QFile* file)
/// Reads and processes a gOpenMol PLT file.
{
  ///// Check the format of the PLT file (text, big endian binary or little endian binary)
  ///// by reading the magic number (=3)
  LoadPLTThread::Format pltFormat = LoadPLTThread::TextFormat;

  // first check whether it is in text format
  QTextStream textStream(file);
  QDataStream dataStream(file);
  Q_UINT32 pltCode;
  textStream >> pltCode;
  if(pltCode != 3)
  {
    // check which type of binary mode it is in (big or little endian mode)
    file->at(0);
    QDataStream dataStream(file); // default mode is big endian
    dataStream >> pltCode;
    if(pltCode == 3)
      pltFormat = LoadPLTThread::BigEndianFormat;
    else
    {
      // check little endian mode
      file->at(0);
      dataStream.setByteOrder(QDataStream::LittleEndian);
      dataStream >> pltCode;
      if(pltCode == 3)
        pltFormat = LoadPLTThread::LittleEndianFormat;
      else
        return false;
    }
  }
  //qDebug("format = %u (hex = %X)", pltFormat, pltFormat);
  //qDebug("header size in bytes: %d", file->at());

  // at this point pltFormat is set and both textStream and dataStream are
  // positioned after the first value.

  ///// read the rest of the header (mirrored functionality for both streams
  ///// as they have no class relationship)
  Q_UINT32 pltType;
  Q_UINT32 numPointsX, numPointsY, numPointsZ;
  float originX, originY, originZ, maxX, maxY, maxZ;

  if(pltFormat == LoadPLTThread::TextFormat)
  {
    // read the surface type
    textStream >> pltType;
    // read the number of points in the x, y and z directions
    textStream >> numPointsZ >> numPointsY >> numPointsX;
    // read the minimum and maximum extents of the cube
    textStream >> originZ >> maxZ >> originY >> maxY >> originX >> maxX;
  }
  else
  {
    if(pltFormat == LoadPLTThread::LittleEndianFormat)
      dataStream.setByteOrder(QDataStream::LittleEndian); // needed, somehow forgot the setting?

    // read the surface type
    dataStream >> pltType;
    qDebug("type = %u (hex = %X)", pltType, pltType);
    // read the number of points in the x, y and z directions
    dataStream >> numPointsZ >> numPointsY >> numPointsX;
    qDebug("numPoints = (%u, %u, %u)", numPointsX, numPointsY, numPointsZ);
    // read the minimum and maximum extents of the cube
    dataStream >> originZ >> maxZ >> originY >> maxY >> originX >> maxX;
    qDebug("origin = (%f, %f, %f)", originX, originY, originZ);
    qDebug("max = (%f, %f, %f)", maxX, maxY, maxZ);
    qDebug("delta = (%f, %f, %f)", (maxX-originX)/(numPointsX-1), (maxY-originY)/(numPointsY-1), (maxZ-originZ)/(numPointsZ-1));
  }

  switch(pltType)
  {
    case 1  : newDescription = tr("VSS density");
              break;
    case 2  : newDescription = tr("Orbital density");
              break;
    case 3  : newDescription = tr("Probe density");
              break;
    case 100: newDescription = tr("OpenMol density");
              break;
    case 200: newDescription = tr("Gaussian density");
              break;
    case 201: newDescription = tr("Jaguar density");
              break;
    case 202: newDescription = tr("Gamess density");
              break;
    case 203: newDescription = tr("Autodock density");
              break;
    case 204: newDescription = tr("Delphi/Insight density");
              break;
    case 205: newDescription = tr("Grid density");
              break;
    default:  newDescription = tr("unspecified density");
  }

  ///// update the correct density parameters
  if(loadingDensityA)
  {
    numPointsA.setValues(numPointsX, numPointsY, numPointsZ);
    originA.setValues(originX, originY, originZ);
    deltaA.setValues((maxX-originX)/(numPointsX-1), (maxY-originY)/(numPointsY-1), (maxZ-originZ)/(numPointsZ-1));
  }
  else
  {
    numPointsB.setValues(numPointsX, numPointsY, numPointsZ);
    originB.setValues(originX, originY, originZ);
    deltaB.setValues((maxX-originX)/(numPointsX-1), (maxY-originY)/(numPointsY-1), (maxZ-originZ)/(numPointsZ-1));
  }

  ///// read all density points in a LoadPLTThread (this class takes ownership of the opened file pointer)
  const unsigned int totalPoints = numPointsX * numPointsY * numPointsZ;
  QProgressBar* progress;
  QLabel* label;
  std::vector<double>* points;
  if(loadingDensityA)
  {
    progress = ProgressBarA;
    label = LabelDensityA;
    points = &densityPointsA;
  }
  else
  {
    progress = ProgressBarB;
    label = LabelDensityB;
    points = &densityPointsB;
  }
  progress->setTotalSteps(totalPoints);
  progress->setProgress(0);
  progress->show();
  label->hide();
  loadingThread = new LoadPLTThread(points, file, this, totalPoints, numPointsX, numPointsY, numPointsZ, pltFormat);
  loadingThread->start(QThread::LowPriority);
  return true;
}
///// updateDensity ///////////////////////////////////////////////////////////
void DensityBase::updateDensity()
/// Updates everything after a new density is loaded.
{
  ///// check whether loading was succesfull
  if(loadingThread == 0)
    return;

  if(!loadingThread->finished())
    loadingThread->wait(); // blocking wait

  if(!loadingThread->success())
  {
    delete loadingThread;
    loadingThread = 0;
    enableWidgets();
    return;
  }

  delete loadingThread;
  loadingThread = 0;

  ///// do not update if the number of points of the new density does not
  ///// equal the number of points of the other density
  if( (loadingDensityA && !densityPointsB.empty()) || (!loadingDensityA && !densityPointsA.empty())
      && !identicalGrids())
    QMessageBox::warning(this, tr("Load Density"), tr("The grid of the new density does not equal\nthat of the other density.\nCombinations or color mapping will not be allowed."));

  if(loadingDensityA)
  {
    ProgressBarA->setProgress(ProgressBarA->totalSteps());
    updateOperation(1);
    LabelDensityA->setText(newDescription);
    ProgressBarA->hide();
    LabelDensityA->show();
  }
  else
  {
    ProgressBarB->setProgress(ProgressBarB->totalSteps());
    updateOperation(2);
    LabelDensityB->setText(newDescription);
    ProgressBarB->hide();
    LabelDensityB->show();
  }
  enableWidgets();
}

///// updateProgress //////////////////////////////////////////////////////////
void DensityBase::updateProgress(const unsigned int progress)
/// Updates the progressbar of the currently loading  density.
{
  if(loadingDensityA)
    ProgressBarA->setProgress(progress);
  else
    ProgressBarB->setProgress(progress);
}

///// typeToNum ///////////////////////////////////////////////////////////////
unsigned int DensityBase::typeToNum(const QString& type)
/// Returns the number corresponding to a type string.
/// Possible input strings:
/// \arg Solid
/// \arg Wireframe
/// \arg Dots
{
  if(type == tr("Solid"))
    return 0;
  else if(type == tr("Wireframe"))
    return 1;
  else if(type == tr("Dots"))
    return 2;
  return 3;
}

///// enableWidgets ///////////////////////////////////////////////////////////
void DensityBase::enableWidgets()
/// Enables/disables all widgets depending on the
/// current status of the class.
{
  ///// disable loading buttons when a new file is being loaded
  if(loadingThread != 0)
  {
    PushButtonLoadA->setEnabled(false);
    PushButtonLoadB->setEnabled(false);
    ComboBoxOperation->setEnabled(false);
  }
  else
  {
    PushButtonLoadA->setEnabled(true);
    PushButtonLoadB->setEnabled(true);
    ComboBoxOperation->setEnabled(!densityPointsA.empty() && !densityPointsB.empty());
  }

  ///// disable widgets if no density is loaded
  const bool hasDensity = densityGrid->densityPresent();
  ButtonGroupVisualizationType->setEnabled(hasDensity);
  ListViewParameters->setEnabled(hasDensity);
  PushButtonAdd->setEnabled(hasDensity);
  PushButtonUpdate->setEnabled(hasDensity);
  CheckBoxUpdate->setEnabled(hasDensity);

  ///// only enable the mapping functionality if 2 identical grids are present for isosurfaces or when slices are selected
  PushButtonMapped->setEnabled((ComboBoxVisualizationType->currentItem() == ISOSURFACES && hasDensity && identicalGrids())
                               //|| ComboBoxVisualizationType->currentItem() == SLICE);
                               || ComboBoxVisualizationType->currentItem() != ISOSURFACES);

  ///// only enable the 'Add pair' widget if the density contains both positive and negative values
  PushButtonAdd2->setEnabled(hasDensity && LabelMax->text().stripWhiteSpace().toDouble() > deltaLevel && LabelMin->text().stripWhiteSpace().toDouble() < -deltaLevel);

  ///// enable/disable widgets that are only available when surfaces are defined
  PushButtonDelete->setEnabled(hasDensity && ListViewParameters->childCount() != 0);
  GroupBoxSettings->setEnabled(hasDensity && ListViewParameters->childCount() != 0);
}

///// identicalGrids //////////////////////////////////////////////////////////
bool DensityBase::identicalGrids()
/// Returns true if the densities A and B are located
/// on the same grid. If true these densities can be succesfully combined.
{

  if(densityPointsA.size() != densityPointsB.size())
    qDebug("grids are not identical because sizes differ: %d and %d",densityPointsA.size(),densityPointsB.size());

  if(!(originA == originB))
    qDebug("grids are not identical because origins differ: A(%f,%f,%f) and B(%f,%f,%f)",originA.x(),originA.y(),originA.z(),originB.x(),originB.y(),originB.z());

  if(!(numPointsA == numPointsB))
    qDebug("grids are not identical because numPoints differ: A(%d,%d,%d) and B(%d,%d,%d)",numPointsA.x(),numPointsA.y(),numPointsA.z(),numPointsB.x(),numPointsB.y(),numPointsB.z());

  if(!(deltaA == deltaB))
    qDebug("grids are not identical because deltas differ: A(%f,%f,%f) and B(%f,%f,%f)",deltaA.x(),deltaA.y(),deltaA.z(),deltaB.x(),deltaB.y(),deltaB.z());

  return densityPointsA.size() == densityPointsB.size() && originA == originB && numPointsA == numPointsB && deltaA == deltaB;
}

///// updateIsoSurfaces ///////////////////////////////////////////////////////
bool DensityBase::updateIsoSurfaces()
/// Updates all changes concerning isosurfaces
{
  bool somethingChanged = false;

  ///// first traverse the surfaces backwards to remove deleted ones
  std::vector<SurfaceProperties>::reverse_iterator rit = surfaceProperties.rbegin();
  unsigned int surfaceIndex = surfaceProperties.size() - 1;
  while(rit != surfaceProperties.rend())
  {
    if((*rit).deleted)
    {
      if(!(*rit).isNew)
      {
        densityGrid->removeSurface(surfaceIndex);
        emit deletedSurface(surfaceIndex);
        somethingChanged = true;
      }
      ///// delete the current surface but first change rit
      std::vector<SurfaceProperties>::iterator itd = surfaceProperties.begin();
      itd += surfaceIndex; // now itd should be equal to rit
      assert((*itd).ID == (*rit).ID);
      rit++;
      surfaceProperties.erase(itd);
    }
    else
      rit++;
    surfaceIndex--;
  }

  ///// traverse the surfaces forward to update them if necessary
  QListViewItemIterator it(ListViewParameters);
  for(unsigned int i = 0; i < surfaceProperties.size(); i++, it++)
  {
    if(surfaceProperties[i].isNew == true)
    {
      // this is a new surface so everything must be updated
      surfaceProperties[i].visible = dynamic_cast<QCheckListItem*>(it.current())->isOn();
      surfaceProperties[i].level = it.current()->text(COLUMN_LEVEL).stripWhiteSpace().toDouble();
      surfaceProperties[i].colour = it.current()->text(COLUMN_RGB).toUInt();
      surfaceProperties[i].opacity = it.current()->text(COLUMN_OPACITY).toUInt();
      surfaceProperties[i].type = typeToNum(it.current()->text(COLUMN_TYPE));
      surfaceProperties[i].isNew = false;

      densityGrid->addSurface(surfaceProperties[i].level);
      emit newSurface(densityGrid->numSurfaces() - 1);
      somethingChanged = true;
    }
    else
    {
      ///// check whether anything has changed
      bool visibilityChanged = false;
      bool levelChanged = false;
      bool colorChanged = false;
      bool opacityChanged = false;
      bool typeChanged = false;
      if(dynamic_cast<QCheckListItem*>(it.current())->isOn() != surfaceProperties[i].visible)
        visibilityChanged = true;
      if(fabs(it.current()->text(COLUMN_LEVEL).stripWhiteSpace().toDouble() - surfaceProperties[i].level) >= deltaLevel*deltaLevel)
        levelChanged = true;
      if(it.current()->text(COLUMN_RGB).toUInt() != surfaceProperties[i].colour)
        colorChanged = true;
      if(it.current()->text(COLUMN_OPACITY).toUInt() != surfaceProperties[i].opacity)
        opacityChanged = true;
      if(typeToNum(it.current()->text(COLUMN_TYPE)) != surfaceProperties[i].type)
        typeChanged = true;
      ///// update the data
      surfaceProperties[i].visible = dynamic_cast<QCheckListItem*>(it.current())->isOn();
      surfaceProperties[i].level = it.current()->text(COLUMN_LEVEL).stripWhiteSpace().toDouble();
      surfaceProperties[i].colour = it.current()->text(COLUMN_RGB).toUInt();
      surfaceProperties[i].opacity = it.current()->text(COLUMN_OPACITY).toUInt();
      surfaceProperties[i].type = typeToNum(it.current()->text(COLUMN_TYPE));

      if(levelChanged)
        densityGrid->changeSurface(i, surfaceProperties[i].level);
      if(levelChanged || colorChanged || opacityChanged || typeChanged || mappingChanged)
      {
        emit updatedSurface(i);
        somethingChanged = true;
      }
      else if(visibilityChanged)
        somethingChanged = true;
    }
  }
  mappingChanged = false;
  return somethingChanged;
}

///// updateVolume ////////////////////////////////////////////////////////////
bool DensityBase::updateVolume()
/// Updates all changes concerning volumetric rendering.
{
  // check for changes
  bool changed = ColorButtonVolumePos->color().rgb() != volumeProperties.positiveColor ||
                 ColorButtonVolumeNeg->color().rgb() != volumeProperties.negativeColor ||
                 LineEditVolumePos->text() != volumeProperties.maxLevel ||
                 LineEditVolumeNeg->text() != volumeProperties.minLevel;

  // notify that the volume needs to be updated
  if(changed)
    emit updatedVolume();

  // save the changes
  if(changed)
  {
    volumeProperties.positiveColor = ColorButtonVolumePos->color().rgb();
    volumeProperties.negativeColor = ColorButtonVolumeNeg->color().rgb();
    volumeProperties.maxLevel = LineEditVolumePos->text();
    volumeProperties.minLevel = LineEditVolumeNeg->text();
  }

  return changed;
}

///// updateSlice /////////////////////////////////////////////////////////////
bool DensityBase::updateSlice()
/// Updates all changes concerning slices.
{
  // check for changes
  bool changed = ColorButtonSlicePos->color().rgb() != sliceProperties.positiveColor ||
                 ColorButtonSliceNeg->color().rgb() != sliceProperties.negativeColor ||
                 ColorButtonSliceBack->color().rgb() != sliceProperties.backgroundColor ||
                 CheckBoxSliceTransparent->isChecked() != sliceProperties.transparent ||
                 ComboBoxSliceMap->currentItem() != static_cast<int>(sliceProperties.map) ||
                 SliderSlice->value() != static_cast<int>(sliceProperties.index);

  // notify that the slice needs to be updated
  if(changed)
    emit updatedSlice();

  // save the changes
  if(changed)
  {
    sliceProperties.positiveColor = ColorButtonSlicePos->color().rgb();
    sliceProperties.negativeColor = ColorButtonSliceNeg->color().rgb();
    sliceProperties.backgroundColor = ColorButtonSliceBack->color().rgb();
    sliceProperties.transparent = CheckBoxSliceTransparent->isChecked();
    sliceProperties.map = ComboBoxSliceMap->currentItem();
    sliceProperties.index = SliderSlice->value();
  }

  return changed;
}


///////////////////////////////////////////////////////////////////////////////
///// Static Variables                                                    /////
///////////////////////////////////////////////////////////////////////////////

const double DensityBase::deltaLevel = 0.001;

