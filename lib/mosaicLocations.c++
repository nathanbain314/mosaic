#include "mosaicLocations.h"

void createLocations( vector< vector< int > > &mosaicLocations, vector< vector< int > > &mosaicLocations2, vector< vector< int > > &edgeLocations, vector< vector< int > > &edgeLocations2, vector< vector< float > > &offsets, vector< vector< float > > &dimensions, int mosaicTileWidth, int mosaicTileHeight, int imageTileWidth, int imageTileHeight, int width, int height )
{
  for( int i = 0; i < offsets.size(); ++i )
  {
    float w = (float) mosaicTileWidth;
    float h = (float) mosaicTileHeight;

    float xHorizontalOffset = w * offsets[i][2];
    float yHorizontalOffset = h * offsets[i][3];

    float xVerticalOffset = w * offsets[i][4];
    float yVerticalOffset = h * offsets[i][5];

    float w2 = (float) imageTileWidth;
    float h2 = (float) imageTileHeight;

    float xHorizontalOffset2 = w2 * offsets[i][2];
    float yHorizontalOffset2 = h2 * offsets[i][3];

    float xVerticalOffset2 = w2 * offsets[i][4];
    float yVerticalOffset2 = h2 * offsets[i][5];

    float tileWidth = w * dimensions[offsets[i][6]][0];
    float tileHeight = h * dimensions[offsets[i][6]][1];

    float tileWidth2 = w2 * dimensions[offsets[i][6]][0];
    float tileHeight2 = h2 * dimensions[offsets[i][6]][1];

    for( float ySign = -1; ySign <= 1; ySign += 2 )
    {
      for( float xSign = -1; xSign <= 1; xSign += 2 )
      {
        float startX = w * offsets[i][0] + tileWidth;
        float startY = h * offsets[i][1] + tileHeight;

        float startX2 = w2 * offsets[i][0] + tileWidth2;
        float startY2 = h2 * offsets[i][1] + tileHeight2;

        for( float y = startY, y2 = startY2; (ySign > 0 && startY < 3*height/2) || (ySign < 0 && startY > -3*height/2); startY += ySign*yVerticalOffset, startY2 += ySign*yVerticalOffset2, startX += ySign*xVerticalOffset, startX2 += ySign*xVerticalOffset2)// y += yOffset, y2 += yOffset2 )
        {
          for( float x = startX, x2 = startX2, y = startY, y2 = startY2; (xSign > 0 && x < 3*width/2) || (xSign < 0 && x > -3*width/2); x += xSign*xHorizontalOffset, x2 += xSign*xHorizontalOffset2, y += xSign*yHorizontalOffset, y2 += xSign*yHorizontalOffset2 )
          {
            int cornersInside = 0;
            for( int l = 0; l <= 1; ++l )
            {
              for( int k = 0; k <= 1; ++k )
              {
                if( x - k * tileWidth >= 0 && y - l * tileHeight >= 0 && x - k * tileWidth < width && y - l * tileHeight < height )
                {
                  ++cornersInside;
                }
              }
            }

            if( cornersInside == 4 )
            {
              mosaicLocations.push_back({ (int)round(x-tileWidth), (int)round(y-tileHeight), (int)round(x), (int)round(y), int(offsets[i][6]) } );

              mosaicLocations2.push_back({ (int)round(x2-tileWidth2), (int)round(y2-tileHeight2), (int)round(x2), (int)round(y2), int(offsets[i][6]) } );
            }
            else if( cornersInside > 0 )
            {
              edgeLocations.push_back({ (int)round(x-tileWidth), (int)round(y-tileHeight), (int)round(x), (int)round(y), int(offsets[i][6]) } );

              edgeLocations2.push_back({ (int)round(x2-tileWidth2), (int)round(y2-tileHeight2), (int)round(x2), (int)round(y2), int(offsets[i][6]) } );
            }
          }
        }
      }
    }
  }
}

