#include "mosaicTools.h"

void generateThumbnails( vector< cropType > &cropData, vector< vector< unsigned char > > &mosaicTileData, vector< vector< unsigned char > > &imageTileData, vector< string > &inputDirectory, string imageDirectory, int mosaicTileWidth, int mosaicTileHeight, int imageTileWidth, int imageTileHeight,  bool exclude, bool spin, int cropStyle, bool flip, bool quiet, bool recursiveSearch )
{
  // Used for reading directory
  DIR *dir;
  struct dirent *ent;
  string str;

  // Used for processing image
  int size, xOffset, yOffset, width, height, mosaicTileArea = mosaicTileWidth*mosaicTileHeight*3, imageTileArea = imageTileWidth*imageTileHeight*3;
  bool different = mosaicTileWidth != imageTileWidth && !exclude;

  unsigned char *c1, *c2;

  // Minimum size allowed: must be at least as large as mosaicTileWidth and imageTileWidth, and 256 if exclude is set
  int minWidth = max(mosaicTileWidth,imageTileWidth);
  int minHeight = max(mosaicTileHeight,imageTileHeight);

  int num_images = 0;

  vector< string > names;

  cout << "Reading directory " << imageDirectory << endl;

  // Count the number of valid image files in the directory
  if ((dir = opendir (imageDirectory.c_str())) != NULL) 
  {
    while ((ent = readdir (dir)) != NULL)
    {
      if( ent->d_name[0] != '.' && vips_foreign_find_load( string( imageDirectory + ent->d_name ).c_str() ) != NULL )
      {
        names.push_back( imageDirectory + ent->d_name );
        cout << "\rFound " << ++num_images << " images " << flush;
      }
      else if( recursiveSearch && ent->d_name[0] != '.' && ent->d_type == DT_DIR )
      {
        inputDirectory.push_back( imageDirectory + ent->d_name );
      }
    }
  }

  cout << endl;

  ProgressBar *processing_images = new ProgressBar(num_images, "Processing images");

  VipsAngle rotAngle[4] = {VIPS_ANGLE_D0,VIPS_ANGLE_D90,VIPS_ANGLE_D180,VIPS_ANGLE_D270};

  // Iterate through all images in directory
  for( int i = 0; i < num_images; ++i )
  {
    if( !quiet ) processing_images->Increment();
    try
    {
      str = names[i];

      for( int r = 0; r < (spin ? 4:1); ++r )
      {
        VImage initImage = VImage::vipsload((char *)str.c_str()).rot(rotAngle[r]);

        // Get the width, height, and smallest and largest sizes
        width = initImage.width();// VImage::new_memory().vipsload( (char *)str.c_str() ).width();
        height = initImage.height();// VImage::new_memory().vipsload( (char *)str.c_str() ).height();
        double widthRatio = (double)width/(double)mosaicTileWidth;
        double heightRatio = (double)height/(double)mosaicTileHeight;
        double minRatio = min(widthRatio,heightRatio);
        double maxRatio = max(widthRatio,heightRatio);
        // Whether the a square is offset vertically or not
        bool vertical = widthRatio < heightRatio;

        if( width < minWidth || height < minHeight )
        {
          continue;
        }

        VImage image2, image;

        // Shrink the image and crop based on cropping style
        switch( cropStyle )
        {
          // Crop the middle square
          case 0:
            image = initImage.thumbnail_image(mosaicTileWidth,VImage::option()->set( "height", mosaicTileHeight )->set( "crop", VIPS_INTERESTING_CENTRE )->set( "size", VIPS_SIZE_DOWN ));
            break;
          // Crop the square with the most entropy
          case 1:
            image = initImage.thumbnail_image(mosaicTileWidth,VImage::option()->set( "height", mosaicTileHeight )->set( "crop", VIPS_INTERESTING_ENTROPY )->set( "size", VIPS_SIZE_DOWN ));
            break;
          // Crop the square most likely to 
          case 2:
            image = initImage.thumbnail_image(mosaicTileWidth,VImage::option()->set( "height", mosaicTileHeight )->set( "crop", VIPS_INTERESTING_ATTENTION )->set( "size", VIPS_SIZE_DOWN ));
            break;
          // Shrink the image to be cropped later
          case 3:
            int w2 = width/minRatio;
            int h2 = height/minRatio;

            image = initImage.thumbnail_image(w2,VImage::option()->set( "size", VIPS_SIZE_DOWN )->set( "crop", VIPS_INTERESTING_CENTRE )->set( "height", h2 )); 
            break;
        }

        // Offset of crop
        xOffset = min(-(int)image.xoffset(),(int)ceil(maxRatio/minRatio*(double)mosaicTileWidth) - mosaicTileWidth);
        yOffset = min(-(int)image.yoffset(),(int)ceil(maxRatio/minRatio*(double)mosaicTileHeight) - mosaicTileHeight);

        // Convert image to 3 band image
        if( image.bands() == 1 )
        {
          image = image.bandjoin(image).bandjoin(image);
        }
        else if( image.bands() == 4 )
        {
          image = image.flatten();
        }

        // If the mosaicTileWidth and imageTileWidth are different then shrink and crop again
        if( different )
        {
          switch( cropStyle )
          {
            // Crop the middle square
            case 0:
              image2 = initImage.thumbnail_image(imageTileWidth,VImage::option()->set( "height", imageTileHeight )->set( "crop", VIPS_INTERESTING_CENTRE )->set( "size", VIPS_SIZE_DOWN ));
              break;
            // Crop the square with the most entropy
            case 1:
              image2 = initImage.thumbnail_image(imageTileWidth,VImage::option()->set( "height", imageTileHeight )->set( "crop", VIPS_INTERESTING_ENTROPY )->set( "size", VIPS_SIZE_DOWN ));
              break;
            // Crop the square most likely to 
            case 2:
              image2 = initImage.thumbnail_image(imageTileWidth,VImage::option()->set( "height", imageTileHeight )->set( "crop", VIPS_INTERESTING_ATTENTION )->set( "size", VIPS_SIZE_DOWN ));
              break;
            // Shrink the image to be cropped later
            case 3:
              int w2 = (width*imageTileWidth)/(mosaicTileWidth*minRatio);
              int h2 = (height*imageTileWidth)/(mosaicTileWidth*minRatio);

              image2 = initImage.thumbnail_image(w2,VImage::option()->set( "size", VIPS_SIZE_DOWN )->set( "crop", VIPS_INTERESTING_CENTRE )->set( "height", h2 ));
              break;
          }

          if( image2.bands() == 1 )
          {
            image2 = image2.bandjoin(image2).bandjoin(image2);
          }
          else if( image2.bands() == 4 )
          {
            image2 = image2.flatten();
          }
        }

        // If cropStyle is 3 then crop every possible crop
        if( cropStyle == 3 )
        {
          // For every possible crop
          for( int ii = vertical ? floor(maxRatio/minRatio*(double)mosaicTileHeight) - mosaicTileHeight : floor(maxRatio/minRatio*(double)mosaicTileWidth) - mosaicTileWidth; ii >= 0; --ii )
          {
            VImage newImage, newImage2;

            // Crop it correctly if it is a vertical or horizontal image
            newImage = image.copy().extract_area((!vertical)*ii,vertical*ii,mosaicTileWidth,mosaicTileHeight);
            if(different) newImage2 = image2.copy().extract_area((!vertical)*ii*imageTileWidth/mosaicTileWidth,vertical*ii*imageTileHeight/mosaicTileHeight,imageTileWidth,imageTileHeight);

            // Mirror the image if flip is set
            for( int ff = flip; ff >= 0; --ff)
            {
              if(flip) newImage = newImage.flip(VIPS_DIRECTION_HORIZONTAL);

              // Get the image data
              c1 = (unsigned char *)newImage.data();

              // Push the data onto the vector
              mosaicTileData.push_back( vector< unsigned char >(c1, c1 + mosaicTileArea) );

              if( different )
              {
                if(flip) newImage2 = newImage2.flip(VIPS_DIRECTION_HORIZONTAL);
                c2 = (unsigned char *)newImage2.data();
                imageTileData.push_back( vector< unsigned char >(c2, c2 + imageTileArea) );
              }

              // Save the data detailing the image name, crop offset, mirror status, and rotation
              cropData.push_back( make_tuple(str,(!vertical)*minRatio*(double)ii,vertical*minRatio*(double)ii,r,ff) );
            }
          }
        }
        // Otherwise just save the image
        else
        {
          // Mirror the image if flip is set
          for( int ff = flip; ff >= 0; --ff)
          {
            if(flip)
            {
              image = image.flip(VIPS_DIRECTION_HORIZONTAL);
              if( different ) image2 = image2.flip(VIPS_DIRECTION_HORIZONTAL);
            }

            // Get the image data
            c1 = (unsigned char *)image.data();

            // Push the data onto the vector
            mosaicTileData.push_back( vector< unsigned char >(c1, c1 + mosaicTileArea) );

            if( different )
            {
              c2 = (unsigned char *)image2.data();
              imageTileData.push_back( vector< unsigned char >(c2, c2 + imageTileArea) );
            }

            cropData.push_back( make_tuple(str,xOffset*minRatio,yOffset*minRatio,r,ff) );
          }
        }
      }
    }
    catch (...)
    {
    }
  }

  if( !quiet ) processing_images->Finish();
}

