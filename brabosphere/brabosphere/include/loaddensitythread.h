/***************************************************************************
                     loaddensitythread.h  -  description
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

/// \file
/// Contains the declaration of the class LoadDensityThread.

#ifndef LOADDENSITYTHREAD_H
#define LOADDENSITYTHREAD_H

///// Forward class declarations & header files ///////////////////////////////

// Qt forward class declarations
class QFile;

// Xbrabo forward class declarations
class DensityBase;

// Base class header files
#include <qthread.h>

///// class LoadDensityThread /////////////////////////////////////////////////
class LoadDensityThread : public QThread
{
  public:
    ///// constructor/destructor
    LoadDensityThread(std::vector<double>* densityPoints, QFile* file, DensityBase* densityDialog, const unsigned int totalPoints);         // constructor
    ~LoadDensityThread();               // destructor
  
    ///// pure virtuals
    virtual void run() = 0;             // reimplementation of this pure virtual in a subclass does the actual work
    
    ///// other public member functions
    void stop();                        // requests stopping the thread
    bool success();                     // returns true if everything loaded succesfully

  protected:
    ///// protected member data
    std::vector<double>* data;          ///< The pointer to the recipient for the data.
    unsigned int numValues;             ///< The total number of values to read. 
    QFile* gridFile;                    ///< The pointer to the grid file.
    bool stopRequested;                 ///< Is set to true if the thread should be stopped.
    DensityBase* parent;                ///< The widget which should get notifications.
    unsigned int progress;              ///< Used to transfer the progress to the parent dialog.

};
