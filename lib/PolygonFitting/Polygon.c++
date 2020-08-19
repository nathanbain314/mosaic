#include "PolygonFitting/Polygon.h"

Polygon::Polygon( vector< Vertex > &_vertices, vector< Edge > &_edges )
{
  edges = _edges;
  vertices = _vertices;
}

Polygon::Polygon( const Polygon& rhs )
{
  edges = rhs.edges;
  vertices = rhs.vertices;
}

Polygon& Polygon::operator=( const Polygon& rhs )
{
  edges = rhs.edges;
  vertices = rhs.vertices;

  return *this;
}

void Polygon::addVertex( float x, float y )
{
  vertices.push_back( Vertex(x,y) );
}

void Polygon::addVertex( Vertex v )
{
  vertices.push_back( v );
}

void Polygon::addEdge( int v1, int v2 )
{
  edges.push_back( Edge( v1, v2 ) );
}

void Polygon::addEdge( Edge e )
{
  edges.push_back( e );
}

void Polygon::switchDirection()
{
  for( int i = 0; i < vertices.size()/2; ++i )
  {
    Vertex t = vertices[i];
    vertices[i] = vertices[vertices.size() - 1 - i];
    vertices[vertices.size() - 1 - i] = t;
  }
}

void Polygon::makeClockwise()
{
  float n = 0;
  for( int i = 0; i < edges.size(); ++i )
  {
    n += ( vertices[edges[i].v2].x - vertices[edges[i].v1].x ) * ( vertices[edges[i].v2].y + vertices[edges[i].v1].y );
  }

  if( n < 0 )
  {
    switchDirection();
  }
}

void Polygon::scaleBy( float scale )
{
  for( int i = 0; i < vertices.size(); ++i )
  {
    vertices[i].x *= scale;
    vertices[i].y *= scale;
  }
}

void Polygon::offsetBy( Vertex offset )
{
  for( int i = 0; i < vertices.size(); ++i )
  {
    vertices[i] = vertices[i].offset( offset );
  }
}

void Polygon::rotateBy( float r, bool offset )
{
  float pi = 3.14159265358979;

  r *= pi / 180.0;

  Vertex origin = Vertex( -1000000, -1000000 );

  for( int i = 0; i < vertices.size(); ++i )
  {
    float x = vertices[i].x * cos( r ) - vertices[i].y * sin( r );
    float y = vertices[i].x * sin( r ) + vertices[i].y * cos( r );
    vertices[i].x = x;
    vertices[i].y = y;

    origin.x = max( -x, origin.x );
    origin.y = max( -y, origin.y );
  }
  
  if( offset )
  {
    offsetBy(Vertex(-origin.x,-origin.y));
  }
  else
  {
    center();
  }
}

Vertex Polygon::center()
{
  float maxX = -1000000000, minX = 1000000000, maxY = -1000000000, minY = 1000000000;

  for( int i = 0; i < vertices.size(); ++i )
  {
    Vertex v1 = vertices[i];

    //cout << v1 << endl;

    minX = min( v1.x, minX );
    maxX = max( v1.x, maxX );
    minY = min( v1.y, minY );
    maxY = max( v1.y, maxY );
  }

  offsetBy(Vertex(-(minX+maxX)/2,-(minY+maxY)/2));

  return Vertex((minX+maxX)/2,(minY+maxY)/2);
}

Vertex Polygon::computeOffset( float r )
{
  float pi = 3.14159265358979;

  r *= pi / 180.0;

  float px = -1000000, py = -1000000, bx = -1000000, by = -1000000;

  float xRange[2] = {1000000000,-1000000000};
  float yRange[2] = {1000000000,-1000000000};

  for( int i = 0; i < vertices.size(); ++i )
  {
    float x = vertices[i].x * cos( r ) - vertices[i].y * sin( r );
    float y = vertices[i].x * sin( r ) + vertices[i].y * cos( r );

    xRange[0] = min(vertices[i].x,xRange[0]);
    xRange[1] = max(vertices[i].x,xRange[1]);

    yRange[0] = min(vertices[i].y,yRange[0]);
    yRange[1] = max(vertices[i].y,yRange[1]);

    px = max( -x, px );
    py = max( -y, py );
  }

  for( int i = 0; i < 2; ++i )
  {
    xRange[i] -= xRange[0];
    yRange[i] -= yRange[0];
  }

  for( int i = 0; i < 2; ++i )
  { 
    for( int j = 0; j < 2; ++j )
    {
      float x = xRange[j] * cos( r ) - yRange[i] * sin( r );
      float y = xRange[j] * sin( r ) + yRange[i] * cos( r );

      bx = max( -x, bx );
      by = max( -y, by );
    }
  }

  return Vertex( px - bx, py - by );
}

float Polygon::minRatio()
{
  float largestRatio = -1000000000;
//  float bestAngle = 0;

//  float lw, lh;

  for( int i = 0; i < edges.size(); ++i )
  {
    float r = -atan2(vertices[edges[i].v2].y-vertices[edges[i].v1].y,vertices[edges[i].v2].x-vertices[edges[i].v1].x);

    float xRange[2] = {1000000000,-1000000000};
    float yRange[2] = {1000000000,-1000000000};

    for( int j = 0; j < vertices.size(); ++j )
    {
      float x = vertices[j].x * cos( r ) - vertices[j].y * sin( r );
      float y = vertices[j].x * sin( r ) + vertices[j].y * cos( r );

      xRange[0] = min(x,xRange[0]);
      xRange[1] = max(x,xRange[1]);

      yRange[0] = min(y,yRange[0]);
      yRange[1] = max(y,yRange[1]);
    }

    float width = xRange[1] - xRange[0];
    float height = yRange[1] - yRange[0];

    float ratio = max(width,height)/min(width,height);

    largestRatio = max(ratio,largestRatio);
  }

  return largestRatio;
}

float Polygon::minLength()
{
  float minLength = 1000000000;

  for( int i = 0; i < edges.size(); ++i )
  {
    float r = -atan2(vertices[edges[i].v2].y-vertices[edges[i].v1].y,vertices[edges[i].v2].x-vertices[edges[i].v1].x);

    float xRange[2] = {1000000000,-1000000000};
    float yRange[2] = {1000000000,-1000000000};

    for( int j = 0; j < vertices.size(); ++j )
    {
      float x = vertices[j].x * cos( r ) - vertices[j].y * sin( r );
      float y = vertices[j].x * sin( r ) + vertices[j].y * cos( r );

      xRange[0] = min(x,xRange[0]);
      xRange[1] = max(x,xRange[1]);

      yRange[0] = min(y,yRange[0]);
      yRange[1] = max(y,yRange[1]);
    }

    float width = xRange[1] - xRange[0];
    float height = yRange[1] - yRange[0];

    minLength = min(minLength,min(width,height));
  }

  return minLength;
}

bool Polygon::notOnEdge( float width, float height )
{
  for( int j = 0; j < vertices.size(); ++j )
  {
    if( vertices[j].x < 0.000001 || vertices[j].x > width - 0.000001 )
    {
      return false;
    }
    if( vertices[j].y < 0.000001 || vertices[j].y > height - 0.000001 )
    {
      return false;
    }
  }

  return true;
}

float Polygon::area()
{
  float area = 0;

  for( int i=0; i < vertices.size(); i += 2 )
  {
    float x0 = vertices[i].x;
    float y0 = vertices[i].y;
    float x1 = vertices[(i+2) % vertices.size()].x;
    float y1 = vertices[(i+2) % vertices.size()].y;
    float a = x0*y1 - x1*y0;
    area += a;
  }

  area *= -0.5;

  return area;
}