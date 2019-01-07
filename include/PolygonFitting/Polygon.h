#include "PolygonFitting/Edge.h"

class Polygon
{
public:
  vector< Edge > edges;
  vector< Vertex > vertices;
  
  Polygon(){}

  Polygon( vector< Vertex > &_vertices, vector< Edge > &_edges );

  Polygon( const Polygon& rhs );

  Polygon& operator=( const Polygon& rhs );

  void addVertex( double x, double y );

  void addVertex( Vertex v );

  void addEdge( int v1, int v2 );

  void addEdge( Edge e );

  void switchDirection();

  void makeClockwise();

  void scaleBy( double scale );

  void offsetBy( Vertex offset );

  void rotateBy( double r, bool offset = false );

  Vertex center();

  Vertex computeOffset( double r );

  double minRatio();

  double minLength();

  bool notOnEdge( double width, double height );

  double area();
};