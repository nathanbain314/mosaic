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
#include <time.h>
#include <vips/vips8>
#include <nanoflann.hpp>
#include "KDTreeVectorOfVectorsAdaptor.h"
#include "progressbar.h"

using namespace nanoflann;
using namespace vips;
using namespace std;

void generateThumbnails( vector< string > &names, vector< vector< unsigned char > > &mosaicTileData, vector< vector< unsigned char > > &imageTileData, string imageDirectory, int mosaicTileWidth, int imageTileWidth, bool exclude = false );

pair< int, double > computeBest( vector< vector< float > > &imageData, vector< float > &d, int start, int end, int tileWidth, vector< vector< int > > &mosaic, int i, int j, int repeat );

int generateMosaic( vector< vector< float > > &imageData, vector< vector< int > > &mosaic, string inputImage, progressbar *buildingMosaic, int repeat = 0, bool square = false, int resize = 0 );

int generateMosaic( KDTreeVectorOfVectorsAdaptor< vector< vector< int > >, int > &mat_index, vector< vector< int > > &mosaic, string inputImage, progressbar *buildingMosaic, int repeat = 0, bool square = false, int resize = 0 );

void buildImage( vector< vector< unsigned char > > &imageData, vector< vector< int > > &mosaic, string outputImage, int tileWidth );

void buildDeepZoomImage( vector< vector< int > > &mosaic, vector< string > imageNames, int numUnique, string outputImage, ofstream& htmlFile );

void buildContinuousMosaic( vector< vector< vector< int > > > mosaic, vector< VImage > &images, string outputDirectory, ofstream& htmlFile );