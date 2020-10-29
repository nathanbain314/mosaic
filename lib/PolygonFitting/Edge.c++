#include "PolygonFitting/Edge.h"

Edge::Edge( int _v1, int _v2 )
{
  v1 = _v1;
  v2 = _v2;
}

Edge::Edge( const Edge& rhs )
{
  v1 = rhs.v1;
  v2 = rhs.v2;
}

Edge& Edge::operator=( const Edge& rhs )
{
  v1 = rhs.v1;
  v2 = rhs.v2;

  return *this;
}

bool Edge::operator==( const Edge& rhs ) const
{
  return (this->v1 == rhs.v1 && this->v2 == rhs.v2);
}

bool Edge::operator!=( const Edge& rhs ) const
{
  return (this->v1 != rhs.v1 || this->v2 != rhs.v2);
}