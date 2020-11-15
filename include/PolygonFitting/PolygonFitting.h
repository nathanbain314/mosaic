#include "PolygonFitting/Polygon.h"

vector< Edge > grahamScan( vector< Vertex > vertices );

class PolygonFitting
{
public:
  Polygon P, R, bestFit;

  vector< Polygon > polygons;
  vector< Edge > edges;
  vector< Vertex > vertices, fullVertices;
  vector< Edge > contributingEdges, contributingEdgesCopy;  
  vector< Vertex > edgeOffsets;
  vector< int > contributingVertices;  

  PolygonFitting(){}

  PolygonFitting( Polygon _P, Polygon _R );

  PolygonFitting( const PolygonFitting& rhs );

  PolygonFitting& operator=( const PolygonFitting& rhs );

  void createOffsetEdges();

  void computeContributingEdges();

  void computeIntersections( bool removeNonCycles );

  void checkIfValidFit();

  bool findBestScale( float &maxScale, Vertex &bestFitOrigin );

  bool isValidFit( Vertex &c );

  void drawImageFit( float maxScale, Vertex bestFitOrigin );
};

void findBestFit( Polygon P2, Polygon R, float &scale, float &rotation, Vertex &offset, float rotationOffset = 1.0, float maxAngle = 180.0 );

void drawImage( Polygon P, Polygon R, float scale, float rotation, Vertex offset, string outputName, float scaleUp );

void drawImage( string imageName, Polygon R, float scale, float rotation, Vertex offset, string outputName, float scaleUp );

void removeExtrasP( Polygon &P, float epsilon = -1 );

void polygonFromAlphaImage( Polygon &P, string imageName, float resize = 1.0 );

void concavePolygonFromAlphaImage( Polygon &P, string imageName, float resize );

void convexPolygonFromVertices( vector< Vertex > &vertices, Polygon &P );

float ccw( Vertex &v1, Vertex &v2, Vertex &v3 );

bool minY(Vertex i, Vertex j);

float distance( Vertex v1, Vertex v2 );

bool grahamScanSort ( Vertex v1, Vertex v2, Vertex p );

vector< Edge > grahamScan( vector< Vertex > vertices );

Edge reverseEdge( Edge e );

pair< float, float > intersection( Vertex &v1, Vertex &v2, Vertex &v3, Vertex &v4 );

Vertex intersectionPoint( float t, Vertex &v1, Vertex &v2 );

bool isOnLineSegment( Vertex v1, Vertex v2, Vertex v3 );

float intersectionEdges( Vertex v1, Vertex v2, Vertex v3, Vertex v4, Vertex v5, Vertex v6, Vertex o1, Vertex o2, Vertex o3 );

void decimate( Polygon &P, float epsilon );