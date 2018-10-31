#include <vector>
#include <cmath>
#include <iostream>

using namespace std;

void createCircleIndices( vector< int > &shapeIndices, int mosaicTileWidth, int mosaicTileHeight, int tileWidth, int tileHeight );

void createJigsawIndices( vector< vector < int > > &shapeIndices, double mosaicTileWidth, double mosaicTileHeight, vector< int > &tileWidth, vector< int > &tileHeight );

void createJigsawIndices0( vector< int > &shapeIndices, double mosaicTileWidth, double mosaicTileHeight, int tileWidth, int tileHeight );

void createJigsawIndices1( vector< int > &shapeIndices, double mosaicTileWidth, double mosaicTileHeight, int tileWidth, int tileHeight );