int generateLABBlock( vector< vector< float > > &imageData, vector< vector< int > > &mosaic, vector< int > &indices, vector< bool > &used, int repeat, unsigned char * c, int blockX, int blockY, int blockWidth, int blockHeight, int tileWidth, int tileHeight, int width, int numHorizontal, int numVertical, ProgressBar* buildingMosaic, bool quiet )
{
  // Whether to update progressbar
  bool show = !quiet && !(blockX+blockY);

  // Vector lab tile data
  vector< float > d(tileWidth*tileHeight*3);

  int num_images = imageData.size();

  // For every index
  for( int p = 0; p < indices.size(); ++p )
  {
    // Get position of tile
    int i = indices[p] / blockWidth;
    int j = indices[p] % blockWidth;
    
    // Skip if outside of the array
    if( i >= blockHeight || j >= blockWidth ) continue;
    
    // Update progressbar if necessary
    if(show) buildingMosaic->Increment();

    // Add offset position
    i += blockY;
    j += blockX;

    // Start and end of tiles to check for repeating images
    int xEnd = ( (j+repeat+1<(int)mosaic[0].size()) ? j+repeat+1 : mosaic[0].size() );
    int yEnd = ( (i+repeat+1<(int)mosaic.size()) ? i+repeat+1 : mosaic.size() );

    // Extract rgb data of tile from image and convert to lab data
    for( int y = 0, n = 0; y < tileHeight; ++y )
    {
      for( int x = 0; x < tileWidth; ++x, n+=3 )
      {
        // Index of pixel
        int l = ( ( i * tileHeight + y ) * width + j * tileWidth + x ) * 3;
        
        float r,g,b;
        // Convert rgb to rgb16
        vips_col_sRGB2scRGB_8(c[l],c[l+1],c[l+2], &r,&g,&b );
        // Convert rgb16 to xyz
        vips_col_scRGB2XYZ( r, g, b, &r, &g, &b );
        // Convert xyz to lab
        vips_col_XYZ2Lab( r, g, b, &d[n], &d[n+1], &d[n+2] );
      }
    }

    int best = 0;
    double difference = DBL_MAX;

    // Iterate through every image and find the most similar one
    for( int k = 0; k < num_images; ++k )
    {
      // Check if image already exists within repeat blocks of tile
      if( repeat > 0 )
      {
        bool valid = true;

        // Break early if match found
        for( int y = (i-repeat > 0) ? i-repeat : 0; valid && y < yEnd; ++y )
        {
          for( int x = (j-repeat > 0) ? j-repeat : 0; valid && x < xEnd; ++x )
          {
            // Skip if is current tile
            if(  y == i && x == j ) continue;
            if( mosaic[y][x] == k )
            {
              valid = false;
            }
          }
        }

        // Skip current image if it was found
        if( !valid ) continue;
      }

      // Compute sum of color differences
      double sum = 0;
      for( int l = 0; l < tileWidth*tileHeight*3; l+=3 )
      {
        sum += vips_col_dE00( imageData[k][l], imageData[k][l+1], imageData[k][l+2], d[l], d[l+1], d[l+2] );
      }

      // Update for best color difference
      if( sum < difference )
      {
        difference = sum;
        best = k;
      }
    }

    // Set mosaic tile data
    mosaic[i][j] = best;

    // Set the best image as being used
    used[ best ] = true;
  }

  return 0;
}

