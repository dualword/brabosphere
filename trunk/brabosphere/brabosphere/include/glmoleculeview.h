/***************************************************************************
                       glmoleculeview.h  -  description
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

/// \file
/// Contains the declaration of the class GLMoleculeView

#ifndef GLMOLECULEVIEW_H
#define GLMOLECULEVIEW_H

///// Forward class declarations & header files ///////////////////////////////

// STL includes
#include <list>
#include <vector>

// Xbrabo forward class declarations
class AtomSet;
class DensityGrid;
class DensityBase;
class NewAtomBase;

// Base class header file
#include <glsimplemoleculeview.h>

///// class GLMoleculeView ////////////////////////////////////////////////////
class GLMoleculeView : public GLSimpleMoleculeView
{
  Q_OBJECT

  public:
    ///// constructor/destructor
    GLMoleculeView(AtomSet* atomset, QWidget* parent = 0, const char* name = 0);// constructor
    ~GLMoleculeView();                  // destructor

    ///// public member functions
    void setAtomSet(AtomSet* atomSet);  // updates the AtomSet pointer to a new set.
    bool alterCartesian();              // alters the cartesian coordinates of the selected atoms
    bool alterInternal();               // alters the internal coordinates formed by the selected atoms
    bool deleteSelectedAtoms();         // deletes the selected atoms
    
    ///// public structs
    struct GLTextureParameters
    /// A struct containing all the OpenGL parameters pertaining to texturing.
    {
      int maximumSize;                  ///< The maximum size of a 2D texture (should be a power of 2)
    };

    ///// static public member functions
    static void toggleSelectionMode();  // Toggles the manipulation target
    static void setParameters(GLTextureParameters params);  // sets new OpenGL texture parameters

  signals:
    void atomsetChanged();              ///< Is emitted when the number of atoms has been changed

  public slots:
    ///// Command related public slots
    void alterCartesianCommand();       // creates a Command to alter to cartesian coordinates of the selected atoms
    void alterInternalCommand();        // creates a Command to alter the value of the selected internal coordinate
    void deleteSelectedAtomsCommand();  // creates a Command to delete all selected atoms
    void selectAllCommand();            // creates a Command to select all atoms.
    void unselectAllCommand();          // creates a Command to deselect all atoms.
    void centerViewCommand();           // creates a Command to center the view
    void resetOrientationCommand();     // creates a Command to reset the orientation of the view
    void zoomFitCommand();              // creates a Command to zoom the scene for a perfect fit
    void resetViewCommand();            // creates a Command to reset the entire view

    ///// other slots
	  void showDensity();                 // shows electron density isosurfaces 
    void addAtoms();                    // adds atoms using a dialog

  protected:
    virtual void initializeGL();        // initial OpenGL setup
    void mouseMoveEvent(QMouseEvent* e);// event which takes place when the mouse is moved while a mousebutton is pressed
    void keyPressEvent(QKeyEvent* e);   // event which takes places when a key is pressed
    virtual void updateShapes();        // updates the shapes vector
    void processSelectionCommand(const unsigned int id);      // creates a Command to call processSelection
    void translateCommand(const int amountX, const int amountY, const int amountZ);       // creates a Command to translate the scene
    void rotateCommand(const float amountX, const float amountY, const float amountZ);    // creates a Command to rotate the scene
    void translateSelectionCommand(const int amountX, const int amountY, const int amountZ);        // creates a Command to translate the selected atoms
    void rotateSelectionCommand(const double amountX, const double amountY, const double amountZ);  // creates a Command to rotate the selected atoms
    void changeSelectedICCommand(const int range);// creates a Command to change the selected internal coordinate

private slots:
    void addGLSurface(const unsigned int index);  // adds a surface to the GL display list
    void updateGLSurface(const unsigned int index);         // updates an existing surface GL display list 
    void deleteGLSurface(const unsigned int index);         // deletes an existing surface GL display list
    void updateScene();                 // does the necessary updating when something changed in DensityBase
    void updateVolume();                // updates the textures used for volume rendering
    void updateSlice();                 // updates the current slice

  private:
    friend class CommandCoordinates;
    friend class CommandReadCoordinates;
    friend class CommandTranslateSelectionXY;
    friend class CommandTranslateSelectionZ;
    friend class CommandRotateSelection;
    friend class CommandChangeIC;
    friend class CommandSelection;
    friend class CommandSelectEntity;
    friend class CommandTranslation;
    friend class CommandTranslateXY;
    friend class CommandCenterView;
    friend class CommandZoom;
    friend class CommandZoomFit;
    friend class CommandTranslateZ;
    friend class CommandRotation;
    friend class CommandRotate;
    friend class CommandResetOrientation;
    friend class CommandResetView;

    ///// private enums
    enum ShapeTypesExtra{SHAPE_SURFACE = SHAPE_NEXT, SHAPE_VOLUME, SHAPE_SLICE};///< extention of GLSimpleMoleculeView::ShapeTypes
    enum Directions{DIRECTION_NONE, DIRECTION_POSX, DIRECTION_NEGX, DIRECTION_POSY, DIRECTION_NEGY, DIRECTION_POSZ, DIRECTION_NEGZ};        // directions for volumetric rendering

    ///// private member functions
    float boundingSphereRadius();      // calculates the radius of the bounding sphere
    bool translateSelection(const int xRange, const int yRange, const int zRange);        // translates the selected atoms according to the current view
    bool rotateSelection(const double angleX, const double angleY, const double angleZ);  // rotates the selected atoms around their local center of mass
    bool changeSelectedIC(const int range);       // changes the selected internal coordinate
    void drawItem(const unsigned int index);      // draws the item shapes[index]
    void drawSurface(const unsigned int index);   // draws an isosurface
    void drawVolume();                  // draws a grid with volumetric rendering
    void drawSlice();                   // draws a slice
    unsigned int getDirection() const;  // gets the current viewing direction
    QImage glSlice(const QImage& image) const;    // resizes an image according to the maximum slice size settings and OpenGL limitations
    
    ///// private member data   
    DensityGrid* densityGrid;           ///< An isodensity surface.
    DensityBase* densityDialog;         ///< A dialog for changing the isodensity surfaces.
    NewAtomBase* newAtomDialog;         ///< A dialog for adding atoms to the atomset
    std::vector<GLuint> glSurfaces;     ///< A vector that holds the GL display list indices for surfaces.
    GLuint volumeObjects;               ///< Holds the start index of the first GL display list for volume rendering textures.
    unsigned int numVolumeObjects;      ///< Holds the number of allocated display lists for texturing.
    GLuint sliceObject;                 ///< Holds the OpenGL display list number for slices

    ///// static private member data
    static bool manipulateSelection;    ///< If true, only the selected atoms are manipulated instead of the entire system.
    static GLTextureParameters textureParameters; ///< Holds the OpenGL texturing parameters
};
   
#endif