void createJigsawLocations( vector< vector< int > > &mosaicLocations, vector< vector< int > > &mosaicLocations2, vector< vector< int > > &edgeLocations, vector< vector< int > > &edgeLocations2, int mosaicTileWidth, int mosaicTileHeight, int imageTileWidth, int imageTileHeight, int width, int height )
{
  int numHorizontal = width / mosaicTileWidth;
  int numVertical = height / mosaicTileHeight;

  int grid[numVertical][numHorizontal];

  for( int y = 0; y < numVertical; ++y )
  {
    for( int x = 0; x < numHorizontal; ++x )
    {
      vector< int > indices(16);
      iota( indices.begin(), indices.end(), 0 );

      // Shuffle the points so that patterns do not form
      random_shuffle( indices.begin(), indices.end() );

      for( int i = 0; i < 16; ++i )
      {
        int n = indices[i];

        int a = ( n >> 3 ) & 1;
        int b = ( n >> 2 ) & 1;
        int c = ( n >> 1 ) & 1;
        int d = ( n >> 0 ) & 1;

        bool goodShape = true;

        if( y == 0 && a == 0 )
        {
          goodShape = false;
        }

        if( y == numVertical - 1 && b == 0 )
        {
          goodShape = false;
        }

        if( x == 0 && c == 0 )
        {
          goodShape = false;
        }

        if( x == numHorizontal-1 && d == 0 )
        {
          goodShape = false;
        }

        if( y > 0 && (( grid[y-1][x] >> 2 ) & 1) == a )
        {
          goodShape = false;
        }

        if( x > 0 && (( grid[y][x-1] >> 0 ) & 1) == c )
        {
          goodShape = false;
        }

        if( goodShape )
        {
          if( y == 0 || y == numVertical-1 || x == 0 || x == numHorizontal-1 )
          {
            edgeLocations.push_back({ x*mosaicTileWidth-int((float)c*(float)mosaicTileWidth*0.3), y*mosaicTileHeight-int((float)a*(float)mosaicTileHeight*0.3), (x+1)*mosaicTileWidth+int((float)(c+d)*(float)mosaicTileWidth*0.3), (y+1)*mosaicTileHeight+int((float)(a+b)*(float)mosaicTileHeight*0.3), n } );

            edgeLocations2.push_back({ x*imageTileWidth-int((float)c*(float)imageTileWidth*0.3), y*imageTileHeight-int((float)a*(float)imageTileHeight*0.3), (x+1)*imageTileWidth+int((float)(c+d)*(float)imageTileWidth*0.3), (y+1)*imageTileHeight+int((float)(a+b)*(float)imageTileHeight*0.3), n } );
          }
          else
          {
            mosaicLocations.push_back({ x*mosaicTileWidth-int((float)c*(float)mosaicTileWidth*0.3), y*mosaicTileHeight-int((float)a*(float)mosaicTileHeight*0.3), (x+1)*mosaicTileWidth+int((float)(c+d)*(float)mosaicTileWidth*0.3), (y+1)*mosaicTileHeight+int((float)(a+b)*(float)mosaicTileHeight*0.3), n } );

            mosaicLocations2.push_back({ x*imageTileWidth-int((float)c*(float)imageTileWidth*0.3), y*imageTileHeight-int((float)a*(float)imageTileHeight*0.3), (x+1)*imageTileWidth+int((float)(c+d)*(float)imageTileWidth*0.3), (y+1)*imageTileHeight+int((float)(a+b)*(float)imageTileHeight*0.3), n } );
          }

          grid[y][x] = n;

          break;
        }
      }
    }
  }
}

