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

  void addVertex( float x, float y );

  void addVertex( Vertex v );

  void addEdge( int v1, int v2 );

  void addEdge( Edge e );

  void switchDirection();

  void makeClockwise();

  void scaleBy( float scale );

  void offsetBy( Vertex offset );

  void rotateBy( float r, bool offset = false );

  Vertex center();

  Vertex computeOffset( float r );

  float minRatio();

  float minLength();

  bool notOnEdge( float width, float height );

  float area();
};