#include "PolygonFitting/PolygonFitting.h"

void generateVoronoiPolygons( vector< Polygon > &polygons, int count, int numrelaxations, int width, int height );

void generateImagePolygons( string imageDirectory, vector< string > &inputDirectory, vector< Polygon > &imagePolygons, vector< vector< unsigned char > > &images, vector< vector< unsigned char > > &masks, vector< pair< int, int > > &dimensions, double buildScale, double renderScale, bool recursiveSearch );

void RunMosaicPuzzle( string inputName, string outputImage, vector< string > inputDirectory, int numCells, bool secondPass, double renderScale, double buildScale, double angleOffset, bool trueColor, bool skipNearby, double sizePower );