void createMultisizeSquaresLocations( string inputImage, vector< vector< int > > &shapeIndices, vector< vector< int > > &mosaicLocations, vector< vector< int > > &mosaicLocations2, vector< vector< int > > &edgeLocations, vector< vector< int > > &edgeLocations2, vector< vector< float > > &offsets, vector< vector< float > > &dimensions, vector< int > &tileWidth, vector< int > &tileHeight, vector< int > &tileWidth2, vector< int > &tileHeight2, int mosaicTileWidth, int mosaicTileHeight, int imageTileWidth, int imageTileHeight, int width, int height )
{  
  createLocations( mosaicLocations, mosaicLocations2, edgeLocations, edgeLocations2, offsets, dimensions, mosaicTileWidth, mosaicTileHeight, imageTileWidth, imageTileHeight, width, height );

  // Load input image
  VImage image = VImage::vipsload( (char *)inputImage.c_str() ).autorot().colourspace(VIPS_INTERPRETATION_B_W);

  // Resize the image for correct mosaic tile size
  image = image.resize( (float)width / (float)(image.width()) );

  // Get image data
  unsigned char * c = ( unsigned char * )image.data();

  for( int l = 0; l < mosaicLocations.size(); ++l )
  {
    int j = mosaicLocations[l][0];
    int i = mosaicLocations[l][1];
    int t = mosaicLocations[l][4];

    vector< float > d(shapeIndices[t].size());

    for( int x = 0; x < shapeIndices[t].size(); ++x )
    {
      int p = ( ( i + shapeIndices[t][x]/tileWidth[t] ) * image.width() + j + shapeIndices[t][x]%tileWidth[t] );

      d[x] = c[p];
    }

    float avg = 0;

    for( int x = 0; x < d.size(); ++x )
    {
      avg += d[x];
    }

    avg /= d.size();

    float deviation = 0;

    for( int x = 0; x < d.size(); ++x )
    {
      deviation += (d[x] - avg)*(d[x] - avg);
    }

    deviation /= d.size();

    if( deviation > 200 && t < shapeIndices.size() - 1 )
    {
      int x = mosaicLocations[l][2];
      int y = mosaicLocations[l][3];

      mosaicLocations.erase(mosaicLocations.begin() + l);

      mosaicLocations.push_back({ (int)round((float)x-tileWidth[t+1]), (int)round((float)y-tileHeight[t+1]), x, y, t+1 } );
      mosaicLocations.push_back({ (int)round((float)x-tileWidth[t+1]), (int)round((float)y-tileHeight[t]), x, (int)round((float)y-tileHeight[t+1]), t+1 } );
      mosaicLocations.push_back({ (int)round((float)x-tileWidth[t]), (int)round((float)y-tileHeight[t+1]), (int)round((float)x-tileWidth[t+1]), y, t+1 } );
      mosaicLocations.push_back({ (int)round((float)x-tileWidth[t]), (int)round((float)y-tileHeight[t]), (int)round((float)x-tileWidth[t+1]), (int)round((float)y-tileHeight[t+1]), t+1 } );

      x = mosaicLocations2[l][2];
      y = mosaicLocations2[l][3];

      mosaicLocations2.erase(mosaicLocations2.begin() + l);

      mosaicLocations2.push_back({ (int)round((float)x-tileWidth2[t+1]), (int)round((float)y-tileHeight2[t+1]), x, y, t+1 } );
      mosaicLocations2.push_back({ (int)round((float)x-tileWidth2[t+1]), (int)round((float)y-tileHeight2[t]), x, (int)round((float)y-tileHeight2[t+1]), t+1 } );
      mosaicLocations2.push_back({ (int)round((float)x-tileWidth2[t]), (int)round((float)y-tileHeight2[t+1]), (int)round((float)x-tileWidth2[t+1]), y, t+1 } );
      mosaicLocations2.push_back({ (int)round((float)x-tileWidth2[t]), (int)round((float)y-tileHeight2[t]), (int)round((float)x-tileWidth2[t+1]), (int)round((float)y-tileHeight2[t+1]), t+1 } );

      --l;
    }
  }
}

