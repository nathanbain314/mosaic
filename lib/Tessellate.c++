#include "Tessellate.h"

void buildImage( vector< vector< vector< unsigned char > > > &imageData, vector< int > &mosaic, vector< vector< int > > mosaicLocations, vector< vector< int > > shapeIndices, string outputImage, vector< int > tileWidth, int width, int height )
{
  cout << "Generating image " << outputImage << endl;

  // Create output image data 
  unsigned char *data = new unsigned char[width*height*3];

  for( int k = 0; k < mosaicLocations.size(); ++k )
  {
    int j = mosaicLocations[k][0];
    int i = mosaicLocations[k][1];
    int t = mosaicLocations[k][4];
//    cout << j << " " << i << " " << t << " " << tileWidth[t] << " " << width << " " << height << endl;
    
    for( int x = 0; x < shapeIndices[t].size(); ++x )
    {
      int l = 3 * ( ( i + shapeIndices[t][x]/tileWidth[t] ) * width + j + shapeIndices[t][x]%tileWidth[t] );

      //data[l+0] = ((i/tileWidth[t])%2)*t*255;//imageData[t][mosaic[k]][3*shapeIndices[t][x]+0];
      //data[l+1] = t*255;//imageData[t][mosaic[k]][3*shapeIndices[t][x]+1];
      //data[l+2] = 255;//imageData[t][mosaic[k]][3*shapeIndices[t][x]+2];

      data[l+0] = imageData[t][mosaic[k]][3*shapeIndices[t][x]+0];
      data[l+1] = imageData[t][mosaic[k]][3*shapeIndices[t][x]+1];
      data[l+2] = imageData[t][mosaic[k]][3*shapeIndices[t][x]+2];
    }
  }

  VImage::new_from_memory( data, width*height*3, width, height, 3, VIPS_FORMAT_UCHAR ).vipssave((char *)outputImage.c_str());

  // Free the data
  delete [] data;
}

int gcd(int a, int b) {
    return b == 0 ? a : gcd(b, a % b);
}

int generateRGBBlock( vector< unique_ptr< my_kd_tree_t > > &mat_index, vector< int > &mosaic, vector< vector< int > > &mosaicLocations, vector< vector< int > > shapeIndices, vector< int > &indices, int repeat, unsigned char * c, int blockX, int blockY, int blockWidth, int blockHeight, vector< int > tileWidth, int width, ProgressBar* buildingMosaic, bool quiet )
{
  // Whether to update progressbar
  bool show = !quiet && !(blockX+blockY);

  // Color difference of images
  int out_dist_sqr;

  // For every index
  for( int p = 0; p < indices.size(); ++p )
  {
    int k = indices[p];
    int j = mosaicLocations[k][0];
    int i = mosaicLocations[k][1];
    int t = mosaicLocations[k][4];

    // Update progressbar if necessary
    if(show) buildingMosaic->Increment();

    vector< int > d(shapeIndices[t].size()*3);

    // Get rgb data about tile and save in vector
    for( int x = 0; x < shapeIndices[t].size(); ++x )
    {
      int l = 3 * ( ( i + shapeIndices[t][x]/tileWidth[t] ) * width + j + shapeIndices[t][x]%tileWidth[t] );

      d[3*x+0] = c[l];
      d[3*x+1] = c[l+1];
      d[3*x+2] = c[l+2];
    }

    size_t ret_index;

    mat_index[t]->query( &d[0], 1, &ret_index, &out_dist_sqr );

    // Set mosaic tile data
    mosaic[k] = ret_index;
  }

  return 0;
}

