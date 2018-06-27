#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <algorithm>

using namespace std;

void createLocations( vector< vector< int > > &mosaicLocations, vector< vector< int > > &mosaicLocations2, vector< vector< int > > &edgeLocations, vector< vector< int > > &edgeLocations2, vector< vector< double > > &offsets, vector< vector< double > > &dimensions, int mosaicTileWidth, int mosaicTileHeight, int imageTileWidth, int imageTileHeight, int width, int height );

void createTetrisLocations( vector< vector< int > > &mosaicLocations, vector< vector< int > > &mosaicLocations2, vector< vector< int > > &edgeLocations, vector< vector< int > > &edgeLocations2, vector< vector< double > > &offsets, vector< vector< double > > &dimensions, int mosaicTileWidth, int mosaicTileHeight, int imageTileWidth, int imageTileHeight, int width, int height, int numHorizontal, int numVertical );