// Takes a vector of image thumbnails, input image and number of images per row and creates output image
int generateMosaic( vector< vector< float > > &imageData, vector< vector< int > > &mosaic, string inputImage, ProgressBar *buildingMosaic, int repeat, bool square, int resize, bool quiet )
{
  // Whether to show progressbar
  quiet = quiet || ( buildingMosaic == NULL );

  // Load input image
  VImage image = VImage::new_memory().vipsload( (char *)inputImage.c_str() );//.colourspace( VIPS_INTERPRETATION_LAB );

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

  // Resize the image for correct mosaic tile size
  if( resize != 0 ) image = image.resize( (double)resize / (double)(image.width()) );

  int width = image.width();
  int height = image.height();

  // Get image data
  unsigned char * c = ( unsigned char * )image.data();

  // Get number of horizontal and vertical tiles
  int numHorizontal = mosaic[0].size();
  int numVertical = mosaic.size();

  // Data about tile size
  int tileWidth = width / numHorizontal;
  int tileHeight = height / numVertical;
  int tileArea = tileWidth*tileHeight*3;

  // Number of images to check against
  int num_images = imageData.size();

  // Whether an image was used or not
  vector< bool > used( num_images, false );

  // Number of threads for multithreading: needs to be a square number
  int sqrtThreads = ceil(sqrt(sysconf(_SC_NPROCESSORS_ONLN)));
  int threads = sqrtThreads*sqrtThreads;

  // Totat number of tiles in block
  int total = ceil((double)numVertical/(double)sqrtThreads)*ceil((double)numHorizontal/(double)sqrtThreads);

  // Create list of numbers in thread block
  vector< int > indices( total );
  iota( indices.begin(), indices.end(), 0 );

  // Shuffle the points so that patterens do not form
  if( repeat > 0 ) shuffle( indices.begin(), indices.end(), default_random_engine(time(NULL)) );

  // Break mosaic into blocks and find best matching tiles inside each block on a different thread
  future< int > ret[threads];

  for( int i = 0, k = 0; i < sqrtThreads; ++i )
  {
    for( int j = 0; j < sqrtThreads; ++j, ++k )
    {
      ret[k] = async( launch::async, &generateLABBlock, ref(imageData), ref(mosaic), ref(indices), ref(used), repeat, c, j*numHorizontal/sqrtThreads, i*numVertical/sqrtThreads, (j+1)*numHorizontal/sqrtThreads-j*numHorizontal/sqrtThreads, (i+1)*numVertical/sqrtThreads-i*numVertical/sqrtThreads, tileWidth, tileHeight, width, numHorizontal, numVertical, buildingMosaic, quiet );
    }
  }

  // Wait for threads to finish
  for( int k = 0; k < threads; ++k )
  {
    ret[k].get();
  }

  // Get total number of different images used
  return accumulate(used.begin(), used.end(), 0);
}

