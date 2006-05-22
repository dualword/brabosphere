/***************************************************************************
                     loaddensitythread.cpp  -  description
                             -------------------
    begin                : Mon May 22 2006
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
  \class LoadDensityThread
  \brief This is a base class for loading 3D electron density grid files.
*/
/// \file
/// Contains the implementation of the class LoadDensityThread.

///// Header files ////////////////////////////////////////////////////////////

// C++ header files
#include <cassert>

// Xbrabo header files
#include "densitybase.h"
#include "loaddensitythread.h"

///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// Constructor /////////////////////////////////////////////////////////////
LoadDensityThread::LoadDensityThread(std::vector<double>* densityPoints, QFile* file, DensityBase* densityDialog, const unsigned int totalPoints) 
  : QThread(), 
  data(densityPoints), 
  numValues(totalPoints),
  gridFile(file), 
  stopRequested(false),
  parent(densityDialog),
  progress(0)
/// The default constructor.
/// \param[out] densityPoints : the resulting density values read from file.
/// \param[in] totalPoints : the total number of points to read.
/// \param[in] file : a pointer to an opened grid file.
/// \param[in] densityDialog : the parent DensityBase widget were messages are sent to.
{
  assert(data != 0);
  assert(numValues > 0);
  assert(gridFile != 0);
  assert(parent != 0);
}

///// stop ////////////////////////////////////////////////////////////////////
void LoadDensityThread::stop()
/// Requests the thread to stop.
{
  stopRequested = true;
}

///// success /////////////////////////////////////////////////////////////////
bool LoadDensityThread::success()
/// Returns whether the desired number of points was succesfully read.  
{
  return data->size() == numValues;
}