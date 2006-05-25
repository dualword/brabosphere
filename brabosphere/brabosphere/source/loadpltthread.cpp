/***************************************************************************
                       loadpltthread.cpp  -  description
                             -------------------
    begin                : Wed May 24 2006
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
  \class LoadPLTThread
  \brief This class loads the density data from a PLT file.

  It is passed a QFile file pointer and takes ownership of this file.
*/
/// \file
/// Contains the implementation of the class LoadPLTThread.

///// Header files ////////////////////////////////////////////////////////////

// C++ header files
#include <cassert>

// Qt header files
#include <qapplication.h>
#include <qdatastream.h>
#include <qevent.h>
#include <qfile.h>
#include <qtextstream.h>

// Xbrabo header files
#include "densitybase.h"
#include "loadpltthread.h"

///////////////////////////////////////////////////////////////////////////////
///// Public Member Functions                                             /////
///////////////////////////////////////////////////////////////////////////////

///// Constructor /////////////////////////////////////////////////////////////
LoadPLTThread::LoadPLTThread(std::vector<double>* densityPoints, QFile* file, DensityBase* densityDialog, const unsigned int totalPoints, const unsigned int nPointsX, const unsigned int nPointsY, const unsigned int nPointsZ, const unsigned int format) 
  : LoadDensityThread(densityPoints, file, densityDialog, totalPoints),
  numPointsX(nPointsX),
  numPointsY(nPointsY),
  numPointsZ(nPointsZ),
  pltFormat(format)
/// The default constructor.
/// \param[in] format : the format of the file (text/binary)
/// This parameter apparently cannot be of type (enum) 'Format' as that gives 
/// cryptic errors during compilation.
{
  assert(numPointsX > 0);
  assert(numPointsY > 0);
  assert(numPointsZ > 0);
  assert(pltFormat <= LittleEndianFormat);
}

///// Destructor //////////////////////////////////////////////////////////////
LoadPLTThread::~LoadPLTThread()
/// The default destructor.
{

}

///// run /////////////////////////////////////////////////////////////////////
void LoadPLTThread::run()
/// Does the actual reading after the proper parameters
/// have been set. It is run with a call to start().
{  

  // initialisation
  data->clear();
  data->reserve(numValues);
  const unsigned int updateFreq = numValues/100;
  float value = 0.0;

  // read all grid points 
  if(pltFormat == TextFormat)
  {
    // create a text stream on the opened file (should be positioned right after the header
    // read in DensityBase::loadPLT)
    QTextStream stream(gridFile);
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
      if(stopRequested || stream.atEnd())
        break;
    }
  }
  else
  {
    // create a data stream on the opened file (should be positioned right after the header
    // read in DensityBase::loadPLT)
    QDataStream stream(gridFile);
    if(pltFormat == LittleEndianFormat)
      stream.setByteOrder(QDataStream::LittleEndian);
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
      if(stopRequested || stream.atEnd())
        break;
    }
  }

  // cleanup
  delete gridFile;
  
  // cleanup if stopped prematurely
  if(data->size() != numValues)
  {
    qDebug("number of values read = %d, should have been %d", data->size(), numValues);
    data->clear();
  }
  else
  {
    // reshuffle the density points: PLT format varies x the fastest, whereas here the CUBE convention
    // is used: z varies the fastest.
    std::vector<double> shuffledDensity(numValues);
    std::vector<double>::iterator it = data->begin();
    for(unsigned int z = 0; z < numPointsZ; z++)
      for(unsigned int y = 0; y < numPointsY; y++)
        for(unsigned int x = 0; x < numPointsX; x++)
          shuffledDensity[x*numPointsY*numPointsZ + y*numPointsZ + z] = *it++;
    data->assign(shuffledDensity.begin(), shuffledDensity.end());
  }

  // notify the thread has ended
  QCustomEvent* e = new QCustomEvent(static_cast<QEvent::Type>(1002));
  QApplication::postEvent(parent, e);
}
