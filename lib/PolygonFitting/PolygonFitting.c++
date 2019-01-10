#include "PolygonFitting/PolygonFitting.h"

// Initiate with polygon P to fit in polygon R
PolygonFitting::PolygonFitting( Polygon _P, Polygon _R )
{
  P = _P;
  R = _R;
}

PolygonFitting::PolygonFitting( const PolygonFitting& rhs )
{
  P = rhs.P;
  R = rhs.R;

  polygons = rhs.polygons;
  edges = rhs.edges;
  vertices = rhs.vertices;
}

PolygonFitting& PolygonFitting::operator=( const PolygonFitting& rhs )
{
  P = rhs.P;
  R = rhs.R;

  polygons = rhs.polygons;
  edges = rhs.edges;
  vertices = rhs.vertices;

  return *this;
}

// Create vertices and edges by offsetting each edge in P by each edge in R
void PolygonFitting::createOffsetEdges()
{
  for( int j = 0; j < R.vertices.size(); ++j )
  {
    Vertex offset = R.vertices[j];

    for( int i = 0; i < P.vertices.size(); ++i )
    {
      vertices.push_back( P.vertices[ i ].offset( offset ) );
    }
  }
}

double ccw( Vertex &v1, Vertex &v2, Vertex &v3 )
{
  return (v2.x - v1.x)*(v3.y - v1.y) - (v2.y - v1.y)*(v3.x - v1.x);
}

bool minY(Vertex i, Vertex j)
{ 
  return (i.y < j.y) || (i.y == j.y && i.x < j.x);
}

double distance( Vertex v1, Vertex v2 )
{
  return (v1.x-v2.x)*(v1.x-v2.x) + (v1.y-v2.y)*(v1.y-v2.y);
}

bool grahamScanSort ( Vertex v1, Vertex v2, Vertex p )
{
  double angle = ccw( v1, p, v2 );

  if( angle == 0 )
  {
    return distance( v1, p ) < distance( v2, p );
  }

  return angle > 0;
}

vector< Edge > grahamScan( vector< Vertex > vertices )
{
  vector< Vertex > verticesCopy = vertices;

  int n = vertices.size();

  // Swap vertices[0] with the point with the lowest y-coordinate
  vector< Vertex >::iterator vertexIterator;

  vertexIterator = min_element( vertices.begin(), vertices.end(), minY);

  iter_swap(vertices.begin(), vertexIterator);

  // Sort points by polar angle with vertices[0]
  sort( vertices.begin()+1, vertices.end(), bind(grahamScanSort, placeholders::_1, placeholders::_2, vertices[0]));

  stack< int > convexHull;

  for( int i = 0; i < 3; ++i )
  {
    vertexIterator = find(verticesCopy.begin(),verticesCopy.end(),vertices[i]);
    convexHull.push( vertexIterator - verticesCopy.begin() );
  }

  vertexIterator = find(verticesCopy.begin(),verticesCopy.end(),vertices[0]);
  int first = vertexIterator - verticesCopy.begin();

  // Compute hull
  for( int i = 3; i < n; ++i )
  {
    int top = convexHull.top();
    convexHull.pop();

    int second = convexHull.top();

    while( second != first && ccw( verticesCopy[top], verticesCopy[second], vertices[i] ) <= 0 )
    {
      top = second;
      convexHull.pop();
      second = convexHull.top();
    }

    convexHull.push( top );

    vertexIterator = find(verticesCopy.begin(),verticesCopy.end(),vertices[i]);
    convexHull.push( vertexIterator - verticesCopy.begin() );
  }

  convexHull.push( first );

  vector< Edge > convexHullEdges;

  int p1 = convexHull.top();
  convexHull.pop();

  while(!convexHull.empty())
  {
      int p2 = convexHull.top();
      convexHull.pop();

      convexHullEdges.push_back( Edge(p1,p2) );

      p1 = p2;
  }

  return convexHullEdges;
}

Edge reverseEdge( Edge e )
{
  return Edge( e.v2, e.v1 );
}