void createMultisizeTrianglesLocations( string inputImage, vector< vector< int > > &shapeIndices, vector< vector< int > > &mosaicLocations, vector< vector< int > > &mosaicLocations2, vector< vector< int > > &edgeLocations, vector< vector< int > > &edgeLocations2, vector< vector< float > > &offsets, vector< vector< float > > &dimensions, vector< int > &tileWidth, vector< int > &tileHeight, vector< int > &tileWidth2, vector< int > &tileHeight2, int mosaicTileWidth, int mosaicTileHeight, int imageTileWidth, int imageTileHeight, int width, int height )
{  
  createLocations( mosaicLocations, mosaicLocations2, edgeLocations, edgeLocations2, offsets, dimensions, mosaicTileWidth, mosaicTileHeight, imageTileWidth, imageTileHeight, width, height );

  // Load input image
  VImage image = VImage::vipsload( (char *)inputImage.c_str() ).autorot().colourspace(VIPS_INTERPRETATION_B_W);

  // Resize the image for correct mosaic tile size
  image = image.resize( (float)width / (float)(image.width()) );

  // Get image data
  unsigned char * c = ( unsigned char * )image.data();

  for( int l = 0; l < mosaicLocations.size(); ++l )
  {
    int j = mosaicLocations[l][0];
    int i = mosaicLocations[l][1];
    int t = mosaicLocations[l][4];

    vector< float > d(shapeIndices[t].size());

    for( int x = 0; x < shapeIndices[t].size(); ++x )
    {
      int p = ( ( i + shapeIndices[t][x]/tileWidth[t] ) * image.width() + j + shapeIndices[t][x]%tileWidth[t] );

      d[x] = c[p];
    }

    float avg = 0;

    for( int x = 0; x < d.size(); ++x )
    {
      avg += d[x];
    }

    avg /= d.size();

    float deviation = 0;

    for( int x = 0; x < d.size(); ++x )
    {
      deviation += (d[x] - avg)*(d[x] - avg);
    }

    deviation /= d.size();

    if( deviation > 200 && t < shapeIndices.size() - 2 )
    {
      if( t%2 == 0 )
      {
        int x = mosaicLocations[l][2];
        int y = mosaicLocations[l][3];

        mosaicLocations.push_back({ (int)round((float)x-tileWidth[t+2]), (int)round((float)y-tileHeight[t+2]), x, y, t+2 } );
        mosaicLocations.push_back({ (int)round((float)x-tileWidth[t]), (int)round((float)y-tileHeight[t+2]), (int)round((float)x-tileWidth[t+2]), y, t+2 } );
        mosaicLocations.push_back({ (int)round((float)x-3*tileWidth[t+2]/2), (int)round((float)y-tileHeight[t]), (int)round((float)x-tileWidth[t+2]/2), (int)round((float)y-tileHeight[t+2]), t+2 } );
        mosaicLocations.push_back({ (int)round((float)x-3*tileWidth[t+2]/2), (int)round((float)y-tileHeight[t+2]), (int)round((float)x-tileWidth[t+2]/2), y, t+3 } );        

        x = mosaicLocations2[l][2];
        y = mosaicLocations2[l][3];

        mosaicLocations2.push_back({ (int)round((float)x-tileWidth2[t+2]), (int)round((float)y-tileHeight2[t+2]), x, y, t+2 } );
        mosaicLocations2.push_back({ (int)round((float)x-tileWidth2[t]), (int)round((float)y-tileHeight2[t+2]), (int)round((float)x-tileWidth2[t+2]), y, t+2 } );
        mosaicLocations2.push_back({ (int)round((float)x-3*tileWidth2[t+2]/2), (int)round((float)y-tileHeight2[t]), (int)round((float)x-tileWidth2[t+2]/2), (int)round((float)y-tileHeight2[t+2]), t+2 } );
        mosaicLocations2.push_back({ (int)round((float)x-3*tileWidth2[t+2]/2), (int)round((float)y-tileHeight2[t+2]), (int)round((float)x-tileWidth2[t+2]/2), y, t+3 } );        
      }
      else
      {
        int x = mosaicLocations[l][2];
        int y = mosaicLocations[l][3];

        mosaicLocations.push_back({ (int)round((float)x-tileWidth[t+2]), (int)round((float)y-tileHeight[t]), x, (int)round((float)y-tileHeight[t+2]), t+2 } );
        mosaicLocations.push_back({ (int)round((float)x-tileWidth[t]), (int)round((float)y-tileHeight[t]), (int)round((float)x-tileWidth[t+2]), (int)round((float)y-tileHeight[t+2]), t+2 } );
        mosaicLocations.push_back({ (int)round((float)x-3*tileWidth[t+2]/2), (int)round((float)y-tileHeight[t+2]), (int)round((float)x-tileWidth[t+2]/2), y, t+2 } );
        mosaicLocations.push_back({ (int)round((float)x-3*tileWidth[t+2]/2), (int)round((float)y-tileHeight[t]), (int)round((float)x-tileWidth[t+2]/2), (int)round((float)y-tileHeight[t+2]), t+1 } );

        x = mosaicLocations2[l][2];
        y = mosaicLocations2[l][3];

        mosaicLocations2.push_back({ (int)round((float)x-tileWidth2[t+2]), (int)round((float)y-tileHeight2[t]), x, (int)round((float)y-tileHeight2[t+2]), t+2 } );
        mosaicLocations2.push_back({ (int)round((float)x-tileWidth2[t]), (int)round((float)y-tileHeight2[t]), (int)round((float)x-tileWidth2[t+2]), (int)round((float)y-tileHeight2[t+2]), t+2 } );
        mosaicLocations2.push_back({ (int)round((float)x-3*tileWidth2[t+2]/2), (int)round((float)y-tileHeight2[t+2]), (int)round((float)x-tileWidth2[t+2]/2), y, t+2 } );
        mosaicLocations2.push_back({ (int)round((float)x-3*tileWidth2[t+2]/2), (int)round((float)y-tileHeight2[t]), (int)round((float)x-tileWidth2[t+2]/2), (int)round((float)y-tileHeight2[t+2]), t+1 } );
      }

      mosaicLocations.erase(mosaicLocations.begin() + l);
      mosaicLocations2.erase(mosaicLocations2.begin() + l);

      --l;
    }
  }
}

