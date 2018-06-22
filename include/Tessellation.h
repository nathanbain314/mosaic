#include "mosaicTools.h"

class Quad
{
  double _x[4], _y[4], _s[2];

public:
  Quad( vector< double > restrictionPoints );

  Quad( const Quad& rhs );

  Quad& operator=( const Quad& rhs );


  bool isInside( double x, double y, double mosaicTileWidth, double mosaicTileHeight ); 
};

class Tessellation
{
  vector< Quad > _quads;

public:
  Tessellation( vector< vector< double > > restrictionPoints );

  Tessellation( const Tessellation& rhs );

  Tessellation& operator=( const Tessellation& rhs );

  void createIndices( vector< int > &shapeIndices, int mosaicTileWidth, int mosaicTileHeight );
};
