/***************************************************************************
                      preferencesbase.h  -  description
                             -------------------
    begin                : Sat Aug 10 2002
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
/// Contains the declaration of the class PreferencesBase.

#ifndef PREFERENCESBASE_H
#define PREFERENCESBASE_H

///// Forward class declarations & header files ///////////////////////////////

// STL header files
#include <vector>

// Qt forward class declarations
class QIconViewItem;
class QStyle;

// QextMDI forward class declarations
#if defined(USE_KMDI) || defined(USE_KMDI_DLL)
#  define QextMdiMainFrm KMdiMainFrm
#endif
class QextMdiMainFrm;

// Xbrabo forward class declarations
class ColorButton;
#include "glmoleculeview.h"

// Base class header file
#include "preferenceswidget.h"


///// class PreferencesBase ///////////////////////////////////////////////////
class PreferencesBase : public PreferencesWidget
{
  Q_OBJECT

  public:
    PreferencesBase(QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0);  // constructor
    ~PreferencesBase();                 // destructor

    ///// public member functions for retrieving data               
    //unsigned int preferredBasisset() const;       // returns the preferred basisset
    bool useBinDirectory() const;                 // returns true if .11 files should be written to a special directory
    GLView::GLBaseParameters getGLBaseParameters() const;   // returns a struct with the OpenGL base parameters
    GLSimpleMoleculeView::GLMoleculeParameters getGLMoleculeParameters() const; // returns a struct with the OpenGL molecule parameters
    GLMoleculeView::GLTextureParameters getGLTextureParameters() const;         // returns a struct with the OpenGL texture parameters
    QStringList getPVMHosts() const;              // returns a list of PVM hosts
    void setToolbarsInfo(const QString& info, const bool status);     // sets the info needed to restore the toolbars
    void getToolbarsInfo(bool& status, QString& info) const;// returns the toolbars info
    void applyChanges();                // applies any changes to the widgets and updates everything as needed.
   
  signals:
    void newPVMHosts(const QStringList& hosts);   // is emitted when the PVM host list has changed
        
  public slots:
    void loadSettings();                // loads the program settings
    void saveSettings();                // saves the program settings
    void updateVisuals();               // updates the look of the entire program

  protected slots:
    void accept();                      // is called when the changes are accepted (OK clicked)
    void reject();                      // is called when the changes are rejected (Cancel or X clicked)
  
  private slots:
    ///// global
    void changed();                     // sets the 'changed' status of the widget
    void selectWidget(QIconViewItem* item);       // shows a widget from the widgetstack depending on the iconview item
    ///// Paths
    void changeExecutable();            // updates LineEditExecutable with the selected item in ListViewExecutable
    void updateExecutable(const QString& text);   // keeps ListViewExecutables in sync with LineEditExecutable
    void updateAllExecutables();        // renames all items in the ListView according to a pattern
    void selectBinDir();                // selects a directory to store the .11
    void selectExecutable();            // selects an executable
    void selectBasisDir();              // selects a directory to read the basissets from
    ///// Visuals
    void selectBackground();            // selects a background image
    void updateUndoRedo();              // updates the undo/redo spinboxes
    ///// Molecule
    void updateLineEditBondSizeLines(); // updates LineEditBondSizeLines according to SliderBondSizeLines
    void updateLineEditBondSizeTubes(); // updates LineEditBondSizeTubes according to SliderBondSizeTubes
    void updateSliderBondSizeTubes();   // updates SliderBondSizeTubes according to LineEditBondSizeTubes
    void updateLineEditForceSizeTubes();// updates LineEditForceSizeTubes according to SliderForceSizeTubes
    void updateSliderForceSizeTubes();  // updates SliderForceSizeTubes according to LineEditForceSizeTubes
    void updateOpacitySelection();      // updates TextLabelSelection according to SliderSelectionOpacity
    void updateOpacityForces();         // updates TextLabelForces according to SliderForcesOpacity
    void updateColorButtonForce();      // enables/disables ColorButtonForce according to the selected color type
    ///// OpenGL
    void updateSliderSlices();          // updates the maximum value for SliderSlices
    ///// PVM
    void changePVMHost();               // updates LineEditPVMHost with the selected item in ListViewPVMHosts
    void updatePVMHost(const QString& text);      // keeps ListViewPVMHosts in sync with LineEditPVMHost
    void newPVMHost();                  // adds a PVM host
    void deletePVMHost();               // deletes a PVM host
    //void changedPVM();                  // the PVM host list was changed

  private:
    friend class CommandPreferences;

    ///// private enums
    enum{SettingsVersion = 110};        //< The current version for the settings file 

    ///// private member functions
    void makeConnections();             // sets up all permanent connections
    void init();                        // initializes the dialog
    void initOpenGL();                  // determines the capabilities of OpenGL
    void saveWidgets();                 // saves the status of the widgets
    void restoreWidgets();              // restores the status of the widgets
    void updateStyle();                 // Changes the style of the application
    void updatePaths();                 // updates the data of the Paths class

    ///// private structs
    struct WidgetData
    /// A struct for saving the status of all widgets of PreferencesWidget
    {
      ///// Paths
      QStringList executables;          ///< ListViewExecutables (second column)
      QString path;                     ///< LineEditPath
      QString extension;                ///< LineEditExtension
      bool binInCalcDir;                ///< RadioButtonBin{1|2}
      QString binDir;                   ///< LineEditBin
      QString basissetDir;              ///< LineEditBasis
      unsigned int basisset;            ///< ComboBoxBasis
      
      ///// Molecule
      unsigned int styleMolecule;       ///< ComboBoxMolecule
      unsigned int styleForces;         ///< ComboBoxForces
      int fastRenderLimit;              ///< SpinBoxFastRender
      bool showElements;                ///< CheckBoxElement
      bool showNumbers;                 ///< CheckBoxNumber
      int sizeLines;                    ///< SliderBondSizeLines
      QString sizeBonds;                ///< LineEditBondSizeTubes
      QString sizeForces;               ///< LineEditForceSizeTubes
      unsigned int colorLabels;         ///< ColorButtonLabel
      unsigned int colorICs;            ///< ColorButtonIC
      unsigned int colorBackgroundGL;   ///< ColorButtonBackgroundGL
      unsigned int colorSelections;     ///< ColorButtonSelection
      unsigned int colorForces;         ///< ColorButtonForce
      unsigned int opacitySelections;   ///< SliderSelectionOpacity
      unsigned int opacityForces;       ///< SliderForceOpacity
      bool forcesOneColor;              ///< ComboBoxForceColor

      ///// Application
      unsigned int backgroundType;      ///< ButtonGroupBackground
      QString backgroundImage;          ///< LineEditBackground
      unsigned int backgroundColor;     ///< ColorButtonBackground
      unsigned int styleApplication;    ///< ComboBoxStyle
      unsigned int undoRedo;            ///< ButtonGroupUndoRedo
      unsigned int undoLevels;          ///< SpinBoxUndoLevels;
      unsigned int undoRAM;             ///< SpinBoxUndoRAM
      
      ///// OpenGL
      unsigned int lightPosition;       ///< ButtonGroupLightPosition
      unsigned int lightColor;          ///< ColorButtonLight;
      unsigned int materialSpecular;    ///< SliderSpecular
      unsigned int materialShininess;   ///< SliderShininess
      bool antialias;                   ///< CheckBoxAA
      bool smoothShading;               ///< CheckBoxSmooth
      bool depthCue;                    ///< CheckBoxDepthCue;
      unsigned int quality;             ///< SliderQuality
      unsigned int sliceQuality;        ///< SliderSlices
      bool perspectiveProjection;       ///< ButtonGroupProjection
      bool use3DTextures;               ///< CheckBoxVolumeHQ

      ///// PVM
      QStringList pvmHosts;             ///< ListViewPVMHosts      
    };

    ///// private member data
    // widgets
    QString startupStyleName;           ///< Contains the startup style name
    QextMdiMainFrm* mainWindow;         ///< Contains a pointer to the main window
    // other
    WidgetData data;                    ///< Contains the status of all widgets.
    bool widgetChanged;                 ///< = true if any of the widgets have changed.
    //bool pvmHostsChanged;               ///< = true if the PVM host list has changed.
    QString tempBondSizeBS;             ///< Contains the bond size for ball&stick.
    QString tempBondSizeL;              ///< Contains the bond size for lines.
    float minLineWidthGL;               ///< Contains the minimum possible linewidth for a glLineWidth call.
    float maxLineWidthGL;               ///< Contains the maximum possible linewidth for a glLineWidth call.
    float lineWidthGranularity;         ///< Contains the granularity for OpenGL linewidths.
    int max2DTextureSize;               ///< Maximum allowed size for 2D textures
    int max3DTextureSize;               ///< Maximum allowed size for 3D textures
    QString toolbarsInfo;               ///< Contains the info needed to restore the toolbars.
    bool toolbarsStatus;                ///< Contains the visibility of the statusbar (not present in toolbarsInfo).   
};

#endif

