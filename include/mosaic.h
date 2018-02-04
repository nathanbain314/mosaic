#include <iostream>
#include <vector>
#include <cmath>
#include <climits>
#include <unistd.h>
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
#include "progressbar.h"

using namespace nanoflann;
using namespace vips;
using namespace std;

typedef tuple< string, int, int, int, bool > cropType;
typedef KDTreeVectorOfVectorsAdaptor< vector< vector< int > >, int >  my_kd_tree_t;

void generateThumbnails( vector< cropType > &cropData, vector< vector< unsigned char > > &mosaicTileData, vector< vector< unsigned char > > &imageTileData, string imageDirectory, int mosaicTileWidth, int imageTileWidth, bool exclude = false, bool spin = false, int cropStyle = 0, bool flip = false );

pair< int, double > computeBest( vector< vector< float > > &imageData, vector< float > &d, int start, int end, int tileWidth, vector< vector< int > > &mosaic, int i, int j, int repeat );

int generateMosaic( vector< vector< float > > &imageData, vector< vector< int > > &mosaic, string inputImage, progressbar *buildingMosaic, int repeat = 0, bool square = false, int resize = 0 );

int generateMosaic( my_kd_tree_t &mat_index, vector< vector< int > > &mosaic, string inputImage, progressbar *buildingMosaic, int repeat = 0, bool square = false, int resize = 0 );

void buildImage( vector< vector< unsigned char > > &imageData, vector< vector< int > > &mosaic, string outputImage, int tileWidth );

void buildDeepZoomImage( vector< vector< int > > &mosaic, vector< cropType > cropData, int numUnique, string outputImage, ofstream& htmlFile );

void buildContinuousMosaic( vector< vector< vector< int > > > mosaic, vector< VImage > &images, string outputDirectory, ofstream& htmlFile );