void createTetrisLocations( vector< vector< int > > &mosaicLocations, vector< vector< int > > &mosaicLocations2, vector< vector< int > > &edgeLocations, vector< vector< int > > &edgeLocations2, vector< vector< float > > &offsets, vector< vector< float > > &dimensions, int mosaicTileWidth, int mosaicTileHeight, int imageTileWidth, int imageTileHeight, int width, int height, int numHorizontal, int numVertical )
{
  vector< int > starts = 
  {
    0,
    0,
    1,
    0,
    1,
    0,
    0,
    0,
    2,
    0,
    0,
    0,
    1,
    0,
    1,
    0,
    1,
    0,
    0
  };

  vector< vector< vector< int > > > shapes = 
  {
    {
      {1,1},
      {1,1}
    },
    {
      {1,0},
      {1,1},
      {1,0}
    },
    {
      {0,1},
      {1,1},
      {0,1}
    },
    {
      {1,1,1},
      {0,1,0}
    },
    {
      {0,1,0},
      {1,1,1}
    },
    {
      {1,1,1},
      {1,0,0}
    },
    {
      {1,1,1},
      {0,0,1}
    },
    {
      {1,0,0},
      {1,1,1}
    },
    {
      {0,0,1},
      {1,1,1}
    },
    {
      {1,1},
      {1,0},
      {1,0}
    },
    {
      {1,0},
      {1,0},
      {1,1}
    },
    {
      {1,1},
      {0,1},
      {0,1}
    },
    {
      {0,1},
      {0,1},
      {1,1}
    },
    {
      {1,1,0},
      {0,1,1}
    },
    {
      {0,1,1},
      {1,1,0}
    },
    {
      {1,0},
      {1,1},
      {0,1}
    },
    {
      {0,1},
      {1,1},
      {1,0}
    },
    {
      {1,1,1,1}
    },
    {
      {1},
      {1},
      {1},
      {1}
    }
  };

  vector< vector< int > > grid(numVertical, vector< int >(numHorizontal,0));
  vector< vector< int > > grid2(numVertical+2, vector< int >(numHorizontal+2,0));

  vector< vector< int > > chosenShapes;
  chosenShapes.push_back({-1,-1,-1,-1});

  vector< vector< int > > indices(1,vector< int >(shapes.size()));
  iota( indices[0].begin(), indices[0].end(), 0 );

  // Shuffle the points so that patterns do not form
  random_shuffle( indices[0].begin(), indices[0].end() );

  bool gridFull = false;

  //for( int p = 0; p < 500; ++p )
  while(!gridFull)
  {
    if( chosenShapes.back()[0] >= 0 && chosenShapes.back()[1] >= 0 )
    {
      for( int k = 0; k < shapes[chosenShapes.back()[2]].size(); ++k )
      {
        for( int l = 0; l < shapes[chosenShapes.back()[2]][k].size(); ++l )
        {
          if( shapes[chosenShapes.back()[2]][k][l] == 0 ) continue;

          int x = chosenShapes.back()[0] - starts[chosenShapes.back()[2]] + l;
          int y = chosenShapes.back()[1] + k;

          if( x >= 0 && y >= 0 && x < numHorizontal && y < numVertical )
          {
            grid[y][x] = 0;
          }
        }
      }
    }

    int x1 = -1;
    int y1 = -1;

    for( int i = 0; i < numVertical; ++i )
    {
      for( int j = 0; j < numHorizontal; ++j )
      {
        if( grid[i][j] == 0 )
        {
          x1 = j;
          y1 = i;
          break;
        }
      }
      if( x1 >= 0 && y1 >= 0 ) break;
    }

    int bestShape = -1;
    int bestIndex = -1;

    for( int i = chosenShapes.back()[3]+1; i < shapes.size(); ++i )
    {
      bool goodShape = true;

      int shape = indices.back()[i];

      for( int k = 0; k < shapes[shape].size(); ++k )
      {
        for( int l = 0; l < shapes[shape][k].size(); ++l )
        {
          if( shapes[shape][k][l] == 0 ) continue;

          int x = x1 - starts[shape] + l;
          int y = y1 + k;

          if( x >= 0 && y >= 0 && x < numHorizontal && y < numVertical && grid[y][x] == 1 )
          {
            goodShape = false;
          }
        }
      }

      if( goodShape )
      {
        bestIndex = i;
        bestShape = shape;
        break;
      }
    }

    bool hasHole = false;

    // Check if hole exists in grid

    for( int i = 0; i < numVertical; ++i )
    {
      for( int j = 0; j < numHorizontal; ++j )
      {
        grid2[i+1][j+1]=0;

        if( grid[i][j]==1) continue;

        grid2[i+1][j+1] = (i==0 || grid[i-1][j]==0) + (i==numVertical-1 || grid[i+1][j]==0) + (j==0||grid[i][j-1]==0) + (j==numHorizontal-1 ||grid[i][j+1]==0);

        if(i==numVertical-1) grid2[i+1][j+1] += 100;
      }
    }

    for( int i = 1; i < numVertical+1; ++i )
    {
      for( int j = 1; j < numHorizontal+1; ++j )
      {
        if( grid[i-1][j-1]==1) continue;

        if(grid2[i-1][j] + grid2[i+1][j] + grid2[i][j-1] + grid2[i][j+1]<4) hasHole = true;
      }
    }

    if(bestShape >= 0 && !hasHole)
    {
      chosenShapes.back()[0] = x1;
      chosenShapes.back()[1] = y1;
      chosenShapes.back()[2] = bestShape;
      chosenShapes.back()[3] = bestIndex;

      for( int k = 0; k < shapes[bestShape].size(); ++k )
      {
        for( int l = 0; l < shapes[bestShape][k].size(); ++l )
        {
          if( shapes[bestShape][k][l] == 0 ) continue;

          int x = x1 - starts[bestShape] + l;
          int y = y1 + k;

          if( x >= 0 && y >= 0 && x < numHorizontal && y < numVertical )
          {
            grid[y][x] = 1;
          }
        }
      }

      chosenShapes.push_back({-1,-1,-1,-1});

      indices.push_back(vector< int >(shapes.size()));
      iota( indices.back().begin(), indices.back().end(), 0 );

      // Shuffle the points so that patterns do not form
      random_shuffle( indices.back().begin(), indices.back().end() );
    }
    else
    {
      chosenShapes.pop_back();
      indices.pop_back();
    }

    gridFull = true;

    for( int k = 0; k < numVertical; ++k )
    {
      for( int l = 0; l < numHorizontal; ++l )
      {
        if( grid[k][l] == 0 ) gridFull = false;
      }
    }
    
  }

  for( int i = 0; i < chosenShapes.size(); ++i )
  {
    int x1 = chosenShapes[i][0];
    int y1 = chosenShapes[i][1];
    
    if( y1 < 0 ) continue;

    int shape = chosenShapes[i][2];

    x1 -= starts[shape];

    int x2 = x1+shapes[shape][0].size();
    int y2 = y1+shapes[shape].size();

    if( x1 < 0 || x2 >= numHorizontal || y2 >= numVertical )
    {
      edgeLocations.push_back({ (int)round(x1*mosaicTileWidth), (int)round(y1*mosaicTileHeight), (int)round(x2*mosaicTileWidth), (int)round(y2*mosaicTileHeight), shape } );
      edgeLocations2.push_back({ (int)round(x1*imageTileWidth), (int)round(y1*imageTileHeight), (int)round(x2*imageTileWidth), (int)round(y2*imageTileHeight), shape } );            
    }
    else
    {
      mosaicLocations.push_back({ (int)round(x1*mosaicTileWidth), (int)round(y1*mosaicTileHeight), (int)round(x2*mosaicTileWidth), (int)round(y2*mosaicTileHeight), shape } );
      mosaicLocations2.push_back({ (int)round(x1*imageTileWidth), (int)round(y1*imageTileHeight), (int)round(x2*imageTileWidth), (int)round(y2*imageTileHeight), shape } );            
    }
  }
}