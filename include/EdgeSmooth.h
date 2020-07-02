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

#include <Eigen/Sparse>

typedef Eigen::SparseMatrix<double> SpMat; // declares a column-major sparse matrix type of double
typedef Eigen::Triplet<double> T;

#include <vips/vips8>
#include "progress_bar.hpp"

using namespace vips;
using namespace std;

void generateEdgeWeights( VImage &image, float * edgeData, int tileHeight, float edgeWeight, bool smoothImage = false, bool quiet = false );