pair< double, double > intersection( Vertex &v1, Vertex &v2, Vertex &v3, Vertex &v4 )
{
  double x[4] = {v1.x,v2.x,v3.x,v4.x};
  double y[4] = {v1.y,v2.y,v3.y,v4.y};

  double denominator = ( x[3] - x[2] ) * ( y[0] - y[1] ) - ( x[0] - x[1] ) * ( y[3] - y[2] );

  double numerator1  = ( x[0] - x[2] ) * ( y[2] - y[3] ) + ( x[3] - x[2] ) * ( y[0] - y[2] );
  double numerator2  = ( x[0] - x[2] ) * ( y[0] - y[1] ) + ( x[1] - x[0] ) * ( y[0] - y[2] );

  // Check if parallel
  if( abs(denominator) < 0.0000001 || ( numerator1 == -numerator2 && numerator1 != 0 ) )
  {
    return pair< double, double >( -1, -1 );
  }

  return pair< double, double >( numerator1/denominator, numerator2/denominator );
}

Vertex intersectionPoint( double t, Vertex &v1, Vertex &v2 )
{
  double x = v1.x + t * ( v2.x - v1.x );
  double y = v1.y + t * ( v2.y - v1.y );

  return Vertex( x, y );
}

double distanceToLine( Vertex p, Vertex v1, Vertex v2 )
{
  double x[3] = {p.x,v1.x,v2.x};
  double y[3] = {p.y,v1.y,v2.y};

  double xDiff = x[2] - x[1];
  double yDiff = y[2] - y[1];

  double denominator = sqrt( xDiff * xDiff + yDiff * yDiff );

  if( denominator == 0 ) return -1;

  double numerator = abs( yDiff * x[0] - xDiff * y[0] + x[2] * y[1] - y[2] * x[1] );

  return numerator / denominator;
}

void PolygonFitting::computeContributingEdges()
{
  P.scaleBy(-1);

  vector< int > verticesUsed;

  vector< Vertex >::iterator it;

  for( int i2 = 0; i2 <= R.edges.size(); ++i2 )
  {
    int i = i2 % R.edges.size();
    Vertex v1 = R.vertices[ R.edges[i].v1 ];
    Vertex v2 = R.vertices[ R.edges[i].v2 ];

    double smallestAngle = 1000000000;
    int bestVertex = 0;

    for( int j = 0; j < P.vertices.size(); ++j )
    {
      Vertex p = P.vertices[j].offset( v1 );

      double angle = ccw( v1, v2, p );
 
      if( angle < smallestAngle )
      {
        smallestAngle = angle;
        bestVertex = j;
      }
    }

    v1 = v1.offset( P.vertices[bestVertex] );
    v2 = v2.offset( P.vertices[bestVertex] );

    if( i2 > 0 )
    {
      int s = P.vertices.size();

      int s2 = contributingEdges.size();

      int prevVertex = verticesUsed[s2-1];

      Vertex v3 = vertices[contributingEdges[s2-1].v1];
      Vertex v4 = vertices[contributingEdges[s2-1].v2];

      pair< double, double > t = intersection( v3, v4, v1, v2 );
    
      if( t.first >= 1+0.0000000 || t.first <= -0.0000000 || t.second >= 1+0.0000000 || t.second <= -0.0000000 )
      {
        int direction = -1;

        Vertex p1 = R.vertices[ R.edges[i].v1 ].offset( P.vertices[( prevVertex - 1 + s ) % s ] );
        Vertex p2 = R.vertices[ R.edges[i].v1 ].offset( P.vertices[( prevVertex + 1 + s ) % s ] );

        for( int k = prevVertex; k != bestVertex; k = ( k + direction + s ) % s )
        {
          Vertex vv1 = R.vertices[ R.edges[i].v1 ].offset( P.vertices[k] );

          vertices.push_back( vv1 );

          Vertex vv2 = R.vertices[ R.edges[i].v1 ].offset( P.vertices[( k + direction + s ) % s] );

          vertices.push_back( vv2 );

          contributingEdges.push_back( Edge( vertices.size()-2, vertices.size()-1 ) );

          verticesUsed.push_back( ( k + direction + s ) % s );

          edgeOffsets.push_back( P.vertices[k] );
          edgeOffsets.push_back( P.vertices[( k + direction + s ) % s] );
        }
      }
      else if( prevVertex != bestVertex )
      {
        vertices.push_back( R.vertices[ R.edges[i].v1 ].offset( P.vertices[prevVertex] ) );
        vertices.push_back( R.vertices[ R.edges[i].v1 ] );

        contributingEdges.push_back( Edge( vertices.size()-2, vertices.size()-1 ) );

        verticesUsed.push_back( prevVertex );

        edgeOffsets.push_back( P.vertices[prevVertex] );
        edgeOffsets.push_back( Vertex( 0, 0 ) );

        vertices.push_back( R.vertices[ R.edges[i].v1 ] );
        vertices.push_back( R.vertices[ R.edges[i].v1 ].offset( P.vertices[bestVertex] ) );

        contributingEdges.push_back( Edge( vertices.size()-2, vertices.size()-1 ) );

        verticesUsed.push_back( bestVertex );

        edgeOffsets.push_back( Vertex( 0, 0 ) );
        edgeOffsets.push_back( P.vertices[bestVertex] );
      }
    }

    if( i2 == R.edges.size() ) continue;

    it = find(vertices.begin(),vertices.end(),v1);

    int v1Index = it - vertices.begin();

    vertices.push_back( v1 );

    vertices.push_back( v2 );
    
    contributingEdges.push_back( Edge( vertices.size()-2, vertices.size()-1 ) );

    verticesUsed.push_back( bestVertex );

    edgeOffsets.push_back( P.vertices[bestVertex] );
    edgeOffsets.push_back( P.vertices[bestVertex] );
  }
}

