/***************************************************************************
                         densitybase.h  -  description
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

/// \file
/// Contains the declaration of the class DensityBase

#ifndef DENSITYBASE_H
#define DENSITYBASE_H

///// Forward class declarations & header files ///////////////////////////////

// STL includes
#include <vector>

// Qt forward class declarations
class QFile;

// Xbrabo forward class declarations
class DensityGrid;
class LoadDensityThread;
class MappedSurfaceWidget;

// Xbrabo includes
#include <point3d.h>

// Base class header files
#include "densitywidget.h"

///// class DensityBase ///////////////////////////////////////////////////////
class DensityBase : public DensityWidget
{
  Q_OBJECT

  public:
    ///// constructor/destructor
    DensityBase(DensityGrid* grid, QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0);       // constructor
    ~DensityBase();                     // destructor

    ///// public enums
    enum VisType{ISOSURFACES = 0, VOLUME, SLICE, NO_VISUALIZATION};   ///< For returning the type of visualization

    ///// public member function for retrieving data
    unsigned int visualizationType() const;       // returns the active type of visualization
    bool surfaceVisible(const unsigned int surface) const;        // returns true if a surface is visible
    bool surfaceMapping() const;        // returns whether a color map is in use for the isosurfaces
    QColor surfaceColor(const unsigned int surface) const;  // returns the color of a surface
    unsigned int surfaceOpacity(const unsigned int surface) const;    // returns the opacity of a surface
    unsigned int surfaceType(const unsigned int surface) const;       // returns the drawing type of a surface

  signals:
    void newSurface(const unsigned int surface);  // is emitted after a new surface is created
    void updatedSurface(const unsigned int surface);        // is emitted when a surface has changed
    void deletedSurface(const unsigned int surface);        // is emitted when an existing surface is deleted
    void updatedVolume();               // is emitted when the volume rendering parameters have changed
    void updatedSlice();                // is emitted when the slice parameters have changed
    void redrawScene();                 // is emitted when something has changed

  public slots:
    void loadDensityA();                // loads a new cube file for density A
    void loadDensityB();                // loads a new cube file for density B
    void addSurface();                  // adds a new surface
    void addSurfacePair();              // adds a pair of surfaces with opposite signs
    void deleteSurface();               // deletes an existing surface
    void updateAll();                   // updates all changes

  protected:
    void customEvent(QCustomEvent* e);  // reimplemented to receive events from loadingThread
    void showEvent(QShowEvent* e);      // reimplemented to keep the colour column fixed after a hide/show cycle
    void hideEvent(QHideEvent* e);      // reimplemented to keep the colour column fixed after a hide/show cycle

  private slots:
    void updateVisualizationType();     // does the necessary updating when a new visualization type was chosen
    void updateSliderLevel();           // updates SliderLevel from the contents of LineEditLevel
    void updateLineEditLevel();         // updates LineEditLevel from the value of SliderLevel
    void updateListView();              // updates ListViewParameters upon changes in the settings
    void updateSettings();              // updates the settings upon changes in ListViewParameters
    void updateVisibility(QListViewItem* item, const QPoint&, int column); // updates the visibility of a surface
    void updateOperation(const unsigned int density = 0);   // updates the possible operations 
    void updateOpacity();               // updates LabelOpacity with the current opacity value
    void setSingleColor();              // allows single colors to be used for isosurfaces
    void setMapping();                  // allows density mapping to be used to color isosurfaces
    void resetMappedMaxima();           // resets the maxima in mappingWidget to their original values
    void resetVolumeMaxima();           // resets the maxima for rendering volumes to their original values
    void resetSliceMaxima();            // resets the maxima for rendering slices to their original values
    void checkUpdate();                 // calls updateAll if automatic updates are enabled

  private:
    friend class GLMoleculeView; // temporary for volume rendering test
    ///// private enums
    enum Columns{COLUMN_VISIBLE, COLUMN_ID, COLUMN_RGB, COLUMN_LEVEL, COLUMN_COLOUR, COLUMN_OPACITY, COLUMN_TYPE}; ///< Indices of each column in the ListView

    ///// private member functions
    void makeConnections();             // sets up all connections
    void loadDensity(const bool densityA);        // loads a density for density A or B
    bool loadCube(QFile* file);         // reads a cube file
    bool loadPLT(QFile* file);          // reads a PLT file
    void updateDensity();               // updates everything after loading has finished
    void updateProgress(const unsigned int progress);       // updates the progressbar for the current loading density
    unsigned int typeToNum(const QString& type);      // translates the type into a number
    void enableWidgets();               // enables/disables the correct widgets depending on the status of the class
    bool identicalGrids();              // returns true if the grids of densityA and B are identical
    bool updateIsoSurfaces();           // updates all changes concerning isosurfaces
    bool updateVolume();                // updates all changes concerning volumetric rendering
    bool updateSlice();                 // updates all changes concerning slices.

    ///// private structs
    struct SurfaceProperties            
    /// Contains the properties of a surface.
    {
      bool visible;                     ///< = true if the surface is visible
      bool mapped;                      ///< = true if the surface is color mapped
      double level;                     ///< The isolevel at which the surface is determined
      unsigned int colour;              ///< The colour of the surface
      unsigned int opacity;             ///< The opacity of the surface
      unsigned int type;                ///< The rendering type (solid, wireframe, dots)
      bool deleted;                     ///< = true if the surface was deleted
      bool isNew;                       ///< = true if the surface is a new one
      unsigned int ID;                  ///< The ID of the surface
    };
    struct VolumeProperties
    /// Contains the properties for volume renders
    {
      unsigned int positiveColor;       ///< The colour used to draw positive density
      unsigned int negativeColor;       ///< The colour to draw negative density
      QString maxLevel;                 ///< The maximum density level to plot.
      QString minLevel;                 ///< The minimum density level to plot.
    };
    struct SliceProperties
    /// Contains the properties for a slice
    {
      unsigned int positiveColor;       ///< The colour used to draw positive density
      unsigned int negativeColor;       ///< The color used to draw negative density
      unsigned int backgroundColor;     ///< The background color (if not transparent)
      bool transparent;                 ///< If the background should be transparent
      unsigned int map;                 ///< The type of color map
      unsigned int index;               ///< The index of the current slice
    };

    ///// private member data
    DensityGrid* densityGrid;           ///< A pointer to the DensityGrid.
    unsigned int idCounter;             ///< A counter for uniquely identifying defined surfaces.
    std::vector<SurfaceProperties> surfaceProperties;       ///< A list of the properties of each defined surface.
    LoadDensityThread* loadingThread;   ///< A thread that does the actual reading of the density points from the grid file.
    std::vector<double> densityPointsA; ///< An array holding the density values for Density A.
    std::vector<double> densityPointsB; ///< An array holding the density values for Density B.
    bool loadingDensityA;               ///< Indicates which density is loading.
    Point3D<float> originA;             ///< Holds the coordinates of the origin of density A.
    Point3D<float> originB;             ///< Holds the coordinates of the origin of density B.
    Point3D<unsigned int> numPointsA;   ///< Holds the number of data points in each direction of density A.
    Point3D<unsigned int> numPointsB;   ///< Holds the number of data points in each direction of density B.
    Point3D<float> deltaA;              ///< Holds the cell lengths of density A.
    Point3D<float> deltaB;              ///< Holds the cell lengths of density B.
    QString newDescription;             ///< Holds the description of the contents of a new density.
    int columnColourWidth;              ///< Holds the right column width for the one containing the colour of the surface.
    MappedSurfaceWidget* mappingWidget; ///< The dialog for setting up density mapping.
    int oldVisualizationType;           ///< Keeps tracks of changes in ComboBoxVisualizationType
    bool mappingChanged;                ///< Keeps track of whether isosurfaces have to be redrawn due to changes in mapping
    VolumeProperties volumeProperties;  ///< Keeps track of changes in the visualization of volumes
    SliceProperties sliceProperties;    ///< Keeps track of changes to the slices.

    ///// static private member data
    static const double deltaLevel;     ///< The minimal change allowed in isoLevels.
};
#endif

