#include "mosaicTools.h"

class Quad
{
  float _x[4], _y[4], _s[2];

public:
  Quad( vector< float > restrictionPoints );

  Quad( const Quad& rhs );

  Quad& operator=( const Quad& rhs );


  bool isInside( float x, float y, float mosaicTileWidth, float mosaicTileHeight ); 
};

class Tessellation
{
  vector< Quad > _quads;

public:
  Tessellation( vector< vector< float > > restrictionPoints );

  Tessellation( const Tessellation& rhs );

  Tessellation& operator=( const Tessellation& rhs );

  void createIndices( vector< int > &shapeIndices, int mosaicTileWidth, int mosaicTileHeight, int tileWidth, int tileHeight );
};
