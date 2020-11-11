#include "PolygonFitting/PolygonFitting.h"

void generateVoronoiPolygons( vector< Polygon > &polygons, int count, int numrelaxations, int width, int height );

void generateImagePolygons( string imageDirectory, vector< string > &inputDirectory, vector< Polygon > &imagePolygons, vector< vector< unsigned char > > &images, vector< vector< unsigned char > > &masks, vector< pair< int, int > > &dimensions, float buildScale, float renderScale, bool recursiveSearch );

void RunMosaicPuzzle( string inputName, string outputImage, vector< string > inputDirectory, int numCells, bool secondPass, float renderScale, float buildScale, float angleOffset, float edgeWeight, float morphValue, float shrinkValue, bool smoothImage, bool skipNearby, float sizePower, bool useConcave, bool recursiveSearch, string fileName );