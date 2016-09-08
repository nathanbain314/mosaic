#include <cstdio>
#include <unistd.h>
#include <string>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <algorithm>
#include <numeric>
#include <vector>
#include <climits>
#include <utility>
#include <vips/vips8>
#include <cmath>
#include <tclap/CmdLine.h>

using namespace vips;
using namespace std;

struct PointCloud
{
  struct Point
  {
    int r,g,b;
  };

  vector<Point> pts;

  // Must return the number of data points
  inline size_t kdtree_get_point_count() const { return pts.size(); }

  // Returns the distance between the vector "p1[0:size-1]" and the data point with index "idx_p2" stored in the class:
  inline int kdtree_distance(const int *p1, const size_t idx_p2,size_t /*size*/) const
  {
    const int d0=p1[0]-pts[idx_p2].r;
    const int d1=p1[1]-pts[idx_p2].g;
    const int d2=p1[2]-pts[idx_p2].b;
    return d0*d0+d1*d1+d2*d2;
  }

  // Returns the dim'th component of the idx'th point in the class:
  // Since this is inlined and the "dim" argument is typically an immediate value, the
  //  "if/else's" are actually solved at compile time.
  inline int kdtree_get_pt(const size_t idx, int dim) const
  {
    if (dim==0) return pts[idx].r;
    else if (dim==1) return pts[idx].g;
    else return pts[idx].b;
  }

  // Optional bounding-box computation: return false to default to a standard bbox computation loop.
  //   Return true if the BBOX was already computed by the class and returned in "bb" so it can be avoided to redo it again.
  //   Look at bb.size() to find out the expected dimensionality (e.g. 2 or 3 for point clouds)
  template <class BBOX>
  bool kdtree_get_bbox(BBOX& /* bb */) const { return false; }

};

// Takes string of image directory location and returns vector of average color for each image
pair< PointCloud, vector< VImage > > create_image_array( string image_directory, string output_directory, bool build = false );

void generate_image( PointCloud &q, vector< VImage > thumbnails, string input_image, string output_image, double shrink = 1);

void generate_zoomable_image( PointCloud &q, vector< VImage > thumbnails, string input_image, string output_image, double shrink = 1, bool square = false);

void generate_thumbnail( PointCloud &q, string input_image, string output_image, double shrink, bool square);