Vertex midpoint( Vertex v1, Vertex v2 )
{
  return Vertex( ( v1.x + v2.x )/2, ( v1.y + v2.y )/2 );
}

bool validPoint( Vertex p, Vertex v1, Vertex v2 )
{
  return p.y <= v1.y && p.y <= v2.y && p != v1 && p != v2;
}

int windingNumber( Vertex p, vector< Vertex > &windingVertices )
{
  int n = 0;

  int s = windingVertices.size();

  for( int i = 0; i < s; ++i )
  {
    Vertex &v1 = windingVertices[i];
    Vertex &v2 = windingVertices[(i+1)%s];

    if( v1.y <= p.y )
    {
      if( v2.y > p.y && ccw( v1, p, v2 ) < 0 )
      {
        ++n;
      }
    }
    else
    {
      if( v2.y <= p.y && ccw( v1, p, v2 ) > 0 )
      {
        --n;
      }
    }
  }

  return n;
}

bool PolygonFitting::isValidFit( Vertex &c )
{
  for( int i = 0; i < contributingEdges.size(); ++i )
  {
    Vertex &v1 = vertices[contributingEdges[i].v1];
    Vertex &v2 = vertices[contributingEdges[i].v2];

    double minIX, maxIX, minIY, maxIY;

    if( v1.x < v2.x )
    {
      minIX = v1.x;
      maxIX = v2.x;
    }
    else
    {
      minIX = v2.x;
      maxIX = v1.x;
    }

    if( v1.y < v2.y )
    {
      minIY = v1.y;
      maxIY = v2.y;
    }
    else
    {
      minIY = v2.y;
      maxIY = v1.y;
    }

    for( int j = i + 1; j < contributingEdges.size(); ++j )
    {
      Vertex &v3 = vertices[contributingEdges[j].v1];
      Vertex &v4 = vertices[contributingEdges[j].v2];

      if( min(v3.x,v4.x) > maxIX || max(v3.x,v4.x) < minIX || min(v3.y,v4.y) > maxIY || max(v3.y,v4.y) < minIY ) continue;

      pair< double, double > t = intersection( v1, v2, v3, v4 );

      if( t.first < -0.0001 || t.first > 1.0001 || t.second < -0.0001 || t.second > 1.0001 )
      {
        continue;
      }

      Vertex p = intersectionPoint( t.first, v1, v2 );

      Vertex points[4] = { midpoint( v1, v3 ), midpoint( v2, v3 ), midpoint( v1, v4 ), midpoint( v2, v4 ) };

      bool valid[4] = { validPoint( p, v1, v3 ), validPoint( p, v2, v3 ), validPoint( p, v1, v4 ), validPoint( p, v2, v4 ) };

      for( int k = 0; k < 4; ++k )
      {
        if( !valid[k] ) continue;

        bool closestPointChosen = false;
        double closestDistance = 1000000000;

        
        for( int i2 = 0; i2 < contributingEdges.size(); ++i2 )
        {
          if( i2 == i || i2 == j ) continue;

          Vertex &v5 = vertices[contributingEdges[i2].v1];
          Vertex &v6 = vertices[contributingEdges[i2].v2];

          pair< double, double > t2 = intersection( p, points[k], v5, v6 );

          if( t2.second < 0 || t2.second > 1 ) continue;

          if( t2.first > 0 && t2.first < closestDistance )
          {
            closestPointChosen = true;
            closestDistance = t2.first;
          }
        }
        
        if( !closestPointChosen ) continue;

        Vertex p2 = intersectionPoint( closestDistance/2, p, points[k] );

        int w = windingNumber( p2, vertices );

        if( w == -1 )
        {
          c = p2;

          return true;
        }
      }
    }
  }

  return false;
}

