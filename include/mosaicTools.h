#include <iostream>
#include <vector>
#include <cmath>
#include <climits>
#include <dirent.h>
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
#include "progress_bar.hpp"

using namespace nanoflann;
using namespace vips;
using namespace std;

typedef tuple< string, int, int, int, bool > cropType;
typedef KDTreeVectorOfVectorsAdaptor< vector< vector< int > >, int >  my_kd_tree_t;

vector< string > inputDirectoryPlaceHolder();

int numberOfCPUS();

void generateThumbnails( vector< cropType > &cropData, vector< vector< unsigned char > > &mosaicTileData, vector< vector< unsigned char > > &imageTileData, vector< string > &inputDirectory, string imageDirectory, int mosaicTileWidth, int mosaicTileHeight, int imageTileWidth, int imageTileHeight, bool exclude = false, bool spin = false, int cropStyle = 0, bool flip = false, bool quiet = false, bool recursiveSearch = false );

int generateMosaic( vector< vector< float > > &imageData, vector< vector< int > > &mosaic, string inputImage, ProgressBar *buildingMosaic, int repeat = 0, bool square = false, int resize = 0, bool quiet = false );

int generateMosaic( my_kd_tree_t &mat_index, vector< vector< int > > &mosaic, string inputImage, ProgressBar *buildingMosaic, int repeat = 0, bool square = false, int resize = 0, bool quiet = false );