int generateMosaic( vector< unique_ptr< my_kd_tree_t > > &mat_index, vector< vector< int > > &mosaicLocations, vector< vector< int > > shapeIndices, vector< int > &mosaic, string inputImage, ProgressBar *buildingMosaic, int repeat, bool square, int resize, bool quiet, vector< int > tileWidth )
{
  // Whether to show progressbar
  quiet = quiet || ( buildingMosaic == NULL );

  // Load input image
  VImage image = VImage::vipsload( (char *)inputImage.c_str() );

  // Convert to a three band image
  if( image.bands() == 1 )
  {
    image = image.bandjoin(image).bandjoin(image);
  }
  if( image.bands() == 4 )
  {
    image = image.flatten();
  }

  // Crop out the middle square if specified
  if(square)
  {
    image = (image.width() < image.height()) ? image.extract_area(0, (image.height()-image.width())/2, image.width(), image.width()) :
                                               image.extract_area((image.width()-image.height())/2, 0, image.height(), image.height());
  }

  int width = image.width();
  int height = image.height();

  // Get image data
  unsigned char * c = ( unsigned char * )image.data();

  // Number of threads for multithreading: needs to be a square number
  int sqrtThreads = 1;//ceil(sqrt(sysconf(_SC_NPROCESSORS_ONLN)));
  int threads = sqrtThreads*sqrtThreads;

  // Totat number of tiles in block
  //int total = ceil((double)numVertical/(double)sqrtThreads)*ceil((double)numHorizontal/(double)sqrtThreads);

  // Create list of numbers in thread block
  vector< int > indices( mosaicLocations.size() );
  iota( indices.begin(), indices.end(), 0 );

  // Shuffle the points so that patterns do not form
  shuffle( indices.begin(), indices.end(), default_random_engine(time(NULL)) );

  // Break mosaic into blocks and find best matching tiles inside each block on a different thread
  future< int > ret[threads];

  for( int i = 0, k = 0; i < sqrtThreads; ++i )
  {
    for( int j = 0; j < sqrtThreads; ++j, ++k )
    {
      ret[k] = async( launch::async, &generateRGBBlock, ref(mat_index), ref(mosaic), ref(mosaicLocations), ref(shapeIndices), ref(indices), repeat, c, 0,0,0,0,/*j*numHorizontal/sqrtThreads, i*numVertical/sqrtThreads, (j+1)*numHorizontal/sqrtThreads-j*numHorizontal/sqrtThreads, (i+1)*numVertical/sqrtThreads-i*numVertical/sqrtThreads,*/ tileWidth, width, buildingMosaic, quiet );
    }
  }

  // Wait for threads to finish
  for( int k = 0; k < threads; ++k )
  {
    ret[k].get();
  }

  return 00;
}