struct Cmp
{
    bool operator ()(const pair< int, double> &a, const pair< int, double> &b)
    {
      if( a.first == b.first ) return false;
        return a.second < b.second;
    }
};

double angleBetweenVectors( Vertex v1, Vertex v2 )
{
  double dot = v1.x * v2.x + v1.y * v2.y;
  double det = v1.x * v2.x - v1.y * v2.y;

  return atan2( det, dot ) * 180.0 / 3.14159265358979;
}

bool PolygonFitting::findBestScale( double &maxScale, Vertex &bestFitOrigin )
{
  Vertex origin;

  bool scalingUp = true;
  int scaleMultiplier = 1;
  double scale = maxScale;
  double scaleOffset = 1;

  int numScaleDirectionChanges = 0;

  bool changeDirections = false;
  int numSinceDirectionChange = 0;

  bool isNewBestScale = false;

  contributingEdgesCopy = contributingEdges;

  vector< Vertex > verticesCopy = vertices;

  vector< Vertex > edgeOffsetsCopy = edgeOffsets;

  while( numSinceDirectionChange < 10 )
  {
    vertices = verticesCopy;

    for( int i = 0; i < contributingEdges.size(); ++i )
    {
      int v1 = contributingEdges[i].v1;
      int v2 = contributingEdges[i].v2;

      vertices[v1] = vertices[v1].offset( scale*edgeOffsets[2*i+0].x, scale*edgeOffsets[2*i+0].y );
      vertices[v2] = vertices[v2].offset( scale*edgeOffsets[2*i+1].x, scale*edgeOffsets[2*i+1].y );
    }

    if( isValidFit( origin ) )
    {
      if( scale+1 > maxScale )
      {
        bestFitOrigin = origin;
        maxScale = scale+1;
        isNewBestScale = true;
      }

      numScaleDirectionChanges += ( scaleMultiplier == -1 );
      changeDirections = ( changeDirections || ( scaleMultiplier == -1 ) );

      scaleMultiplier = 1;
    }
    else
    {
      numScaleDirectionChanges += ( scaleMultiplier == 1 ); 

      changeDirections = ( changeDirections || ( scaleMultiplier == 1 ) );

      scaleMultiplier = -1;

      if( scalingUp )
      {
        scalingUp = false;
        if(scaleOffset != 1) scaleOffset /= 2;
      }
    }

    numSinceDirectionChange += changeDirections;

    if( scale + 1 <= maxScale && scaleMultiplier < 0 )
    {
      return false;
    }

    if( scalingUp )
    {
      scale += scaleOffset;
      scaleOffset *= 2;
    }
    else
    {
      scaleOffset /= 2;
      scale += scaleMultiplier * scaleOffset;
    }
  }

  return isNewBestScale;
}

void findBestFit( Polygon P, Polygon R, double &scale, double &rotation, Vertex &offset, double rotationOffset, double startRotation )
{
  scale = 0;

  Vertex origin = Vertex( -1000000000, -1000000000 );

  for( int i = 0; i < R.vertices.size(); ++i )
  {
    Vertex v1 = R.vertices[i];

    origin.x = max( -v1.x, origin.x );
    origin.y = max( -v1.y, origin.y );
  }

  R.offsetBy( origin );

  R.scaleBy(10000);

  P.rotateBy(startRotation);

  for( double r = startRotation; r < 360; r += rotationOffset )
  {
    PolygonFitting pf( P, R );

    pf.computeContributingEdges();

    if( pf.findBestScale( scale, offset ) )
    {
      rotation = r;
    }

    if( r+rotationOffset < 360 )
    {
      P.rotateBy(rotationOffset);
    }
  }

  scale /= 10000;
  offset.x /= 10000;
  offset.y /= 10000;

  offset = offset.offset( -origin.x, -origin.y );
}

