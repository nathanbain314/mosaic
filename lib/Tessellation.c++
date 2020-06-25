#include "Tessellation.h"

Quad::Quad(vector< float > restrictionPoint )
{
  _x[0] = restrictionPoint[0];
  _x[1] = restrictionPoint[1];
  _x[2] = restrictionPoint[0];
  _x[3] = restrictionPoint[1];

  _y[0] = restrictionPoint[2];
  _y[1] = restrictionPoint[3];
  _y[2] = restrictionPoint[4];
  _y[3] = restrictionPoint[5];

  _s[0] = (_y[1]-_y[0])/(_x[1]-_x[0]);
  _s[1] = (_y[3]-_y[2])/(_x[3]-_x[2]);
}

Quad::Quad( const Quad& rhs )
{
  for( int i = 0; i < 4; ++i )
  {
    _x[i] = rhs._x[i];
    _y[i] = rhs._y[i];
  }

  _s[0] = rhs._s[0];
  _s[1] = rhs._s[1];
}

Quad& Quad::operator=( const Quad& rhs )
{
  for( int i = 0; i < 4; ++i )
  {
    _x[i] = rhs._x[i];
    _y[i] = rhs._y[i];
  }

  _s[0] = rhs._s[0];
  _s[1] = rhs._s[1];

  return *this;
}

bool Quad::isInside( float x, float y, float mosaicTileWidth, float mosaicTileHeight )
{
  if( 0 >= floor(_x[0]*mosaicTileWidth-x) && 0 <= ceil(_x[1]*mosaicTileWidth-x) && 0 >= floor(mosaicTileHeight / mosaicTileWidth * _s[0] * ( x - _x[0]*mosaicTileWidth ) + _y[0]*mosaicTileHeight - y) && 0 <= ceil(mosaicTileHeight / mosaicTileWidth * _s[1] * ( x - _x[2]*mosaicTileWidth ) +  _y[2]*mosaicTileHeight - y ) )
  {
    return true;
  }
  return false;
}


Tessellation::Tessellation( vector< vector< float > > restrictionPoints )
{
  for( int i = 0; i < restrictionPoints.size(); ++i )
  {
    _quads.push_back(Quad(restrictionPoints[i]));
  }
}

Tessellation::Tessellation( const Tessellation& rhs )
{
  _quads = rhs._quads;
}

Tessellation& Tessellation::operator=( const Tessellation& rhs )
{
  _quads = rhs._quads;

  return *this;
}

void Tessellation::createIndices( vector< int > &shapeIndices, int mosaicTileWidth, int mosaicTileHeight, int tileWidth, int tileHeight )
{
  for( int y = 0, p = 0; y < tileHeight; ++y )
  {
    for( int x = 0; x < tileWidth; ++x, ++p )
    {
      bool goodPoint = false;

      for( int i = 0; i < _quads.size(); ++i )
      {
        if( _quads[i].isInside(x,y,mosaicTileWidth,mosaicTileHeight) )
        {
          goodPoint = true;
        }
      }

      if( goodPoint ) shapeIndices.push_back(p);
    }
  }
}