int generateRGBBlock( my_kd_tree_t &mat_index, vector< vector< int > > &mosaic, vector< int > &indices, vector< bool > &used, int repeat, unsigned char * c, int blockX, int blockY, int blockWidth, int blockHeight, int tileWidth, int tileHeight, int width, int numHorizontal, int numVertical, ProgressBar* buildingMosaic, bool quiet )
{
  // Whether to update progressbar
  bool show = !quiet && !(blockX+blockY);

  // Color difference of images
  int out_dist_sqr;

  // Vector rgb tile data
  vector< int > d(tileWidth*tileHeight*3);

  typedef KDTreeSingleIndexDynamicAdaptor< L2_Simple_Adaptor<int, my_kd_tree_t>, my_kd_tree_t> dynamic_kdtree;

  int num_images = mat_index.kdtree_get_point_count();

  dynamic_kdtree index(mat_index.m_data[0].size(), mat_index, KDTreeSingleIndexAdaptorParams(10) );

  // For every index
  for( int p = 0; p < indices.size(); ++p )
  {
    // Get position of tile
    int i = indices[p] / blockWidth;
    int j = indices[p] % blockWidth;
    
    // Skip if outside of the array
    if( i >= blockHeight || j >= blockWidth ) continue;
    
    // Update progressbar if necessary
    if(show) buildingMosaic->Increment();

    // Add offset position
    i += blockY;
    j += blockX;

    // Get rgb data about tile and save in vector
    for( int y = 0, n = 0; y < tileHeight; ++y )
    {
      for( int x = 0; x < tileWidth; ++x, n+=3 )
      {
        int l = ( ( i * tileHeight + y ) * width + j * tileWidth + x ) * 3;
        d[n] = c[l];
        d[n+1] = c[l+1];
        d[n+2] = c[l+2];
      }
    }

    size_t ret_index;

    if( repeat > 0 )
    {
      int imagesLeft = num_images;

      // Outside of square so that closer images are not the same
      for( int r = 1; r <= repeat; ++r )
      {
        for( int p1 = -r; imagesLeft > 1 && p1 <= r; ++p1 )
        {
          if( j+p1 >= 0 && j+p1 < numHorizontal && i-r >= 0 && mosaic[i-r][j+p1] >= 0 )
          {
            index.removePoint(mosaic[i-r][j+p1]);
            --imagesLeft;
          }
        }

        for( int p1 = -r; imagesLeft > 1 && p1 <= r; ++p1 )
        {
          if( j+p1 >= 0 && j+p1 < numHorizontal && i+r < numVertical && mosaic[i+r][j+p1] >= 0 )
          {
            index.removePoint(mosaic[i+r][j+p1]);
            --imagesLeft;
          }
        }

        for( int p1 = -r+1; imagesLeft > 1 && p1 < r; ++p1 )
        {
          if( i+p1 >= 0 && i+p1 < numVertical && j-r >= 0 && mosaic[i+p1][j-r] >= 0 )
          {
            index.removePoint(mosaic[i+p1][j-r]);
            --imagesLeft;
          }
        }

        for( int p1 = -r+1; imagesLeft > 1 && p1 < r; ++p1 )
        {
          if( i+p1 >= 0 && i+p1 < numVertical && j+r < numHorizontal && mosaic[i+p1][j+r] >= 0 )
          {
            index.removePoint(mosaic[i+p1][j+r]);
            --imagesLeft;
          }
        }
      }

      nanoflann::KNNResultSet<int> resultSet(1);
      resultSet.init(&ret_index, &out_dist_sqr );
      index.findNeighbors(resultSet, &d[0], nanoflann::SearchParams(10));

      index.addPointsBack();
    }
    else
    {
      mat_index.query( &d[0], 1, &ret_index, &out_dist_sqr );
    }

    // Set mosaic tile data
    mosaic[i][j] = ret_index;

    // Set the best image as being used
    used[ ret_index ] = true;
  }

  return 0;
}

