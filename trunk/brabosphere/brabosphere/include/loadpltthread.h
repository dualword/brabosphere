/***************************************************************************
                        loadpltthread.h  -  description
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

/// \file
/// Contains the declaration of the class LoadPLTThread.

#ifndef LOADPLTTHREAD_H
#define LOADPLTTHREAD_H

///// Forward class declarations & header files ///////////////////////////////

// STL includes
//#include <vector>

// Qt forward class declarations
class QFile;

// Xbrabo forward class declarations
class DensityBase;

// Base class header files
#include "loaddensitythread.h"

///// class LoadPLTThread /////////////////////////////////////////////////////
class LoadPLTThread : public LoadDensityThread
{
  public:
    ///// constructor/destructor
    LoadPLTThread(std::vector<double> * densityPoints, QFile* file, DensityBase* densityDialog, const unsigned int totalPoints, const unsigned int nPointsX, const unsigned int nPointsY, const unsigned int nPointsZ, const unsigned int format);  // constructor
    ~LoadPLTThread();               // destructor

    ///// public enums
    enum Format{TextFormat, BigEndianFormat, LittleEndianFormat};     // The possible file formats for PLT files

    ///// pure virtuals
    virtual void run();                 // reimplementation of this pure virtual does the actual work
    
  private:
    ///// private member data
    unsigned int numPointsX;            ///< The number of data points in the X-direction, needed for the reshuffling phase
    unsigned int numPointsY;            ///< The number of data points in the Y-direction, needed for the reshuffling phase
    unsigned int numPointsZ;            ///< The number of data points in the Z-direction, needed for the reshuffling phase
    unsigned int pltFormat;             ///< The format of the file to be read
};

#endif