void drawImage( Polygon P, Polygon R, double scale, double rotation, Vertex offset, string outputName, double scaleUp )
{
  double width = 0;
  double height = 0;

  Vertex origin = Vertex( -1000000, -1000000 );

  for( int i = 0; i < R.vertices.size(); ++i )
  {
    Vertex v1 = R.vertices[i];

    width = max( v1.x, width );
    height = max( v1.y, height );

    origin.x = max( -v1.x, origin.x );
    origin.y = max( -v1.y, origin.y );
  }

  width += origin.x;
  height += origin.y;

  VImage image = VImage::black(width*scaleUp+4,height*scaleUp+4).invert();

  R.offsetBy( origin );
  R.scaleBy( scaleUp );
  R.offsetBy( Vertex( 2, 2) );

  vector< double > ink = {0};

  for( int i = 0; i < R.edges.size(); ++i )
  {
    Vertex v1 = R.vertices[R.edges[i].v1];
    Vertex v2 = R.vertices[R.edges[i].v2];

    image.draw_rect(ink,v1.x,v1.y,4,4);

    image.draw_line(ink,v1.x,v1.y,v2.x,v2.y);
  }

  P.center();
  P.rotateBy( rotation );
  P.scaleBy( scale );
  P.offsetBy( offset );

  P.offsetBy( origin );
  P.scaleBy( scaleUp );

  P.offsetBy( Vertex( 2, 2) );

  for( int i = 0; i < P.vertices.size(); ++i )
  {
    Vertex v1 = P.vertices[i];

    image.draw_rect(ink,v1.x,v1.y,4,4);
  }

  for( int i = 0; i < P.edges.size(); ++i )
  {
    Vertex v1 = P.vertices[P.edges[i].v1];
    Vertex v2 = P.vertices[P.edges[i].v2];

    image.draw_line(ink,v1.x,v1.y,v2.x,v2.y);
  }

  image.vipssave("out.png");
}

void drawImage( string imageName, Polygon R, double scale, double rotation, Vertex offset, string outputName, double scaleUp )
{
  double width = 0;
  double height = 0;

  Vertex origin = Vertex( -1000000, -1000000 );

  for( int i = 0; i < R.vertices.size(); ++i )
  {
    Vertex v1 = R.vertices[i];

    width = max( v1.x, width );
    height = max( v1.y, height );

    origin.x = max( -v1.x, origin.x );
    origin.y = max( -v1.y, origin.y );
  }

  width += origin.x;
  height += origin.y;

  VImage image = VImage::black(width*scaleUp+4,height*scaleUp+4).invert();
  image = image.bandjoin(image).bandjoin(image);

  R.offsetBy( origin );
  R.scaleBy( scaleUp );
  R.offsetBy( Vertex( 2, 2) );

  vector< double > ink = {0,0,0};

  for( int i = 0; i < R.edges.size(); ++i )
  {
    Vertex v1 = R.vertices[R.edges[i].v1];
    Vertex v2 = R.vertices[R.edges[i].v2];

    image.draw_line(ink,v1.x,v1.y,v2.x,v2.y);
  }

  VImage centerImage = VImage::vipsload((char *)imageName.c_str()).similarity( VImage::option()->set( "scale", scale*scaleUp )->set( "angle", rotation ));

  int left, top, centerImageWidth, centerImageHeight;

  left = centerImage.find_trim( &top, &centerImageWidth, &centerImageHeight );

  centerImage = centerImage.extract_area( left, top, centerImageWidth, centerImageHeight );

  int xOffset = (offset.x + origin.x)*scaleUp+2 - centerImage.width()/2;
  int yOffset = (offset.y + origin.y)*scaleUp+2 - centerImage.height()/2;

  centerImage = centerImage.embed(xOffset,yOffset,centerImage.width()+xOffset,centerImage.height()+yOffset);//->set( "idx", -(offset.x + origin.x) )->set( "idy", -(offset.y + origin.y)*scaleUp ));

  VImage mask = centerImage.extract_band(3);

  centerImage = centerImage.flatten();

  image = mask.ifthenelse( centerImage, image, VImage::option()->set( "blend", true ) );

  image.vipssave("out.png");
}

