#include "shapeIndices.h"

void createCircleIndices( vector< int > &shapeIndices, int mosaicTileWidth, int mosaicTileHeight, int tileWidth, int tileHeight )
{
  for( int y = -tileHeight/2, p = 0; y < tileHeight/2; ++y )
  {
    for( int x = -tileWidth/2; x < tileWidth/2; ++x, ++p )
    {
      bool goodPoint = false;

      if( (double)(4*x * x) / ( tileWidth * tileWidth ) + (double)(4*y * y) / ( tileHeight * tileHeight ) <= 1.0 )
      {
        goodPoint = true;
      }

      if( goodPoint ) shapeIndices.push_back(p);
    }
  }
}

void createJigsawIndices( vector< vector < int > > &shapeIndices, double mosaicTileWidth, double mosaicTileHeight, vector< int > &tileWidth, vector< int > &tileHeight )
{
  double pi = 3.14159265358979;

  for( int i = -1, n = 0; i <= 1; i += 2 )
  {
    for( int j = -1; j <= 1; j += 2 )
    {
      double yOffset = mosaicTileHeight * (.3*(i>0));
      for( int k = -1; k <= 1; k += 2 )
      {
        for( int l = -1; l <= 1; l += 2, ++n )
        {
          double xOffset = mosaicTileWidth * (.3*(k>0));

          for( int y = 0, p = 0; y < tileHeight[n]; ++y )
          {
            for( int x = 0; x < tileWidth[n]; ++x, ++p )
            {
              bool goodPoint = false;
/*
              if( x < 0.3 * mosaicTileWidth && y > 0.3 * mosaicTileHeight && y < 0.7 * mosaicTileHeight )
              {
                if( -10.0 * x / mosaicTileWidth - pi / 4 + 3 - 1.5 * sin( -10.0 * x / mosaicTileWidth - pi / 4 + 3 ) - sin( 10.0 * y / mosaicTileHeight + pi / 2 - 5 ) < 0 )
                {
                  goodPoint = true;
                }
              }
*/

              if( x >= xOffset + 0.3 * mosaicTileWidth && x <= xOffset + 0.7 * mosaicTileWidth )
              {
                /*
                if( y < yOffset && -10.0 * x / mosaicTileWidth - pi / 4 + 3*(k>0) - 1.5 * sin( -10.0 * x / mosaicTileWidth - pi / 4 + 3*(k>0) ) - sin( 10.0 * y / mosaicTileHeight + pi / 2 - 5 - 3*(i>0) ) < 0 )
                {
                  goodPoint = true;
                }
*/
                /*
                if( y >= yOffset + mosaicTileHeight && 10.0 * x / mosaicTileWidth - pi / 4 - 10 - 3*(k>0) - 1.5 * sin( 10.0 * x / mosaicTileWidth - pi / 4 - 10 - 3*(k>0) ) - sin( 10.0 * y / mosaicTileHeight + pi / 2 - 5 - 3*(i>0) ) < 0 )
                {
                  goodPoint = true;
                }*/
                if( y < yOffset && -10.0 * y / mosaicTileHeight - pi / 4 + 3*(i>0) - 1.5 * sin( -10.0 * y / mosaicTileHeight - pi / 4 + 3*(i>0) ) - sin( 10.0 * x / mosaicTileWidth + pi / 2 - 5 - 3*(k>0) ) < 0 )
                {
                  goodPoint = true;
                }

                if( y >= yOffset + mosaicTileHeight && 10.0 * y / mosaicTileHeight - pi / 4 - 10 - 3*(i>0) - 1.5 * sin( 10.0 * y / mosaicTileHeight - pi / 4 - 10 - 3*(i>0) ) - sin( 10.0 * x / mosaicTileWidth + pi / 2 - 5 - 3*(k>0) ) < 0 )
                {
                  goodPoint = true;
                }
              }

              if( y >= yOffset + 0.3 * mosaicTileHeight && y <= yOffset + 0.7 * mosaicTileHeight )
              {
                if( x < xOffset && -10.0 * x / mosaicTileWidth - pi / 4 + 3*(k>0) - 1.5 * sin( -10.0 * x / mosaicTileWidth - pi / 4 + 3*(k>0) ) - sin( 10.0 * y / mosaicTileHeight + pi / 2 - 5 - 3*(i>0) ) < 0 )
                {
                  goodPoint = true;
                }

                if( x >= xOffset + mosaicTileWidth && 10.0 * x / mosaicTileWidth - pi / 4 - 10 - 3*(k>0) - 1.5 * sin( 10.0 * x / mosaicTileWidth - pi / 4 - 10 - 3*(k>0) ) - sin( 10.0 * y / mosaicTileHeight + pi / 2 - 5 - 3*(i>0) ) < 0 )
                {
                  goodPoint = true;
                }
              }

              if( x >= xOffset && x < xOffset + mosaicTileWidth && y >= yOffset && y < yOffset + mosaicTileHeight )
              {
                goodPoint = true;

                if( x >= xOffset + 0.3 * mosaicTileWidth && x <= xOffset + 0.7 * mosaicTileWidth )
                {
                  if( i < 0 && 10.0/mosaicTileHeight * y + 3*(i>0) - pi / 4 - 1.5 * sin( 10.0/mosaicTileHeight * y + 3*(i>0) - pi / 4 ) - sin( 10.0/mosaicTileWidth * x - 5 - 3*(k>0) + pi / 2 ) <= 0 )
                  {
                    goodPoint = false;
                  }
                  if( j < 0 && -10.0/mosaicTileHeight * y + 10 + 3*(i>0) - pi / 4 - 1.5 * sin( -10.0/mosaicTileHeight * y + 10 + 3*(i>0) - pi / 4 ) - sin( 10.0/mosaicTileWidth * x - 5 - 3*(k>0) + pi / 2 ) <= 0 )
                  {
                    goodPoint = false;
                  }
                }
                
                if( y >= yOffset + 0.3 * mosaicTileHeight && y <= yOffset + 0.7 * mosaicTileHeight )
                {
                  if( k < 0 && 10.0/mosaicTileWidth * x + 3*(k>0) - pi / 4 - 1.5 * sin( 10.0/mosaicTileWidth * x + 3*(k>0) - pi / 4 ) - sin( 10.0/mosaicTileHeight * y - 5 - 3*(i>0) + pi / 2 ) <= 0 )
                  {
                    goodPoint = false;
                  }
                  if( l < 0 && -10.0/mosaicTileWidth * x + 10 + 3*(k>0) - pi / 4 - 1.5 * sin( -10.0/mosaicTileWidth * x + 10 + 3*(k>0) - pi / 4 ) - sin( 10.0/mosaicTileHeight * y - 5 - 3*(i>0) + pi / 2 ) <= 0 )
                  {
                    goodPoint = false;
                  }
                }
              }
/*
              if( x > 1.3 * mosaicTileWidth && x < 1.6 * mosaicTileWidth && y > 0.3 * mosaicTileHeight && y < 0.7 * mosaicTileHeight )
              {
                if( 10.0 * x / mosaicTileWidth - pi / 4 - 13 - 1.5 * sin( 10.0 * x / mosaicTileWidth - pi / 4 - 13 ) - sin( 10.0 * y / mosaicTileHeight + pi / 2 - 5 ) < 0 )
                {
                  goodPoint = true;
                }
              }
*/
              if( goodPoint ) shapeIndices[n].push_back(p);
            }
          }
        }
      }
    }
  }
}

