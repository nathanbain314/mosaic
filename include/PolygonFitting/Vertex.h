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
  float x, y;

  Vertex(){}

  Vertex( float _x, float _y );

  Vertex( const Vertex& rhs );

  Vertex& operator=( const Vertex& rhs );

  bool equals( const Vertex& rhs, float epsilon );

  bool operator==( const Vertex& rhs ) const;

  bool operator!=( const Vertex& rhs ) const;

  Vertex offset( float _x, float _y );

  Vertex offset( Vertex v );

  Vertex scale( float s, Vertex o );
};

ostream& operator<<( ostream &os, const Vertex& v );