void createLocations( vector< vector< int > > &mosaicLocations, vector< vector< int > > &mosaicLocations2, int mosaicTileWidth, int mosaicTileHeight, int imageTileWidth, int imageTileHeight, int width, int height, int tessellateType )
{
  switch( tessellateType )
  {
    case 0:
    {
      double w = (double) mosaicTileWidth;
      double h = (double) mosaicTileHeight;

      double hw = w / 2.0;

      double s = 2.0*h/(3.0*w);

      for( int i = mosaicTileHeight, i2 = imageTileHeight; i <= height; i += 4*mosaicTileHeight/3, i2 += 4*imageTileHeight/3 )
      {
        for( int j = mosaicTileWidth, j2 = imageTileWidth; j <= width; j += mosaicTileWidth, j2 += imageTileWidth )
        {
          mosaicLocations.push_back({ j-mosaicTileWidth, i-mosaicTileHeight, j, i, 0 } );
          mosaicLocations2.push_back({ j2-imageTileWidth, i2-imageTileHeight, j2, i2, 0 } );
        }
      }

      for( int i = 5*mosaicTileHeight/3, i2 = 5*imageTileHeight/3; i <= height; i += 4*mosaicTileHeight/3, i2 += 4*imageTileHeight/3 )
      {
        for( int j = 3*mosaicTileWidth/2, j2 = 3*imageTileWidth/2; j <= width; j += mosaicTileWidth, j2 += imageTileHeight )
        {
          mosaicLocations.push_back({ j-mosaicTileWidth, i-mosaicTileHeight, j, i, 0 } );
          mosaicLocations2.push_back({ j2-imageTileWidth, i2-imageTileHeight, j2, i2, 0 } );
        }
      }

      break;
    }

    case 1:
    {
      double w = (double) mosaicTileWidth;
      double h = (double) mosaicTileHeight;

      double hw = w / 2.0;

      double s = h/hw;

      for( int i = mosaicTileHeight, i2 = imageTileHeight; i <= height; i += 4*mosaicTileHeight, i2 += 4*imageTileHeight )
      {
        for( int j = mosaicTileWidth, j2 = imageTileWidth; j <= width; j += mosaicTileWidth, j2 += imageTileWidth )
        {
          mosaicLocations.push_back({ j-mosaicTileWidth, i-mosaicTileHeight, j, i, 0 } );
          mosaicLocations2.push_back({ j2-imageTileWidth, i2-imageTileHeight, j2, i2, 0 } );
        }
      }

      for( int i = mosaicTileHeight, i2 = imageTileHeight; i <= height; i += 4*mosaicTileHeight, i2 += 4*imageTileHeight )
      {
        for( int j = 3*mosaicTileWidth/2, j2 = 3*imageTileWidth/2; j <= width; j += mosaicTileWidth, j2 += imageTileWidth )
        {
          mosaicLocations.push_back({ j-mosaicTileWidth, i-mosaicTileHeight, j, i, 1 } );
          mosaicLocations2.push_back({ j2-imageTileWidth, i2-imageTileHeight, j2, i2, 1 } );
        }
      }

      for( int i = 3*mosaicTileHeight, i2 = 3*imageTileHeight; i <= height; i += 4*mosaicTileHeight, i2 += 4*imageTileHeight )
      {
        for( int j = 3*mosaicTileWidth/2, j2 = 3*imageTileWidth/2; j <= width; j += mosaicTileWidth, j2 += imageTileWidth )
        {
          mosaicLocations.push_back({ j-mosaicTileWidth, i-mosaicTileHeight, j, i, 0 } );
          mosaicLocations2.push_back({ j2-imageTileWidth, i2-imageTileHeight, j2, i2, 0 } );
        }
      }

      for( int i = 3*mosaicTileHeight, i2 = 3*imageTileHeight; i <= height; i += 4*mosaicTileHeight, i2 += 4*imageTileHeight )
      {
        for( int j = mosaicTileWidth, j2 = imageTileWidth; j <= width; j += mosaicTileWidth, j2 += imageTileWidth )
        {
          mosaicLocations.push_back({ j-mosaicTileWidth, i-mosaicTileHeight, j, i, 1 } );
          mosaicLocations2.push_back({ j2-imageTileWidth, i2-imageTileHeight, j2, i2, 1 } );
        }
      }

      for( int i = 2*mosaicTileHeight, i2 = 2*imageTileHeight; i <= height; i += 4*mosaicTileHeight, i2 += 4*imageTileHeight )
      {
        for( int j = mosaicTileWidth, j2 = imageTileWidth; j <= width; j += mosaicTileWidth, j2 += imageTileWidth )
        {
          mosaicLocations.push_back({ j-mosaicTileWidth, i-mosaicTileHeight, j, i, 2 } );
          mosaicLocations2.push_back({ j2-imageTileWidth, i2-imageTileHeight, j2, i2, 2 } );
        }
      }

      for( int i = 4*mosaicTileHeight, i2 = 4*imageTileHeight; i <= height; i += 4*mosaicTileHeight, i2 += 4*imageTileHeight )
      {
        for( int j = 3*mosaicTileWidth/2, j2 = 3*imageTileWidth/2; j <= width; j += mosaicTileWidth, j2 += imageTileWidth )
        {
          mosaicLocations.push_back({ j-mosaicTileWidth, i-mosaicTileHeight, j, i, 2 } );
          mosaicLocations2.push_back({ j2-imageTileWidth, i2-imageTileHeight, j2, i2, 2 } );
        }
      }

      break;
    }

    case 2:
    {
      double w = (double) mosaicTileWidth;// * (sqrt(2.0)+1.0);
      double h = (double) mosaicTileHeight;// * (sqrt(2.0)+1.0);

      double w1 = floor(w / sqrt(2.0));
      double w2 = floor(w * (1.0/sqrt(2)+1.0));
      double h1 = floor(h / sqrt(2.0));
      double h2 = floor(h * (1.0/sqrt(2)+1.0));

      double wOffset = floor(w * (sqrt(2.0)+2.0));
      double hOffset = floor(h * (sqrt(2.0)+2.0));

      w = floor(w * (sqrt(2.0)+1.0));
      h = floor(h * (sqrt(2.0)+1.0));

      double w_2 = (double) imageTileWidth;// * (sqrt(2.0)+1.0);
      double h_2 = (double) imageTileHeight;// * (sqrt(2.0)+1.0);

      double w12 = floor(w_2 / sqrt(2.0));
      double w22 = floor(w_2 * (1.0/sqrt(2)+1.0));
      double h12 = floor(h_2 / sqrt(2.0));
      double h22 = floor(h_2 * (1.0/sqrt(2)+1.0));

      double wOffset2 = floor(w_2 * (sqrt(2.0)+2.0));
      double hOffset2 = floor(h_2 * (sqrt(2.0)+2.0));

      w_2 = floor(w_2 * (sqrt(2.0)+1.0));
      h_2 = floor(h_2 * (sqrt(2.0)+1.0));


      double s = h/w;

      for( int i = h2, i2 = h22; i <= height; i += hOffset, i2 += hOffset2 )
      {
        for( int j = wOffset, j2 = wOffset2; j <= width; j += wOffset, j2 += wOffset2 )
        {
          mosaicLocations.push_back({ j-mosaicTileWidth, i-mosaicTileHeight, j, i, 0 } );
          mosaicLocations2.push_back({ j2-imageTileWidth, i2-imageTileHeight, j2, i2, 0 } );
        }
      }

      for( int i = hOffset, i2 = hOffset2; i <= height; i += hOffset, i2 += hOffset2 )
      {
        for( int j = w2, j2 = w22; j <= width; j += wOffset, j2 += wOffset2 )
        {
          mosaicLocations.push_back({ j-mosaicTileWidth, i-mosaicTileHeight, j, i, 0 } );
          mosaicLocations2.push_back({ j2-imageTileWidth, i2-imageTileHeight, j2, i2, 0 } );
        }
      }

      for( int i = h, i2 = h_2; i <= height; i += hOffset, i2 += hOffset2 )
      {
        for( int j = w, j2 = w_2; j <= width; j += wOffset, j2 += wOffset2 )
        {
          mosaicLocations.push_back({ int((double)j-w), int((double)i-h), j, i, 1 } );
          mosaicLocations2.push_back({ int((double)j2-w_2), int((double)i2-h_2), j2, i2, 1 } );
        }
      }

      for( int i = h+h2, i2 = h_2+h22; i <= height; i += hOffset, i2 += hOffset2 )
      {
        for( int j = w+w2, j2 = w_2+w22; j <= width; j += wOffset, j2 += wOffset2 )
        {
          mosaicLocations.push_back({ int((double)j-w), int((double)i-h), j, i, 1 } );
          mosaicLocations2.push_back({ int((double)j2-w_2), int((double)i2-h_2), j2, i2, 1 } );
        }
      }

      break;
    }
    case 3:
    {
      for( int i = mosaicTileHeight, i2 = imageTileHeight; i <= height; i += 3*mosaicTileHeight/2, i2 += 3*imageTileHeight/2 )
      {
        for( int j = mosaicTileWidth, j2 = imageTileWidth; j <= width; j += mosaicTileWidth, j2 += imageTileWidth )
        {
          mosaicLocations.push_back({ j-mosaicTileWidth, i-mosaicTileHeight, j, i, 0 } );
          mosaicLocations2.push_back({ j2-imageTileWidth, i2-imageTileHeight, j2, i2, 0 } );
        }
      }

      for( int i = 7*mosaicTileHeight/4, i2 = 7*imageTileHeight/4; i <= height; i += 3*mosaicTileHeight/2, i2 += 3*imageTileHeight/2 )
      {
        for( int j = 3*mosaicTileWidth/2, j2 = 3*imageTileWidth/2; j <= width; j += mosaicTileWidth, j2 += imageTileWidth )
        {
          mosaicLocations.push_back({ j-mosaicTileWidth, i-mosaicTileHeight, j, i, 0 } );
          mosaicLocations2.push_back({ j2-imageTileWidth, i2-imageTileHeight, j2, i2, 0 } );
        }
      }

      break;
    }
  }
}

