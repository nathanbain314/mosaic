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
#include <mutex>
#include <condition_variable>

#include <vips/vips8>
#include <nanoflann.hpp>
#include "KDTreeArrayAdaptor.h"
#include "progress_bar.hpp"
#include "EdgeSmooth.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/convex_hull_3.h>

#include <CGAL/AABB_tree.h>
#include <CGAL/AABB_traits.h>
#include <CGAL/AABB_face_graph_triangle_primitive.h>

using namespace nanoflann;
using namespace vips;
using namespace std;

typedef CGAL::Exact_predicates_inexact_constructions_kernel  K;
typedef CGAL::Polyhedron_3<K>                     Polyhedron;
typedef K::Point_3                                Point;
typedef CGAL::Surface_mesh<Point>               Surface_mesh;
typedef K::Segment_3 Segment;

typedef CGAL::AABB_face_graph_triangle_primitive<Polyhedron> Primitive;
typedef CGAL::AABB_traits<K, Primitive> Traits;
typedef CGAL::AABB_tree<Traits> Tree;

typedef tuple< string, int, int, int, bool > cropType;
typedef KDTreeArrayAdaptor< float *, float >  my_kd_tree_t;

vector< string > inputDirectoryPlaceHolder();

void fit(float &c, int type);

void rgbToLab( int r, int g, int b, float &l1, float &a1, float &b1 );

void changeColorspace( Tree &tree, Point &center, unsigned char * c, float * c2, int start, int end, int width, float gamma, bool gammutMapping, ProgressBar * changingColorspace, bool quiet );

int numberOfCPUS();

void generateThumbnails( vector< cropType > &cropData, vector< vector< unsigned char > > &mosaicTileData, vector< vector< unsigned char > > &imageTileData, vector< string > &inputDirectory, string imageDirectory, int mosaicTileWidth, int mosaicTileHeight, int imageTileWidth, int imageTileHeight, bool exclude = false, bool spin = false, int cropStyle = 0, bool flip = false, bool quiet = false, bool recursiveSearch = false, bool multithread = true );

int generateMosaic( my_kd_tree_t *mat_index, vector< vector< int > > &mosaic, string inputImage, int repeat = 0, bool square = false, int resize = 0, float edgeWeight = 0.0f, bool smoothImage = false, bool dither = false, float gamma = 1.0f, bool gammutMapping = false, bool quiet = false );