void polygonFromAlphaImage( Polygon &P, string imageName, double resize )
{
  VImage image = VImage::vipsload((char *)imageName.c_str()).autorot().colourspace(VIPS_INTERPRETATION_sRGB).resize(1.0/resize);

  int width = image.width();
  int height = image.height();

  if( image.bands() == 4 )
  {
    image = image.extract_band(3);

    unsigned char *data = ( unsigned char * )image.data();

    vector< Vertex > vertices;

    bool *valid = new bool[width*height];

    for( int i = 0, p = 0; i < height; ++i )
    {
      for( int j = 0; j < width; ++j, ++p )
      {
        valid[p] = data[p] > 0;
      }
    }

    for( int i = 0, p = 0; i < height; ++i )
    {
      for( int j = 0; j < width; ++j, ++p )
      {
        int numAround = 0;
        numAround += ( i>0 && valid[(i-1)*width+j] );
        numAround += ( i<height-1 && valid[(i+1)*width+j] );
        numAround += ( j>0 && valid[i*width+j-1] );
        numAround += ( j<width-1 && valid[i*width+j+1] );

        if( data[p] > 0 && numAround < 4 )
        {
          vertices.push_back( Vertex( j*resize, i*resize ) );
        }
      }
    }

    vector< Edge > edges = grahamScan( vertices );

    Vertex origin = Vertex( -1000000, -1000000 );

    for( int i = 0; i < edges.size(); ++i )
    {
      P.vertices.push_back( vertices[edges[i].v1] );
      P.edges.push_back( Edge( i, (i+1)%edges.size() ) );

      origin.x = max( -vertices[edges[i].v1].x, origin.x );
      origin.y = max( -vertices[edges[i].v1].y, origin.y );
    }

    P.offsetBy(origin);
  }
}

void removeExtrasP( Polygon &P, double epsilon )
{
  vector< int > indices(P.vertices.size());

  for( int i = 0; i < P.vertices.size(); ++i )
  {
    indices[i] = find(P.vertices.begin(),P.vertices.end(),P.vertices[i]) - P.vertices.begin();
  }

  for( int i = 0; i < P.edges.size(); ++i )
  {
    P.edges[i].v1 = indices[P.edges[i].v1];
    P.edges[i].v2 = indices[P.edges[i].v2];
  }

  // Remove non loop branches until there aren't any more
  for( bool removed = true; removed == true; )
  {
    removed = false;
    vector< int > count = vector< int >( P.vertices.size(), 0 );

    for( int i = 0; i < P.edges.size(); ++i )
    {
      count[ P.edges[i].v1 ]++;
      count[ P.edges[i].v2 ]++;
    }

    for( int i = P.edges.size()-1; i >= 0; --i )
    {
      if( count[ P.edges[i].v1 ] < 2 || count[ P.edges[i].v2 ] < 2 )
      {
        --count[ P.edges[i].v1 ];
        --count[ P.edges[i].v2 ];
        P.edges.erase(P.edges.begin() + i);
        removed = true;
      }
    }
  }

  vector< int > count = vector< int >( P.vertices.size(), 0 );

  vector< int > offsets(P.vertices.size(),0);

  for( int i = 0; i < P.edges.size(); ++i )
  {
    count[ P.edges[i].v1 ]++;
    count[ P.edges[i].v2 ]++;
  }

  int offsetCount = 0;

  for( int i = 0, i2 = 0; i < P.vertices.size(); ++i, ++i2 )
  {
    if( count[i2] < 2 )
    {
      P.vertices.erase(P.vertices.begin()+i);
      --i;
    }
    else
    {
      offsets[i2] = i;
    }
  }

  for( int i = 0; i < P.edges.size(); ++i )
  {
    P.edges[i].v1 = offsets[P.edges[i].v1];
    P.edges[i].v2 = offsets[P.edges[i].v2];
  }
}