void createShapeIndices( vector< vector< int > > &shapeIndices, int mosaicTileWidth, int mosaicTileHeight, int width, int height, int tessellateType )
{
  switch( tessellateType )
  {
    case 0:
    {
      shapeIndices.resize(1);

      double w = (double) mosaicTileWidth;
      double h = (double) mosaicTileHeight;

      double hw = w / 2.0;

      double s = 2.0*h/(3.0*w);

      for( double y = 0, p = 0; y < mosaicTileHeight; ++y )
      {
        for( double x = 0; x < mosaicTileWidth; ++x, ++p )
        {
          if( y >= s * ( x - hw ) && y >= -s * ( x - hw ) && y - h <= s * ( x - hw ) && y - h <= -s * ( x - hw ) )
          {
            shapeIndices[0].push_back(p);
          }
        }
      }

      break;
    }

    case 1:
    {
      shapeIndices.resize(3);

      double w = (double) mosaicTileWidth;
      double h = (double) mosaicTileHeight;

      double hw = w / 2.0;

      double s = h/hw;

      for( double y = 0, p = 0; y < mosaicTileHeight; ++y )
      {
        for( double x = 0; x < mosaicTileWidth; ++x, ++p )
        {
          if( y >= s * ( x - hw ) && y >= -s * ( x - hw ) )
          {
            shapeIndices[0].push_back(p);
          }
        }
      }

      for( double y = 0, p = 0; y < mosaicTileHeight; ++y )
      {
        for( double x = 0; x < mosaicTileWidth; ++x, ++p )
        {
          if( y - h <= s * ( x - hw ) && y - h <= -s * ( x - hw ) )
          {
            shapeIndices[1].push_back(p);
          }
        }
      }

      for( double y = 0, p = 0; y < mosaicTileHeight; ++y )
      {
        for( double x = 0; x < mosaicTileWidth; ++x, ++p )
        {
          shapeIndices[2].push_back(p);
        }
      }

      break;
    }
    case 2:
    {
      shapeIndices.resize(2);

      double w = (double) mosaicTileWidth;// * (sqrt(2.0)+1.0);
      double h = (double) mosaicTileHeight;// * (sqrt(2.0)+1.0);

      for( double y = 0, p = 0; y < h; ++y )
      {
        for( double x = 0; x < w; ++x, ++p )
        {
          shapeIndices[0].push_back(p);
        }
      }

      double w1 = floor(w / sqrt(2.0));
      double w2 = floor(w * (1.0/sqrt(2)+1.0));
      double h1 = floor(h / sqrt(2.0));
      double h2 = floor(h * (1.0/sqrt(2)+1.0));

      double wOffset = floor(w * (sqrt(2.0)+2.0));
      double hOffset = floor(h * (sqrt(2.0)+2.0));

      w = floor(w * (sqrt(2.0)+1.0));
      h = floor(h * (sqrt(2.0)+1.0));

      double s = h/w;

      for( double y = 0, p = 0; y < h; ++y )
      {
        for( double x = 0; x < w; ++x, ++p )
        {
          if( y >= -s * ( x - w1 ) && y >= s * ( x - w2 ) && y - h <= s * ( x - w1 ) && y - h <= -s * ( x - w2 ) )
          {
            shapeIndices[1].push_back(p);
          }
        }
      }
      
      break;
    }
    case 3:
    {
      shapeIndices.resize(1);

      double w = (double) mosaicTileWidth;// * (sqrt(2.0)+1.0);
      double h = (double) mosaicTileHeight;// * (sqrt(2.0)+1.0);

      double pi = 3.14159265358979;

      for( double y = 0, p = 0; y < h; ++y )
      {
        for( double x = 0; x < w; ++x, ++p )
        {
          if( y >= h*(0.125*(cos(2.0*pi*x/w)+1.0)) && y <= h*(-0.125*(cos(2.0*pi*x/w)+1.0)+1.0) )
          {
            shapeIndices[0].push_back(p);
          }
        }
      }      
      break;
    }
  }
}

