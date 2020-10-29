#include "PolygonFitting/Vertex.h"

class Edge
{
public:
  int v1, v2;

  Edge(){}

  Edge( int _v1, int _v2 );

  Edge( const Edge& rhs );

  Edge& operator=( const Edge& rhs );
  
  bool operator==( const Edge& rhs ) const;

  bool operator!=( const Edge& rhs ) const;
};