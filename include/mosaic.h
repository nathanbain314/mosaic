#include <iostream>
#include <vector>
#include <cmath>
#include <climits>
#include <dirent.h>
#include <future>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <random>
#include <set>
#include <tuple>
#include <time.h>
#include <vips/vips8>
#include <nanoflann.hpp>
#include "KDTreeVectorOfVectorsAdaptor.h"
#include "progress_bar.hpp"

using namespace nanoflann;
using namespace vips;
using namespace std;

typedef tuple< string, int, int, int, bool > cropType;
typedef KDTreeVectorOfVectorsAdaptor< vector< vector< int > >, int >  my_kd_tree_t;

void generateRotationalThumbnails( string imageDirectory, vector< vector< unsigned char > > &images, vector< vector< unsigned char > > &masks, vector< pair< int, int > > &dimensions, double scale, double renderScale );

void buildRotationalImage( string inputImage, string outputImage, vector< vector< unsigned char > > &images, vector< vector< unsigned char > > &masks, vector< pair< int, int > > &dimensions, double resize, int numIter, int angleOffset );

void generateThumbnails( vector< cropType > &cropData, vector< vector< unsigned char > > &mosaicTileData, vector< vector< unsigned char > > &imageTileData, string imageDirectory, int mosaicTileWidth, int mosaicTileHeight, int imageTileWidth, int imageTileHeight, bool exclude = false, bool spin = false, int cropStyle = 0, bool flip = false, bool quiet = false );

pair< int, double > computeBest( vector< vector< float > > &imageData, vector< float > &d, int start, int end, int tileWidth, vector< vector< int > > &mosaic, int i, int j, int repeat );

int generateMosaic( vector< vector< float > > &imageData, vector< vector< int > > &mosaic, string inputImage, ProgressBar *buildingMosaic, int repeat = 0, bool square = false, int resize = 0, bool quiet = false );

int generateMosaic( my_kd_tree_t &mat_index, vector< vector< int > > &mosaic, string inputImage, ProgressBar *buildingMosaic, int repeat = 0, bool square = false, int resize = 0, bool quiet = false );

void buildImage( vector< vector< unsigned char > > &imageData, vector< vector< int > > &mosaic, string outputImage, int tileWidth, int tileHeight );

void buildDeepZoomImage( vector< vector< int > > &mosaic, vector< cropType > cropData, int numUnique, int tileWidth, int tileHeight, string outputImage, ofstream& htmlFile, bool quiet = false );

void buildContinuousMosaic( vector< vector< vector< int > > > mosaic, vector< VImage > &images, string outputDirectory, ofstream& htmlFile );