void floodFill( bool *valid, unsigned char *data, int width, int height )
{
  int *filled = new int[width*height];

  for( int i = 0, p = 0; i < height; ++i )
  {
    for( int j = 0; j < width; ++j, ++p )
    {
      filled[p] = (data[p] > 40)?1:0;
    }
  }

  set< int > valuesToCheck;

  for( int i = 0; i < width; ++i )
  {
    if( filled[i] == 0 )
    {
      valuesToCheck.insert( i );
    }
    if( filled[(height-1)*width+i] == 0 )
    {
      valuesToCheck.insert( (height-1)*width+i );
    }
  }

  for( int i = 0; i < height; ++i )
  {
    if( filled[i*width] == 0 )
    {
      valuesToCheck.insert( i*width );
    }
    if( filled[i*width+width-1] == 0 )
    {
      valuesToCheck.insert( i*width+width-1 );
    }
  }

  bool hasUpdated = true;

  set< int >::iterator start = valuesToCheck.begin(), end = valuesToCheck.end();

  while( hasUpdated )
  {
    hasUpdated = false;

    for( ; start != end; ++start )
    {
      int p = *start;

      if( p%width > 0 && filled[ p-1 ] == 0 )
      {
        valuesToCheck.insert(p-1);
        filled[p-1] = 2;
        hasUpdated = true;
      }

      if( p%width < width-1 && filled[ p+1 ] == 0 )
      {
        valuesToCheck.insert(p+1);
        filled[p+1] = 2;
        hasUpdated = true;
      }

      if( p > width-1 && filled[ p-width ] == 0 )
      {
        valuesToCheck.insert(p-width);
        filled[p-width] = 2;
        hasUpdated = true;
      }

      if( p < width*(height-1)-1 && filled[ p+width ] == 0 )
      {
        valuesToCheck.insert(p+width);
        filled[p+width] = 2;
        hasUpdated = true;
      }
    }

    start = valuesToCheck.begin();
    end = valuesToCheck.end();
  }

  for( int i = 0, p = 0; i < height; ++i )
  {
    for( int j = 0; j < width; ++j, ++p )
    {
      valid[p] = false;
      if( filled[p] != 1 ) continue;

      bool isConnected = false;
      isConnected = isConnected || ( p%width < 1 || filled[ p-1 ] == 2 );
      isConnected = isConnected || ( p%width > width-2 || filled[ p+1 ] == 2 );
      isConnected = isConnected || ( p < width || filled[ p-width ] == 2 );
      isConnected = isConnected || ( p > width*(height-2)-1 || filled[ p+width ] == 2 );

      valid[p] = isConnected;
    }
  }
}

void concavePolygonFromAlphaImage( Polygon &P, string imageName, double resize )
{
  VImage image = VImage::vipsload((char *)imageName.c_str()).autorot().colourspace(VIPS_INTERPRETATION_sRGB).resize(1.0/resize);

  int width = image.width();
  int height = image.height();

  bool *valid = new bool[width*height];

  if( image.bands() == 4 )
  {
    image = image.extract_band(3);

    vector< Vertex > vertices;
    vector< Edge > edges;

    unsigned char *data = ( unsigned char * )image.data();

    floodFill( valid, data, width, height );

    for( int i = 0, p = 0; i < height; ++i )
    {
      for( int j = 0; j < width; ++j, ++p )
      {
        if( j > 0 && valid[p] != false && valid[p-1] != false )
        {
          vertices.push_back( Vertex( j*resize, i*resize ) );
          vertices.push_back( Vertex( (j-1)*resize, i*resize ) );
          edges.push_back( Edge( vertices.size()-2, vertices.size()-1 ) );
        }

        if( i > 0 )
        {
          for( int l = ((j == 0) ? 0 : -1); l <= ((j == width - 1) ? 0 : 1); ++l )
          {
            if( valid[p] != false && valid[p-width+l] != false )
            {
              vertices.push_back( Vertex( j*resize, i*resize ) );
              vertices.push_back( Vertex( (j+l)*resize, (i-1)*resize ) );
              edges.push_back( Edge( vertices.size()-2, vertices.size()-1 ) );
            }
          }
        }
      }
    }

    for( int i = 0; i < edges.size(); ++i )
    {
      Vertex v1 = vertices[edges[i].v1];
      Vertex v2 = vertices[edges[i].v2];

      P.addVertex(v1);
      P.addVertex(v2);
      P.addEdge(P.vertices.size()-2,P.vertices.size()-1);
    }

    Vertex origin = Vertex( -1000000, -1000000 );

    for( int i = 0; i < P.vertices.size(); ++i )
    {
      origin.x = max( -P.vertices[i].x, origin.x );
      origin.y = max( -P.vertices[i].y, origin.y );
    }

    P.offsetBy(origin);
  }
}

void convexPolygonFromVertices( vector< Vertex > &vertices, Polygon &P )
{
  vector< Edge > edges = grahamScan( vertices );

  Vertex origin = Vertex( -1000000, -1000000 );

  for( int i = 0; i < edges.size(); ++i )
  {
    P.vertices.push_back( vertices[edges[i].v1] );
    P.edges.push_back( Edge( i, (i+1)%edges.size() ) );

    origin.x = max( -vertices[edges[i].v1].x, origin.x );
    origin.y = max( -vertices[edges[i].v1].y, origin.y );
  }

  P.offsetBy(origin);
}