void createJigsawIndices0( vector< int > &shapeIndices, double mosaicTileWidth, double mosaicTileHeight, int tileWidth, int tileHeight )
{
  double pi = 3.14159265358979;
  for( int y = 0, p = 0; y < tileHeight; ++y )
  {
    for( int x = 0; x < tileWidth; ++x, ++p )
    {
      bool goodPoint = false;

      if( x < 0.3 * mosaicTileWidth && y > 0.3 * mosaicTileHeight && y < 0.7 * mosaicTileHeight )
      {
        if( -10.0 * x / mosaicTileWidth - pi / 4 + 3 - 1.5 * sin( -10.0 * x / mosaicTileWidth - pi / 4 + 3 ) - sin( 10.0 * y / mosaicTileHeight + pi / 2 - 5 ) < 0 )
        {
          goodPoint = true;
        }
      }

      if( x >= 0.3 * mosaicTileWidth && x <= 1.3 * mosaicTileWidth && y >= 0 && y < mosaicTileHeight )
      {
        goodPoint = true;

        if( x >= 0.6 * mosaicTileWidth && x <= 1.0 * mosaicTileWidth )
        {
          if( 10.0/mosaicTileHeight * y - pi / 4 - 1.5 * sin( 10.0/mosaicTileHeight * y - pi / 4 ) - sin( 10.0/mosaicTileWidth * x - 8 + pi / 2 ) <= 0 )
          {
            goodPoint = false;
          }
          if( -10.0/mosaicTileHeight * y + 10 - pi / 4 - 1.5 * sin( -10.0/mosaicTileHeight * y + 10 - pi / 4 ) - sin( 10.0/mosaicTileWidth * x - 8 + pi / 2 ) <= 0 )
          {
            goodPoint = false;
          }
        }
      }

      if( x > 1.3 * mosaicTileWidth && x < 1.6 * mosaicTileWidth && y > 0.3 * mosaicTileHeight && y < 0.7 * mosaicTileHeight )
      {
        if( 10.0 * x / mosaicTileWidth - pi / 4 - 13 - 1.5 * sin( 10.0 * x / mosaicTileWidth - pi / 4 - 13 ) - sin( 10.0 * y / mosaicTileHeight + pi / 2 - 5 ) < 0 )
        {
          goodPoint = true;
        }
      }

      if( goodPoint ) shapeIndices.push_back(p);
    }
  }
}