void RunTessellate( string inputName, string outputName, vector< string > inputDirectory, int numHorizontal, bool trueColor, int cropStyle, bool flip, bool spin, int mosaicTileWidth, int mosaicTileHeight, int imageTileWidth, int repeat, string fileName, bool quiet, bool recursiveSearch, int tessellateType )
{
  bool inputIsDirectory = (vips_foreign_find_save( inputName.c_str() ) == NULL);
  bool isDeepZoom = (vips_foreign_find_save( outputName.c_str() ) == NULL) && !inputIsDirectory;

  vector< string > inputImages;
  vector< string > outputImages;

  if( inputIsDirectory )
  {
    if( inputName.back() != '/' ) inputName += '/';
    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir (inputName.c_str())) != NULL) 
    {
      while ((ent = readdir (dir)) != NULL)
      {   
        if( ent->d_name[0] != '.' && vips_foreign_find_load( string( inputName + ent->d_name ).c_str() ) != NULL )
        {
          inputImages.push_back( inputName + ent->d_name );
          outputImages.push_back( outputName + ent->d_name );
        }
      }
    }
  }
  else
  {
    inputImages.push_back( inputName );
    outputImages.push_back( outputName );
  }

  int width = VImage::new_memory().vipsload( (char *)inputImages[0].c_str() ).width();
  int height = VImage::new_memory().vipsload( (char *)inputImages[0].c_str() ).height();

  if( mosaicTileWidth == 0 ) mosaicTileWidth = width/numHorizontal;
  if( imageTileWidth == 0 || isDeepZoom ) imageTileWidth = mosaicTileWidth;
  if( mosaicTileHeight == 0 ) mosaicTileHeight = mosaicTileWidth;

  int imageTileHeight = imageTileWidth*mosaicTileHeight/mosaicTileWidth;

  int numImages;
  vector< cropType > cropData;
  vector< vector< vector< unsigned char > > > mosaicTileData;
  vector< vector< vector< unsigned char > > > imageTileData;

  bool loadData = (fileName != " ");

  vector< vector< double > > ratios = {
      {1.0},
      {1.0,1.0,1.0},
      {1.0,2.41421356237},
      {1.0}
    };

  vector< int > tileWidth, tileWidth2;

  for( int i = 0; i < ratios[tessellateType].size(); ++i )
  {
    tileWidth.push_back((double)mosaicTileWidth*ratios[tessellateType][i]);
    tileWidth2.push_back((double)imageTileWidth*ratios[tessellateType][i]);
  }

  if( !loadData )
  {
    mosaicTileData.resize(ratios[tessellateType].size());

    if( mosaicTileWidth != imageTileWidth ) imageTileData.resize(ratios[tessellateType].size());

    vector< string > inputDirectoryBlank;

    for( int i = 0; i < inputDirectory.size(); ++i )
    {
      string imageDirectory = inputDirectory[i];
      if( imageDirectory.back() != '/' ) imageDirectory += '/';
      for( int j = 0; j < ratios[tessellateType].size(); ++j )
      {
        generateThumbnails( cropData, mosaicTileData[j], imageTileData[j], j == 0 ? inputDirectory : inputDirectoryBlank, imageDirectory, (double)mosaicTileWidth*ratios[tessellateType][j], (double)mosaicTileHeight*ratios[tessellateType][j], (double)imageTileWidth*ratios[tessellateType][j], (double)imageTileHeight*ratios[tessellateType][j], isDeepZoom, spin, cropStyle, flip, quiet, recursiveSearch );
      }
    }

    numImages = mosaicTileData[0].size();

    if( numImages == 0 ) 
    {
      cout << "No valid images found. The directory might not contain any images, or they might be too small, or they might not be a valid format." << endl;
      return;
    }
  }

  vector< vector< int > > shapeIndices, shapeIndices2, mosaicLocations, mosaicLocations2;

  createShapeIndices( shapeIndices, mosaicTileWidth, mosaicTileHeight, width, height, tessellateType );
  createLocations( mosaicLocations, mosaicLocations2, mosaicTileWidth, mosaicTileHeight, imageTileWidth, imageTileHeight, width, height, tessellateType );

  if( mosaicTileWidth != imageTileWidth )
  {
    createShapeIndices( shapeIndices2, imageTileWidth, imageTileHeight, width*imageTileWidth/mosaicTileWidth, height*imageTileWidth/mosaicTileWidth, tessellateType );
  }

  int imageWidth = 0, imageHeight = 0;

  for( int i = 0; i < mosaicLocations2.size(); ++i )
  {
    imageWidth = max(imageWidth,mosaicLocations2[i][2]);
    imageHeight = max(imageHeight,mosaicLocations2[i][3]);
  }

  int tileArea = mosaicTileWidth*mosaicTileHeight*3;
  int numVertical = int( (double)height / (double)width * (double)numHorizontal * (double)mosaicTileWidth/(double)mosaicTileHeight );
  int numUnique = 0;

  vector< vector< vector< int > > > d;
  vector< vector< float > > lab;

  if( trueColor  )
  {
  }
  else
  {
    d.resize(shapeIndices.size());

    for( int i = 0; i < shapeIndices.size(); ++i )
    {
      tileArea = shapeIndices[i].size();

      d[i] = vector< vector< int > >( mosaicTileData[i].size(), vector< int >(3*tileArea) );

      for( int j = 0; j < mosaicTileData[i].size(); ++j )
      {
        for( int p = 0; p < tileArea; ++p )
        {
          d[i][j][3*p+0] = mosaicTileData[i][j][3*shapeIndices[i][p]+0];
          d[i][j][3*p+1] = mosaicTileData[i][j][3*shapeIndices[i][p]+1];
          d[i][j][3*p+2] = mosaicTileData[i][j][3*shapeIndices[i][p]+2];
        }
      }
    }
  }

  if( inputIsDirectory )
  {
    g_mkdir(outputName.c_str(), 0777);
  }

  vector< unique_ptr< my_kd_tree_t > > mat_index;//(tileArea, d, 10 );

  for( int i = 0; i < shapeIndices.size(); ++i )
  {
    unique_ptr< my_kd_tree_t > ptr(new my_kd_tree_t(3*shapeIndices[i].size(),d[i],10));
    mat_index.push_back( move(ptr) );//3*shapeIndices[i].size(),d[i],10);
  }

  for( int i = 0; i < inputImages.size(); ++i )
  {
    vector< int > mosaic( mosaicLocations.size() );

    int sqrtThreads = 1;//ceil(sqrt(sysconf(_SC_NPROCESSORS_ONLN)));

    ProgressBar *buildingMosaic = new ProgressBar(mosaic.size(), "Building mosaic");

    if( trueColor  )
    {
    }
    else
    {
      numUnique = generateMosaic( mat_index, mosaicLocations, shapeIndices, mosaic, inputImages[i], buildingMosaic, repeat, false, numHorizontal * mosaicTileWidth, quiet, tileWidth );
    }

    if( !quiet ) buildingMosaic->Finish();

    if( isDeepZoom )
    {
    }
    else
    {
      if( mosaicTileWidth == imageTileWidth )
      {
        buildImage( mosaicTileData, mosaic, mosaicLocations, shapeIndices, outputImages[i], tileWidth, width, height );
      }
      else
      {
        buildImage( imageTileData, mosaic, mosaicLocations2, shapeIndices2, outputImages[i], tileWidth2, imageWidth, imageHeight );
      }
    }
  }
}