void decimateHelper( Polygon &P, double epsilon )
{
  vector< int > indices(P.vertices.size());

  for( int i = 0; i < P.vertices.size(); ++i )
  {
    indices[i] = find(P.vertices.begin(),P.vertices.end(),P.vertices[i]) - P.vertices.begin();
  }

  for( int i = 0; i < P.edges.size(); ++i )
  {
    P.edges[i].v1 = indices[P.edges[i].v1];
    P.edges[i].v2 = indices[P.edges[i].v2];
  }

  for( bool removed = true; removed;  )
  {
    removed = false;

    int edgeSize = P.edges.size();

    for( int i = P.edges.size()-1; i >= 0; --i )
    {
      Vertex v1 = P.vertices[P.edges[i].v1];
      Vertex v2 = P.vertices[P.edges[i].v2];

      vector< int > count = vector< int >( P.vertices.size(), 0 );

      for( int i2 = 0; i2 < P.edges.size(); ++i2 )
      {
        count[ P.edges[i2].v1 ]++;
        count[ P.edges[i2].v2 ]++;
      }

      for( int j = edgeSize-1; j >= 0; --j )
      {
        if( i == j ) continue;

        Vertex v3;
        int v3Index;
        if( v2 == P.vertices[P.edges[j].v1] )
        {
          v3 = P.vertices[P.edges[j].v2];
          v3Index = P.edges[j].v2;
        }
        else if( v2 == P.vertices[P.edges[j].v2] )
        {
          v3 = P.vertices[P.edges[j].v1];
          v3Index = P.edges[j].v1;
        }
        else
        {
          continue;
        }

        if( distanceToLine( v2, v1, v3 ) < epsilon && count[P.edges[i].v2] < 3 )
        {
          P.edges[min(i,j)].v1 = P.edges[i].v1;
          P.edges[min(i,j)].v2 = v3Index;

          P.edges.erase(P.edges.begin()+max(i,j));
          --i;
          edgeSize -= 1;

          removed = true;
          break;
        }
        
        if( distanceToLine( v2, v1, v3 ) < epsilon && count[P.edges[i].v2] == 3 )
        {
          P.edges[max(i,j)].v1 = P.edges[i].v1;
          P.edges[max(i,j)].v2 = v3Index;

          removed = true;
          break;
        }
        

      }
    }

    for( int i = P.edges.size()-1; i >= 0; --i )
    {
      Vertex v1 = P.vertices[P.edges[i].v1];
      Vertex v2 = P.vertices[P.edges[i].v2];

      if( v1 == v2 )
      {
        P.edges.erase(P.edges.begin()+i);
        removed = true;
      }
    }

    for( int i = P.edges.size()-1; i >= 0; --i )
    {
      Vertex v1 = P.vertices[P.edges[i].v1];
      Vertex v2 = P.vertices[P.edges[i].v2];

      for( int j = P.edges.size()-1; j >= 0; --j )
      {
        if( i == j ) continue;

        Vertex v3 = P.vertices[P.edges[j].v1];
        Vertex v4 = P.vertices[P.edges[j].v2];

        if( ( v1 == v3 && v2 == v4 ) || ( v1 == v4 && v2 == v3 ) )
        {
          P.edges.erase(P.edges.begin()+max(i,j));
          removed = true;
          break;
        }
      }
    }

  }
}

void decimate( Polygon &P, double epsilon )
{
  double minLength = P.minLength();

  P.scaleBy(1.0/minLength);

  decimateHelper(P,epsilon);

  for( int i = 0; i < P.vertices.size(); ++i )
  {
    for( int j = i+1; j < P.vertices.size(); ++j )
    {
      if( distance(P.vertices[i],P.vertices[j]) < epsilon*epsilon )
      {
        P.vertices[j] = P.vertices[i];
      }
    }
  }

  vector< Vertex > vertices;

  for( int i = 0; i < P.edges.size(); ++i )
  {
    vertices.push_back( P.vertices[P.edges[i].v1] );
    vertices.push_back( P.vertices[P.edges[i].v2] );
  }

  P.vertices.clear();
  P.edges.clear();

  for( int i = 0; i < vertices.size(); i+=2 )
  {
    P.vertices.push_back( vertices[i] );
    P.vertices.push_back( vertices[i+1] );

    P.addEdge( Edge( i, i+1 ) );
  }

  P.scaleBy(minLength);
}