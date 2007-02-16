/***************************************************************************
                        densitygrid.h  -  description
                             -------------------
    begin                : Fri Mar 11 2005
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
/// Contains the declaration of the class DensityGrid

#ifndef DENSITYGRID_H
#define DENSITYGRID_H

///// Forward class declarations & header files ///////////////////////////////

// STL includes
#include <map>
#include <vector>
using std::map;
using std::vector;

// Qt forward class declarations
class QColor;
class QImage;

// Xbrabo includes
#include <point3d.h>

///// class DensityGrid ///////////////////////////////////////////////////////
class DensityGrid
{
  public:
    ///// constructur/destructor
  	DensityGrid();                      // constructor
	  ~DensityGrid();                     // destructor

    ///// public enums
    enum ColorMapType{MAP_BLUE_RAINBOW_RED = 0, MAP_RED_RAINBOW_BLUE, MAP_BLACK_RAINBOW_WHITE, 
                      MAP_WHITE_RAINBOW_BLACK, MAP_BLUE_MAGENTA_RED, MAP_RED_MAGENTA_BLUE,
                      MAP_LAST};        ///< currently equal to the possibilities in the class MappedSurfaceWidget
    enum Plane{PLANE_XY, PLANE_XZ, PLANE_YZ, PLANE_ZX};     ///< Different orientations for slices

    ///// public member functions for changing data
	  void setParameters(const std::vector<double>* values, const Point3D<unsigned int>& pointDimension, const Point3D<float>& pointDelta, const Point3D<float>& pointOrigin);         // set up the parameters for the surface 
    void setMappingParameters(const std::vector<double>* values, const unsigned int map, const float maxValue, const float minValue);       // sets up the mapping density for the given regular density and the color map
	  void addSurface(const double isoDensity); // calculates a new surface
    void changeSurface(const unsigned int surface, const double isoDensity);      // recalculates a surface

    ///// public member functions for retrieving data
    bool densityPresent() const;          // returns whether a density has been loaded
    bool hasMapping() const;              // returns whether a mapping density is present
    unsigned int numSurfaces() const;     // returns the number of calculated surfaces
    unsigned int numTriangles(const unsigned int surface) const;        // returns the number of triangles a certain surface consists of
	  unsigned int numVertices(const unsigned int surface) const;         // returns the number of points a certain surface consists of
	  void getTriangle(const unsigned int surface, const unsigned int index, 
                     Point3D<float>& point1, Point3D<float>& point2, Point3D<float>& point3, 
                     Point3D<float>& normal1, Point3D<float>& normal2, Point3D<float>& normal3) const;// return the data of a triangle of a surface    
    QColor getMappingColor(const Point3D<float>& point) const;        // return the color of the given point according to the active color map
    Point3D<float> getPoint(const unsigned int surface, const unsigned int index) const;    // returns the coordinates of a point on a surface
    void clearParameters();               // clear all data
    void clearSurfaces();                 // removes all existing surfaces
    void removeSurface(const unsigned int surface); // removes a certain surface
    Point3D<float> getOrigin() const;               // returns the set origin
    Point3D<float> getDelta() const;                // returns the set deltas
    Point3D<unsigned int> getNumPoints() const;     // returns the number of points in all directions
    double getMaximumDensity() const;               // returns the most positive value of the density
    double getMinimumDensity() const;               // returns the most negative value of the density
    QImage getSlice(const unsigned int plane, const unsigned int index, const QColor& positiveColor, const QColor& negativeColor, 
                    const double maxPlotValue, const double minPlotValue, const unsigned int colorMap = MAP_LAST) const;// Returns an image to be used as a slice

  private:
	  ///// private structs
	  struct Triangle
    /// A utility struct containing the ID's of 3 points making up a triangle.
	  {
      unsigned int pointID[3];
	  };

    ///// private member functions
    void calculateSurface(const double isoDensity); // does the basic surface calculation
    Point3D<float> intersection(const unsigned int x, const unsigned int y, const unsigned int z, const unsigned int edge);    // calculates the intersection
    Point3D<float> interpolate(const Point3D<float> point1, const Point3D<float> point2, const double var1, const double var2);   // linear interpolation
	  unsigned int getEdgeID(const unsigned int x, const unsigned int y, const unsigned int z, const unsigned int edge);  // returns the ID of the edge 
	  unsigned int getVertexID(const unsigned int x, const unsigned int y, const unsigned int z);     // returns the ID of the vertex 
	  void renameVerticesAndTriangles(vector<Point3D<float> >* singleVerticesList, vector<unsigned int>* singleTriangleIndices);  // renames the vertices and triangles
    void calculateNormals(vector<float>* singleNormals, const unsigned int surface);        // calculates the normals
    unsigned int getArrayIndex(const unsigned int x, const unsigned int y, const unsigned int z) const;         // returns the index into the densityValues array
    QColor mapColor(const double value) const;    // returns the color from the active color map on a scale of value (0.0 - 1.0)

    ///// private member data
    vector<double> densityValues;         ///< a vector containing the input density values
    vector<double> mappingValues;         ///< a vector containing the mapping density values
    Point3D<unsigned int> numPoints;      ///< a Point3D containing the number of points in the 3 directions
    Point3D<float> delta;                 ///< a Point3D containing the cell lengths in the 3 directions
    Point3D<float> origin;                ///< the origin of the density values
    map<unsigned int, Point3D<float> > vertices;    ///< the list of points forming the isosurface
    vector<Triangle> triangles;           ///< the list of triangles forming the isosurface
    double currentIsoLevel;               ///< holds the current isodensity value
    vector<double> isoLevels;             ///< a list of isodensity values for each calculated surface
    vector< vector<Point3D<float> >* > verticesList;///< an easily accessible list of vertices for each calculated surface
    vector< vector<unsigned int>* > triangleIndices;///< an easily accessible list of vertex indices for each calculated surface
    vector< vector<float>* > normals;     ///< a list of normals for each calculated surface
    mutable unsigned int colorMap;        ///< holds the current color map type (temporarily mutated only in getSlice)
    double maxDensity, minDensity;        ///< hold the extrema of the density values
    double maxMapValue, minMapValue;      ///< hold the extrema between which the mapping colors have to be interpolated

	  ///// private static member data
	  static const unsigned int edgeTable[256];        ///< lookup table for edges
	  static const int triTable[256][16];     ///< lookup table for triangles. The original implementation used unsigned ints which is very
                                            ///< strange as the table contains negative number. Works either way, though.
};

#endif

