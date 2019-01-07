#include "PolygonFitting/Vertex.h"

Vertex::Vertex( double _x, double _y )
{
  x = _x;
  y = _y;
}

Vertex::Vertex( const Vertex& rhs )
{
  x = rhs.x;
  y = rhs.y;
}

Vertex& Vertex::operator=( const Vertex& rhs )
{
  x = rhs.x;
  y = rhs.y;

  return *this;
}

bool Vertex::equals( const Vertex& rhs, double epsilon )
{
  return ( abs( this->x - rhs.x ) < epsilon && abs( this->y - rhs.y ) < epsilon );
}

bool Vertex::operator==( const Vertex& rhs )
{
  return ( abs( this->x - rhs.x ) < 0.0000000001 && abs( this->y - rhs.y ) < 0.0000000001 );
}

bool Vertex::operator!=( const Vertex& rhs )
{
  return ( abs( this->x - rhs.x ) > 0.0000000001 || abs( this->y - rhs.y ) > 0.0000000001 );
}

Vertex Vertex::offset( double _x, double _y )
{
  return Vertex( x + _x, y + _y );
}

Vertex Vertex::offset( Vertex v )
{
  return Vertex( x + v.x, y + v.y );
}

Vertex Vertex::scale( double s, Vertex o )
{
  Vertex v(this->x,this->y);
  v = v.offset(Vertex(-o.x,-o.y));
  v.x *= s;
  v.y *= s;
  v = v.offset( o );
  return v;
}

ostream& operator<<( ostream &os, const Vertex& v )
{
  return os << v.x << " " << v.y;
}