int generateMosaic( my_kd_tree_t &mat_index, vector< vector< int > > &mosaic, string inputImage, ProgressBar *buildingMosaic, int repeat, bool square, int resize, bool quiet )
{
  // Whether to show progressbar
  quiet = quiet || ( buildingMosaic == NULL );

  // Number of images to check against
  int num_images = mat_index.kdtree_get_point_count();

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

  // Resize the image for correct mosaic tile size
  if( resize != 0 ) image = image.resize( (double)resize / (double)(image.width()) );

  int width = image.width();
  int height = image.height();

  // Get image data
  unsigned char * c = ( unsigned char * )image.data();

  // Get number of horizontal and vertical tiles
  int numHorizontal = mosaic[0].size();
  int numVertical = mosaic.size();

  // Data about tile size
  int tileWidth = width / numHorizontal;
  int tileHeight = height / numVertical;
  int tileArea = tileWidth*tileHeight*3;

  // Whether an image was used or not
  vector< bool > used( num_images, false );

  // Number of threads for multithreading: needs to be a square number
  int sqrtThreads = ceil(sqrt(sysconf(_SC_NPROCESSORS_ONLN)));
  int threads = sqrtThreads*sqrtThreads;

  // Totat number of tiles in block
  int total = ceil((double)numVertical/(double)sqrtThreads)*ceil((double)numHorizontal/(double)sqrtThreads);

  // Create list of numbers in thread block
  vector< int > indices( total );
  iota( indices.begin(), indices.end(), 0 );

  // Shuffle the points so that patterens do not form
  if( repeat > 0 ) shuffle( indices.begin(), indices.end(), default_random_engine(time(NULL)) );

  // Break mosaic into blocks and find best matching tiles inside each block on a different thread
  future< int > ret[threads];

  for( int i = 0, k = 0; i < sqrtThreads; ++i )
  {
    for( int j = 0; j < sqrtThreads; ++j, ++k )
    {
      ret[k] = async( launch::async, &generateRGBBlock, ref(mat_index), ref(mosaic), ref(indices), ref(used), repeat, c, j*numHorizontal/sqrtThreads, i*numVertical/sqrtThreads, (j+1)*numHorizontal/sqrtThreads-j*numHorizontal/sqrtThreads, (i+1)*numVertical/sqrtThreads-i*numVertical/sqrtThreads, tileWidth, tileHeight, width, numHorizontal, numVertical, buildingMosaic, quiet );
    }
  }

  // Wait for threads to finish
  for( int k = 0; k < threads; ++k )
  {
    ret[k].get();
  }

  // Get total number of different images used
  return accumulate(used.begin(), used.end(), 0);
}