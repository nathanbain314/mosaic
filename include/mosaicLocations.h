#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <algorithm>
#include <vips/vips8>

using namespace std;
using namespace vips;

void createLocations( vector< vector< int > > &mosaicLocations, vector< vector< int > > &mosaicLocations2, vector< vector< int > > &edgeLocations, vector< vector< int > > &edgeLocations2, vector< vector< double > > &offsets, vector< vector< double > > &dimensions, int mosaicTileWidth, int mosaicTileHeight, int imageTileWidth, int imageTileHeight, int width, int height );

void createMultisizeSquaresLocations( string inputImage, vector< vector< int > > &shapeIndices, vector< vector< int > > &mosaicLocations, vector< vector< int > > &mosaicLocations2, vector< vector< int > > &edgeLocations, vector< vector< int > > &edgeLocations2, vector< vector< double > > &offsets, vector< vector< double > > &dimensions, vector< int > &tileWidth, vector< int > &tileHeight, vector< int > &tileWidth2, vector< int > &tileHeight2, int mosaicTileWidth, int mosaicTileHeight, int imageTileWidth, int imageTileHeight, int width, int height );

void createMultisizeTrianglesLocations( string inputImage, vector< vector< int > > &shapeIndices, vector< vector< int > > &mosaicLocations, vector< vector< int > > &mosaicLocations2, vector< vector< int > > &edgeLocations, vector< vector< int > > &edgeLocations2, vector< vector< double > > &offsets, vector< vector< double > > &dimensions, vector< int > &tileWidth, vector< int > &tileHeight, vector< int > &tileWidth2, vector< int > &tileHeight2, int mosaicTileWidth, int mosaicTileHeight, int imageTileWidth, int imageTileHeight, int width, int height );

void createTetrisLocations( vector< vector< int > > &mosaicLocations, vector< vector< int > > &mosaicLocations2, vector< vector< int > > &edgeLocations, vector< vector< int > > &edgeLocations2, vector< vector< double > > &offsets, vector< vector< double > > &dimensions, int mosaicTileWidth, int mosaicTileHeight, int imageTileWidth, int imageTileHeight, int width, int height, int numHorizontal, int numVertical );