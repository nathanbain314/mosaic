#include "Tessellation.h"

Quad::Quad(vector< double > restrictionPoint )
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

bool Quad::isInside( double x, double y, double mosaicTileWidth, double mosaicTileHeight )
{
//  cout << x << " " << y << " " << _s[0] << " " << _s[1] << " " << mosaicTileWidth << " " << mosaicTileHeight << " " << ( x >= _x[0] && y - _y[0]*mosaicTileHeight >= _s[0] * ( x - _x[0]*mosaicTileWidth ) && y - _y[2]*mosaicTileHeight <= _s[1] * ( x - _x[2]*mosaicTileWidth ) ) << endl;
  if( x >= round(_x[0]*mosaicTileWidth) && x <= round(_x[1]*mosaicTileWidth) && round(y - _y[0]*mosaicTileHeight) >= round(mosaicTileHeight / mosaicTileWidth * _s[0] * ( x - _x[0]*mosaicTileWidth )) && round(y - _y[2]*mosaicTileHeight) <= round(mosaicTileHeight / mosaicTileWidth * _s[1] * ( x - _x[2]*mosaicTileWidth )) )
  {
    return true;
  }
  return false;
}


Tessellation::Tessellation( vector< vector< double > > restrictionPoints )
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

void Tessellation::createIndices( vector< int > &shapeIndices, int mosaicTileWidth, int mosaicTileHeight )
{
  for( int y = 0, p = 0; y < mosaicTileHeight; ++y )
  {
    for( int x = 0; x < mosaicTileWidth; ++x, ++p )
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