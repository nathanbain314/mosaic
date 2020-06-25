#include <vector>
#include <cmath>
#include <iostream>

using namespace std;

void createCircleIndices( vector< int > &shapeIndices, int mosaicTileWidth, int mosaicTileHeight, int tileWidth, int tileHeight );

void createJigsawIndices( vector< vector < int > > &shapeIndices, float mosaicTileWidth, float mosaicTileHeight, vector< int > &tileWidth, vector< int > &tileHeight );

void createJigsawIndices0( vector< int > &shapeIndices, float mosaicTileWidth, float mosaicTileHeight, int tileWidth, int tileHeight );

void createJigsawIndices1( vector< int > &shapeIndices, float mosaicTileWidth, float mosaicTileHeight, int tileWidth, int tileHeight );