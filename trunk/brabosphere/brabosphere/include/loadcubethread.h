/***************************************************************************
                       loadcubethread.h  -  description
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

/// \file
/// Contains the declaration of the class LoadCubeThread.

#ifndef LOADCUBETHREAD_H
#define LOADCUBETHREAD_H

///// Forward class declarations & header files ///////////////////////////////

// STL includes
#include <vector>

// Xbrabo forward class declarations
class DensityBase;

// Base class header files
#include "loaddensitythread.h"

///// class LoadCubeThread ////////////////////////////////////////////////////
class LoadCubeThread : public LoadDensityThread
{
  public:
    ///// constructor/destructor
    LoadCubeThread(std::vector<double>* densityPoints, QFile* file, DensityBase* densityDialog, const unsigned int totalPoints, const unsigned int numSkipValues);        // constructor
    ~LoadCubeThread();               // destructor

    ///// pure virtuals
    virtual void run();                 // reimplementation of this pure virtual does the actual work
    
  private:
    ///// private member data
    unsigned int numSkip;               ///< The number of values to skip at each read.
};

#endif

