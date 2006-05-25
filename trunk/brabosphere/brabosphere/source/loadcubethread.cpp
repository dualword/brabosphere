/***************************************************************************
                      loadcubethread.cpp  -  description
                             -------------------
    begin                : Thu Mar 24 2005
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
  \class LoadCubeThread
  \brief This class loads the density data from a CUBE file.

  It is passed a QFile file pointer and takes ownership of this file.
*/
/// \file
/// Contains the implementation of the class LoadCubeThread.

///// Header files ////////////////////////////////////////////////////////////

// C++ header files
#include <cassert>

// Qt header files
#include <qapplication.h>
#include <qevent.h>
#include <qfile.h>
#include <qtextstream.h>

// Xbrabo header files
#include "densitybase.h"
#include "loadcubethread.h"

///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// Constructor /////////////////////////////////////////////////////////////
LoadCubeThread::LoadCubeThread(std::vector<double>* densityPoints, QFile* file, DensityBase* densityDialog, const unsigned int totalPoints, const unsigned int numSkipValues) 
  : LoadDensityThread(densityPoints, file, densityDialog, totalPoints), 
  numSkip(numSkipValues)
/// The default constructor.
/// \param[in] numSkipValues : the number of points to skip when reading from the stream.
{
  assert(numSkipValues < totalPoints);
}

///// Destructor //////////////////////////////////////////////////////////////
LoadCubeThread::~LoadCubeThread()
/// The default destructor.
{

}

///// run /////////////////////////////////////////////////////////////////////
void LoadCubeThread::run()
/// Does the actual reading after the proper parameters
/// have been set. It is run with a call to start().
{  
  // create a text stream on the opened file (should be positioned right after the header
  // read in DensityBase::loadCube)
  QTextStream stream(gridFile);

  // initialisation
  data->clear();
  data->reserve(numValues);
  const unsigned int updateFreq = numValues/100;
  double value = 0.0;

  // read all grid points
  for(unsigned int i = 0; i < numValues; i++)
  {
    // read the next density point
    stream >> value;
	  data->push_back(value);
    if(i % updateFreq == 0)
    {
      progress = i;
      QCustomEvent* e = new QCustomEvent(static_cast<QEvent::Type>(1001),&progress);
      QApplication::postEvent(parent, e);
    }
    // skip the next numSkipValues density points from other MO's
    for(unsigned int skip = 0; skip < numSkip; skip++)
      stream >> value;
    if(stopRequested || stream.atEnd())
      break;
  }

  // cleanup if stopped prematurely
  if(data->size() != numValues)
    data->clear();

  // cleanup
  delete gridFile;

  // notify the thread has ended
  QCustomEvent* e = new QCustomEvent(static_cast<QEvent::Type>(1002));
  QApplication::postEvent(parent, e);
}



