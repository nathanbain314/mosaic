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
#include <stack>
#include <functional>
#include <time.h>

#include <vips/vips8>

using namespace vips;
using namespace std;

class Vertex
{
public:
  double x, y;

  Vertex(){}

  Vertex( double _x, double _y );

  Vertex( const Vertex& rhs );

  Vertex& operator=( const Vertex& rhs );

  bool equals( const Vertex& rhs, double epsilon );

  bool operator==( const Vertex& rhs );

  bool operator!=( const Vertex& rhs );

  Vertex offset( double _x, double _y );

  Vertex offset( Vertex v );

  Vertex scale( double s, Vertex o );
};

ostream& operator<<( ostream &os, const Vertex& v );
