#include "mosaicLocations.h"

void createLocations( vector< vector< int > > &mosaicLocations, vector< vector< int > > &mosaicLocations2, vector< vector< int > > &edgeLocations, vector< vector< int > > &edgeLocations2, vector< vector< double > > &offsets, vector< vector< double > > &dimensions, int mosaicTileWidth, int mosaicTileHeight, int imageTileWidth, int imageTileHeight, int width, int height )
{
  for( int i = 0; i < offsets.size(); ++i )
  {
    double w = (double) mosaicTileWidth;
    double h = (double) mosaicTileHeight;

    double xHorizontalOffset = w * offsets[i][2];
    double yHorizontalOffset = h * offsets[i][3];

    double xVerticalOffset = w * offsets[i][4];
    double yVerticalOffset = h * offsets[i][5];

    double w2 = (double) imageTileWidth;
    double h2 = (double) imageTileHeight;

    double xHorizontalOffset2 = w2 * offsets[i][2];
    double yHorizontalOffset2 = h2 * offsets[i][3];

    double xVerticalOffset2 = w2 * offsets[i][4];
    double yVerticalOffset2 = h2 * offsets[i][5];

    double tileWidth = w * dimensions[offsets[i][6]][0];
    double tileHeight = h * dimensions[offsets[i][6]][1];

    double tileWidth2 = w2 * dimensions[offsets[i][6]][0];
    double tileHeight2 = h2 * dimensions[offsets[i][6]][1];

    for( double ySign = -1; ySign <= 1; ySign += 2 )
    {
      for( double xSign = -1; xSign <= 1; xSign += 2 )
      {
        double startX = w * offsets[i][0] + tileWidth;
        double startY = h * offsets[i][1] + tileHeight;

        double startX2 = w2 * offsets[i][0] + tileWidth2;
        double startY2 = h2 * offsets[i][1] + tileHeight2;

        for( double y = startY, y2 = startY2; (ySign > 0 && startY < 3*height/2) || (ySign < 0 && startY > -3*height/2); startY += ySign*yVerticalOffset, startY2 += ySign*yVerticalOffset2, startX += ySign*xVerticalOffset, startX2 += ySign*xVerticalOffset2)// y += yOffset, y2 += yOffset2 )
        {
          for( double x = startX, x2 = startX2, y = startY, y2 = startY2; (xSign > 0 && x < 3*width/2) || (xSign < 0 && x > -3*width/2); x += xSign*xHorizontalOffset, x2 += xSign*xHorizontalOffset2, y += xSign*yHorizontalOffset, y2 += xSign*yHorizontalOffset2 )
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

void createTetrisLocations( vector< vector< int > > &mosaicLocations, vector< vector< int > > &mosaicLocations2, vector< vector< int > > &edgeLocations, vector< vector< int > > &edgeLocations2, vector< vector< double > > &offsets, vector< vector< double > > &dimensions, int mosaicTileWidth, int mosaicTileHeight, int imageTileWidth, int imageTileHeight, int width, int height, int numHorizontal, int numVertical )
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

  indices[0][0]=1;

  bool gridFull = false;

  //for( int p = 0; p < 500; ++p )
  while(!gridFull)
  {
    if( chosenShapes.back()[0] >= 0 && chosenShapes.back()[1] >= 0 )
    {
//        cout << shapes[chosenShapes.back()[2]].size() << endl;
//        cout << shapes[chosenShapes.back()[2]][0].size() << endl;

      for( int k = 0; k < shapes[chosenShapes.back()[2]].size(); ++k )
      {
        for( int l = 0; l < shapes[chosenShapes.back()[2]][k].size(); ++l )
        {
          if( shapes[chosenShapes.back()[2]][k][l] == 0 ) continue;

          int x = chosenShapes.back()[0] - starts[chosenShapes.back()[2]] + l;
          int y = chosenShapes.back()[1] + k;

//            cout << x << " " << y << endl;

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

//    cout << x1 << " " << y1 << " " << chosenShapes.size() << endl;

    for( int i = chosenShapes.back()[3]+1; i < shapes.size(); ++i )
    {
      bool goodShape = true;

//      cout << "i " << i << " " << indices.back()[i]

      int shape = indices.back()[i];

      for( int k = 0; k < shapes[shape].size(); ++k )
      {
        for( int l = 0; l < shapes[shape][k].size(); ++l )
        {
          if( shapes[shape][k][l] == 0 ) continue;

          int x = x1 - starts[shape] + l;
          int y = y1 + k;

//          cout << x << " " << y << " " << shape << endl;

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

//    cout << x1 << " " << y1 << " " << bestShape << " " << bestIndex << endl;

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

//          cout << "grid " << x << " " << y << endl;

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
//      cout << "chosen " << chosenShapes.back()[0] << " " << chosenShapes.back()[1] << " " << chosenShapes.back()[2] << endl;
/*
      if( chosenShapes.back()[0] >= 0 && chosenShapes.back()[1] >= 0 )
      {
//        cout << shapes[chosenShapes.back()[2]].size() << endl;
//        cout << shapes[chosenShapes.back()[2]][0].size() << endl;

        for( int k = 0; k < shapes[chosenShapes.back()[2]].size(); ++k )
        {
          for( int l = 0; l < shapes[chosenShapes.back()[2]][k].size(); ++l )
          {
            if( shapes[chosenShapes.back()[2]][k][l] == 0 ) continue;

            int x = chosenShapes.back()[0] - starts[chosenShapes.back()[2]] + l;
            int y = chosenShapes.back()[1] + k;

//            cout << x << " " << y << endl;

            if( x >= 0 && y >= 0 && x < numHorizontal && y < numVertical )
            {
              grid[y][x] = 0;
            }
          }
        }
      }
      */

      chosenShapes.pop_back();
      indices.pop_back();
    }

    gridFull = true;

    for( int k = 0; k < numVertical; ++k )
    {
      for( int l = 0; l < numHorizontal; ++l )
      {
        if( grid[k][l] == 0 ) gridFull = false;
        //cout << grid[k][l] << " ";
      }
//      cout << endl;
    }
  //  cout << endl;
    
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

//    cout << x1 << " " << y1 << " " << shape << endl;

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

//    cout << mosaicLocations[i][0] << " " << mosaicLocations[i][1] << " " << mosaicLocations[i][2] << " " << mosaicLocations[i][3] << endl;
  }
}