void createJigsawIndices1( vector< int > &shapeIndices, double mosaicTileWidth, double mosaicTileHeight, int tileWidth, int tileHeight )
{
  double pi = 3.14159265358979;
  for( int y = 0, p = 0; y < tileHeight; ++y )
  {
    for( int x = 0; x < tileWidth; ++x, ++p )
    {
      bool goodPoint = false;

      if( y < 0.3 * mosaicTileHeight && x > 0.3 * mosaicTileWidth && x < 0.7 * mosaicTileWidth )
      {
        if( -10.0 * y / mosaicTileHeight - pi / 4 + 3 - 1.5 * sin( -10.0 * y / mosaicTileHeight - pi / 4 + 3 ) - sin( 10.0 * x / mosaicTileWidth + pi / 2 - 5 ) < 0 )
        {
          goodPoint = true;
        }
      }

      if( y >= 0.3 * mosaicTileHeight && y <= 1.3 * mosaicTileHeight && x >= 0 && x < mosaicTileWidth )
      {
        goodPoint = true;

        if( y >= 0.6 * mosaicTileHeight && y <= 1.0 * mosaicTileHeight )
        {
          if( 10.0/mosaicTileWidth * x - pi / 4 - 1.5 * sin( 10.0/mosaicTileWidth * x - pi / 4 ) - sin( 10.0/mosaicTileHeight * y - 8 + pi / 2 ) <= 0 )
          {
            goodPoint = false;
          }
          if( -10.0/mosaicTileWidth * x + 10 - pi / 4 - 1.5 * sin( -10.0/mosaicTileWidth * x + 10 - pi / 4 ) - sin( 10.0/mosaicTileHeight * y - 8 + pi / 2 ) <= 0 )
          {
            goodPoint = false;
          }
        }
      }

      if( y > 1.3 * mosaicTileHeight && y < 1.6 * mosaicTileHeight && x > 0.3 * mosaicTileWidth && x < 0.7 * mosaicTileWidth )
      {
        if( 10.0 * y / mosaicTileHeight - pi / 4 - 13 - 1.5 * sin( 10.0 * y / mosaicTileHeight - pi / 4 - 13 ) - sin( 10.0 * x / mosaicTileWidth + pi / 2 - 5 ) < 0 )
        {
          goodPoint = true;
        }
      }


      if( goodPoint ) shapeIndices.push_back(p);
    }
  }
}