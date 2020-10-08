#include "mosaicTools.h"

int numberOfCPUS()
{
  int numCPU;

#ifdef WIN32
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  numCPU = sysinfo.dwNumberOfProcessors;
#else
  numCPU = sysconf(_SC_NPROCESSORS_ONLN);
#endif

  return numCPU;
}

void fit(float &c, int type)
{
  if( type == 0 )
  {
    c = max(c,0.0f);
    c = min(c,100.0f);
  }
  else
  {
    c = max(c,-128.0f);
    c = min(c,127.0f);
  }
}

void fit2(float &c)
{
  c = max(c,0.0f);
  c = min(c,255.0f);
}

void rgbToLab( int r, int g, int b, float &l1, float &a1, float &b1 )
{
  vips_col_sRGB2scRGB_8( r, g, b, &l1,&a1,&b1 );
  vips_col_scRGB2XYZ( l1, a1, b1, &l1, &a1, &b1 );
  vips_col_XYZ2Lab( l1, a1, b1, &l1, &a1, &b1 );
}

void changeColorspace( Tree &tree, Point &center, unsigned char * c, float * c2, int start, int end, int width, float gamma, bool gammutMapping, ProgressBar * changingColorspace, bool quiet )
{
  bool show = !quiet && !(start);

  for( int height = start, index = start*width; height < end; ++height )
  {
    for( int w = 0; w < width; ++w, ++index )
    {
      for( int k = 0; k < 3; ++k )
      {
        int n10 = c[3*index+k];
        
        float v = (float)n10;

        v /= 255.0f;

        v = pow( v, gamma );
        c2[3*index+k] = 255.0f * v;
      }

      rgbToLab( c2[3*index+0], c2[3*index+1], c2[3*index+2], c2[3*index+0], c2[3*index+1], c2[3*index+2] );      

      if(gammutMapping)
      {
        Point point_query(c2[3*index+0], c2[3*index+1], c2[3*index+2]);

        Segment segment_query(center,point_query);

        if(tree.do_intersect(segment_query))
        {
          Point closest_point = tree.closest_point(point_query);

          for( int k = 0; k < 3; ++k )
          {
            c2[3*index+k] = closest_point[k];
          }
        }
      }
    }

    if( show ) changingColorspace->Increment();
  }
}

void processImages( vector< cropType > &cropData, vector< vector< unsigned char > > &mosaicTileData, vector< vector< unsigned char > > &imageTileData, vector< string > &names, int start, int end, int mosaicTileWidth, int mosaicTileHeight, int imageTileWidth, int imageTileHeight, int minWidth, int minHeight, bool spin, bool different, int cropStyle, bool flip, bool quiet, ProgressBar *processing_images )
{
  string str;

  int size, xOffset, yOffset, width, height, mosaicTileArea = mosaicTileWidth*mosaicTileHeight*3, imageTileArea = imageTileWidth*imageTileHeight*3;

  unsigned char *c1, *c2;

  VipsAngle rotAngle[4] = {VIPS_ANGLE_D0,VIPS_ANGLE_D90,VIPS_ANGLE_D180,VIPS_ANGLE_D270};

  // Iterate through all images in directory
  for( int i = start; i < end; ++i )
  {
    if( !quiet && !start ) processing_images->Increment();
    try
    {
      str = names[i];

      for( int r = 0; r < (spin ? 2:1); ++r )
      {
        VImage initImage = VImage::vipsload((char *)str.c_str()).autorot().rot(rotAngle[r]);

        // Get the width, height, and smallest and largest sizes
        width = initImage.width();// VImage::new_memory().vipsload( (char *)str.c_str() ).width();
        height = initImage.height();// VImage::new_memory().vipsload( (char *)str.c_str() ).height();
        float widthRatio = (float)width/(float)mosaicTileWidth;
        float heightRatio = (float)height/(float)mosaicTileHeight;
        float minRatio = min(widthRatio,heightRatio);
        float maxRatio = max(widthRatio,heightRatio);
        // Whether the a square is offset vertically or not
        bool vertical = widthRatio < heightRatio;

        if( width < minWidth || height < minHeight )
        {
          continue;
        }

        VImage image2, image;

        int rw = r%2?mosaicTileHeight:mosaicTileWidth;
        int rh = r%2?mosaicTileWidth:mosaicTileHeight;

        // Shrink the image and crop based on cropping style
        switch( cropStyle )
        {
          // Crop the middle square
          case 0:
            image = VImage::thumbnail((char *)str.c_str(),rw,VImage::option()->set( "height", rh )->set( "crop", VIPS_INTERESTING_CENTRE )->set( "size", VIPS_SIZE_DOWN )->set( "auto_rotate", true ));
            break;
          // Crop the square with the most entropy
          case 1:
            image = VImage::thumbnail((char *)str.c_str(),rw,VImage::option()->set( "height", rh )->set( "crop", VIPS_INTERESTING_ENTROPY )->set( "size", VIPS_SIZE_DOWN )->set( "auto_rotate", true )).rot(rotAngle[r]);
            break;
          // Crop the square most likely to 
          case 2:
            image = VImage::thumbnail((char *)str.c_str(),rw,VImage::option()->set( "height", rh )->set( "crop", VIPS_INTERESTING_ATTENTION )->set( "size", VIPS_SIZE_DOWN )->set( "auto_rotate", true )).rot(rotAngle[r]);
            break;
          // Shrink the image to be cropped later
          case 3:
            int w2 = (r%2?height:width)/minRatio;
            int h2 = (r%2?width:height)/minRatio;

            image = VImage::thumbnail((char *)str.c_str(),w2,VImage::option()->set( "height", h2 )->set( "crop", VIPS_INTERESTING_CENTRE )->set( "size", VIPS_SIZE_DOWN )->set( "auto_rotate", true ));
            break;
        }

        // Offset of crop
        xOffset = min(-(int)image.xoffset(),(int)ceil(maxRatio/minRatio*(float)mosaicTileWidth) - mosaicTileWidth);
        yOffset = min(-(int)image.yoffset(),(int)ceil(maxRatio/minRatio*(float)mosaicTileHeight) - mosaicTileHeight);

        image = image.rot(rotAngle[r]);

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
          rw = r%2?imageTileHeight:imageTileWidth;
          rh = r%2?imageTileWidth:imageTileHeight;

          switch( cropStyle )
          {
            // Crop the middle square
            case 0:
              image2 = VImage::thumbnail((char *)str.c_str(),rw,VImage::option()->set( "height", rh )->set( "crop", VIPS_INTERESTING_CENTRE )->set( "size", VIPS_SIZE_DOWN )->set( "auto_rotate", true )).rot(rotAngle[r]);
              break;
            // Crop the square with the most entropy
            case 1:
              image2 = VImage::thumbnail((char *)str.c_str(),rw,VImage::option()->set( "height", rh )->set( "crop", VIPS_INTERESTING_ENTROPY )->set( "size", VIPS_SIZE_DOWN )->set( "auto_rotate", true )).rot(rotAngle[r]);
              break;
            // Crop the square most likely to 
            case 2:
              image2 = VImage::thumbnail((char *)str.c_str(),rw,VImage::option()->set( "height", rh )->set( "crop", VIPS_INTERESTING_ATTENTION )->set( "size", VIPS_SIZE_DOWN )->set( "auto_rotate", true )).rot(rotAngle[r]);
              break;
            // Shrink the image to be cropped later
            case 3:
              int w2 = ((r%2?height:width)*imageTileWidth)/(mosaicTileWidth*minRatio);
              int h2 = ((r%2?width:height)*imageTileWidth)/(mosaicTileWidth*minRatio);

              image2 = VImage::thumbnail((char *)str.c_str(),w2,VImage::option()->set( "height", h2 )->set( "crop", VIPS_INTERESTING_CENTRE )->set( "size", VIPS_SIZE_DOWN )->set( "auto_rotate", true )).rot(rotAngle[r]);
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
          for( int ii = vertical ? floor(maxRatio/minRatio*(float)mosaicTileHeight) - mosaicTileHeight : floor(maxRatio/minRatio*(float)mosaicTileWidth) - mosaicTileWidth; ii >= 0; --ii )
          {
            VImage newImage, newImage2;

            // Crop it correctly if it is a vertical or horizontal image
            newImage = image.copy().extract_area((!vertical)*ii,vertical*ii,mosaicTileWidth,mosaicTileHeight);
            if(different) newImage2 = image2.copy().extract_area((!vertical)*ii*imageTileWidth/mosaicTileWidth,vertical*ii*imageTileHeight/mosaicTileHeight,imageTileWidth,imageTileHeight);

            // Mirror the image if flip is set
            for( int ff = flip; ff >= 0; --ff)
            {
              if(flip)
              {
                newImage = newImage.flip(VIPS_DIRECTION_HORIZONTAL);
                if(different) newImage2 = newImage2.flip(VIPS_DIRECTION_HORIZONTAL);
              }

              for( int rr = 0; rr < (spin ? 3:1); rr += 2 )
              {
                // Get the image data
                c1 = (unsigned char *)newImage.rot(rotAngle[rr]).data();

                // Push the data onto the vector
                mosaicTileData.push_back( vector< unsigned char >(c1, c1 + mosaicTileArea) );

                if( different )
                {
                  c2 = (unsigned char *)newImage2.rot(rotAngle[rr]).data();
                  imageTileData.push_back( vector< unsigned char >(c2, c2 + imageTileArea) );
                }

                // Save the data detailing the image name, crop offset, mirror status, and rotation
//                cropData.push_back( make_tuple(str,(1-r)*(!vertical)*minRatio*(float)ii+r*vertical*minRatio*(float)ii,r*(!vertical)*minRatio*(float)ii+(1-r)*vertical*minRatio*(float)ii,r+rr,ff) );
                cropData.push_back( make_tuple(str,(!vertical)*minRatio*(float)ii,vertical*minRatio*(float)ii,r+rr,ff) );
              }
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

            for( int rr = 0; rr < (spin ? 3:1); rr += 2 )
            {
              // Get the image data
              c1 = (unsigned char *)image.rot(rotAngle[rr]).data();

              // Push the data onto the vector
              mosaicTileData.push_back( vector< unsigned char >(c1, c1 + mosaicTileArea) );

              if( different )
              {
                c2 = (unsigned char *)image2.rot(rotAngle[rr]).data();
                imageTileData.push_back( vector< unsigned char >(c2, c2 + imageTileArea) );
              }

              cropData.push_back( make_tuple(str,(1-r)*xOffset*minRatio+r*yOffset*minRatio,r*xOffset*minRatio+(1-r)*yOffset*minRatio,r+rr,ff) );
            }
          }
        }
      }
    }
    catch (...)
    {
    }
  }
}

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

  if( exclude )
  {
    imageTileWidth = mosaicTileWidth < mosaicTileHeight ? 256*mosaicTileWidth/mosaicTileHeight : 256;
    imageTileHeight = mosaicTileWidth > mosaicTileHeight ? 256*mosaicTileHeight/mosaicTileWidth : 256;
  }

  // Minimum size allowed: must be at least as large as mosaicTileWidth and imageTileWidth, and 256 if exclude is set
  int minWidth = max(mosaicTileWidth,imageTileWidth);
  int minHeight = max(mosaicTileHeight,imageTileHeight);

  int num_images = 0;

  vector< string > names;

  struct stat s;

  if( !quiet ) cout << "Reading directory " << imageDirectory << " for tilesize " << mosaicTileWidth << "x" << mosaicTileHeight << endl;

  // Count the number of valid image files in the directory
  if ((dir = opendir (imageDirectory.c_str())) != NULL) 
  {
    while ((ent = readdir (dir)) != NULL)
    {
      if( ent->d_name[0] != '.' && vips_foreign_find_load( string( imageDirectory + ent->d_name ).c_str() ) != NULL )
      {
        names.push_back( imageDirectory + ent->d_name );
        ++num_images;
        if( !quiet ) cout << "\rFound " << num_images << " images " << flush;
      }
      else if( recursiveSearch && ent->d_name[0] != '.' )
      {
        string newName = imageDirectory + ent->d_name;
        stat((char *)newName.c_str(), &s);
        if (s.st_mode && S_IFDIR)
        {
          inputDirectory.push_back( newName );
        }
      }
    }
  }

  if( !quiet ) cout << endl;

  int threads = numberOfCPUS();

  ProgressBar *processing_images = new ProgressBar(ceil((float)num_images/threads), "Processing images");

  vector< cropType > cropDataThread[threads];
  vector< vector< unsigned char > > mosaicTileDataThread[threads];
  vector< vector< unsigned char > > imageTileDataThread[threads];

  future< void > ret[threads];

  for( int k = 0; k < threads; ++k )
  {
    ret[k] = async( launch::async, &processImages, ref(cropDataThread[k]), ref(mosaicTileDataThread[k]), ref(imageTileDataThread[k]), ref(names), k*num_images/threads, (k+1)*num_images/threads, mosaicTileWidth, mosaicTileHeight, imageTileWidth, imageTileHeight, minWidth, minHeight, spin, different, cropStyle, flip, quiet, processing_images );
  }

  // Wait for threads to finish
  for( int k = 0; k < threads; ++k )
  {
    ret[k].get();

    cropData.insert(cropData.end(), cropDataThread[k].begin(), cropDataThread[k].end());
    mosaicTileData.insert(mosaicTileData.end(), mosaicTileDataThread[k].begin(), mosaicTileDataThread[k].end());
    if( mosaicTileWidth != imageTileWidth ) imageTileData.insert(imageTileData.end(), imageTileDataThread[k].begin(), imageTileDataThread[k].end());
  }

  if( !quiet ) processing_images->Finish();
}

int generateMosaicThread( my_kd_tree_t *mat_index, vector< vector< int > > &mosaic, vector< int > &indices, vector< bool > &used, int repeat, float * c, condition_variable *cv, int threadIdx, int numThreads, float * edgeData, int tileWidth, int tileHeight, int width, int numHorizontal, int numVertical, bool dither, ProgressBar* buildingMosaic, bool quiet )
{
  mutex m;
  unique_lock<mutex> lk(m);

  // Whether to update progressbar
  bool show = !quiet && !(threadIdx);

  // Color difference of images
  float out_dist_sqr;

  // Vector rgb tile data
  vector< float > d(tileWidth*tileHeight*6);

  typedef KDTreeSingleIndexDynamicAdaptor< L2_Simple_Adaptor<float, my_kd_tree_t>, my_kd_tree_t> dynamic_kdtree;

  int num_images = mat_index->kdtree_get_point_count();

  int tileArea = tileWidth*tileHeight*3;

  unique_ptr< dynamic_kdtree > index;

  if( repeat > 0 )
  {
    index = unique_ptr< dynamic_kdtree >( new dynamic_kdtree(mat_index->dimensions, *mat_index, KDTreeSingleIndexAdaptorParams(10) ) );
  }

  // For every index
  for( int p = 0, end = indices.size(); p < end; ++p )
  {
    // Get position of tile
    int i = indices[p] / numHorizontal;
    int j = indices[p] % numHorizontal;

    while( dither && i > 0 && j < numHorizontal-1 && mosaic[i-1][j+1] < 0 )
    {
      cv[threadIdx].wait(lk);
    }

    // Update progressbar if necessary
    if(show) buildingMosaic->Increment();

    // Get rgb data about tile and save in vector
    for( int y = 0, n = 0; y < tileHeight; ++y )
    {
      for( int x = 0; x < tileWidth; ++x, n+=3 )
      {
        int l = ( ( i * tileHeight + y ) * width + j * tileWidth + x ) * 3;
        d[n] = c[l];
        d[n+1] = c[l+1];
        d[n+2] = c[l+2];
        d[tileWidth*tileHeight*3+n] = edgeData[l/3];
        d[tileWidth*tileHeight*3+n+1] = 1.0f;//edgeData[l/3];
        d[tileWidth*tileHeight*3+n+2] = 1.0f;//edgeData[l/3];
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
            index->removePoint(mosaic[i-r][j+p1]);
            --imagesLeft;
          }
        }

        for( int p1 = -r; imagesLeft > 1 && p1 <= r; ++p1 )
        {
          if( j+p1 >= 0 && j+p1 < numHorizontal && i+r < numVertical && mosaic[i+r][j+p1] >= 0 )
          {
            index->removePoint(mosaic[i+r][j+p1]);
            --imagesLeft;
          }
        }

        for( int p1 = -r+1; imagesLeft > 1 && p1 < r; ++p1 )
        {
          if( i+p1 >= 0 && i+p1 < numVertical && j-r >= 0 && mosaic[i+p1][j-r] >= 0 )
          {
            index->removePoint(mosaic[i+p1][j-r]);
            --imagesLeft;
          }
        }

        for( int p1 = -r+1; imagesLeft > 1 && p1 < r; ++p1 )
        {
          if( i+p1 >= 0 && i+p1 < numVertical && j+r < numHorizontal && mosaic[i+p1][j+r] >= 0 )
          {
            index->removePoint(mosaic[i+p1][j+r]);
            --imagesLeft;
          }
        }
      }

      nanoflann::KNNResultSet<float> resultSet(1);
      resultSet.init(&ret_index, &out_dist_sqr );
      index->findNeighbors(resultSet, &d[0], nanoflann::SearchParams(10));

      index->addPointsBack();
    }
    else
    {
      mat_index->query( &d[0], 1, &ret_index, &out_dist_sqr );
    }

    // Set mosaic tile data
    mosaic[i][j] = ret_index;

    // Set the best image as being used
    used[ ret_index ] = true;

    if( dither )
    {
      float averageColor1[3] = {0,0,0};
      float averageColor2[3] = {0,0,0};

      for( int y = 0, n = 0; y < tileHeight; ++y )
      {
        for( int x = 0; x < tileWidth; ++x, n+=3 )
        {
          int l = ( ( i * tileHeight + y ) * width + j * tileWidth + x ) * 3;

          for( int i1 = 0; i1 < 3; ++i1 )
          {
            averageColor1[i1] += c[l+i1];
            averageColor2[i1] += mat_index->m_data[ret_index*tileArea+n+i1];
          }
        }
      }

      float error[3] = {0,0,0};

      for( int i1 = 0; i1 < 3; ++i1 )
      {
        averageColor1[i1] /= (tileWidth*tileHeight);
        averageColor2[i1] /= (tileWidth*tileHeight);

        error[i1] = averageColor1[i1] - averageColor2[i1];
      }

      if( j < mosaic[0].size() - 1 )
      {
        for( int y = 0, n = 0; y < tileHeight; ++y )
        {
          for( int x = 0; x < tileWidth; ++x, n+=3 )
          {
            // Index of pixel
            int l = ( ( i * tileHeight + y ) * width + (j + 1) * tileWidth + x ) * 3;

            for( int i1 = 0; i1 < 3; ++i1 )
            {
              c[l+i1] += error[i1] * 7.0 / 16.0;
              fit(c[l+i1],i1);
            }
          }
        }
      }

      if( i < mosaic.size() - 1 )
      {
        if( j > 0 )
        {
          for( int y = 0, n = 0; y < tileHeight; ++y )
          {
            for( int x = 0; x < tileWidth; ++x, n+=3 )
            {
              // Index of pixel
              int l = ( ( (i + 1) * tileHeight + y ) * width + (j - 1) * tileWidth + x ) * 3;

              for( int i1 = 0; i1 < 3; ++i1 )
              {
                c[l+i1] += error[i1] * 3.0 / 16.0;
                fit(c[l+i1],i1);
              }
            }
          }
        }

        {
          for( int y = 0, n = 0; y < tileHeight; ++y )
          {
            for( int x = 0; x < tileWidth; ++x, n+=3 )
            {
              // Index of pixel
              int l = ( ( (i + 1) * tileHeight + y ) * width + j * tileWidth + x ) * 3;

              for( int i1 = 0; i1 < 3; ++i1 )
              {
                c[l+i1] += error[i1] * 5.0 / 16.0;
                fit(c[l+i1],i1);
              }
            }
          }
        }

        if( j < mosaic[0].size() - 1 )
        {
          for( int y = 0, n = 0; y < tileHeight; ++y )
          {
            for( int x = 0; x < tileWidth; ++x, n+=3 )
            {
              // Index of pixel
              int l = ( ( (i + 1) * tileHeight + y ) * width + (j + 1) * tileWidth + x ) * 3;

              for( int i1 = 0; i1 < 3; ++i1 )
              {
                c[l+i1] += error[i1] * 1.0 / 16.0;
                fit(c[l+i1],i1);
              }
            }
          }
        }
      }

      cv[(threadIdx+1)%numThreads].notify_one();
    }
  }

  return 0;
}

int generateMosaic( my_kd_tree_t *mat_index, vector< vector< int > > &mosaic, string inputImage, int repeat, bool square, int resize, float edgeWeight, bool smoothImage, bool dither, float gamma, bool gammutMapping, bool quiet )
{
  // Number of images to check against
  int num_images = mat_index->kdtree_get_point_count();

  // Load input image
  VImage image = VImage::vipsload( (char *)inputImage.c_str() ).autorot();

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
  if( resize != 0 ) image = image.resize( (float)resize / (float)(image.width()) );

  int width = image.width();
  int height = image.height();

  // Get image data
  unsigned char * c = ( unsigned char * )image.data();
  float * c2 = new float[3*width*height];

  // Get number of horizontal and vertical tiles
  int numHorizontal = mosaic[0].size();
  int numVertical = mosaic.size();

  // Data about tile size
  int tileWidth = width / numHorizontal;
  int tileHeight = height / numVertical;
  int tileArea = tileWidth*tileHeight*3;

  float * edgeData = new float[width*height];

  generateEdgeWeights( image, edgeData, tileHeight, edgeWeight, smoothImage, quiet );

  // Whether an image was used or not
  vector< bool > used( num_images, false );

  // Number of threads for multithreading: needs to be a square number
  int threads = numberOfCPUS();

  // Totat number of tiles in block
  int total = numVertical*numHorizontal;

  ProgressBar *changingColorspace = new ProgressBar(height/threads, "Changing colorspace");

  if( gammutMapping )
  {
    std::vector<Point> points;

    float center1[3] = {0.0,0.0,0.0};

    int n100 = 0;

    for( int n = 0; n < num_images; ++n )
    {
      float averageColor[3] = {0,0,0};

      for( int k = 0; k < tileWidth * tileHeight; ++k )
      {
        for( int i1 = 0; i1 < 3; ++i1 )
        {
          averageColor[i1] += mat_index->m_data[tileArea*n+3*k+i1];
        }
      }

      for( int i1 = 0; i1 < 3; ++i1 )
      {
        averageColor[i1] /= (tileWidth*tileHeight);
        center1[i1] += averageColor[i1];
      }

      points.push_back(Point(averageColor[0],averageColor[1],averageColor[2]));
    }

    Point center(center1[0]/(float)points.size(),center1[1]/(float)points.size(),center1[2]/(float)points.size());

    Polyhedron poly;

    CGAL::convex_hull_3(points.begin(), points.end(), poly);

    Tree tree(faces(poly).first, faces(poly).second, poly);

    future< void > ret2[threads];

    for( int k = 0; k < threads; ++k )
    {
      ret2[k] = async( launch::async, &changeColorspace, ref(tree), ref(center), c, c2, k*height/threads, (k+1)*height/threads, width, gamma, gammutMapping, changingColorspace, quiet );
    }

    // Wait for threads to finish
    for( int k = 0; k < threads; ++k )
    {
      ret2[k].get();
    }
  }
  else
  {
    Point center;

    Tree tree;

    future< void > ret2[threads];

    for( int k = 0; k < threads; ++k )
    {
      ret2[k] = async( launch::async, &changeColorspace, ref(tree), ref(center), c, c2, k*height/threads, (k+1)*height/threads, width, gamma, gammutMapping, changingColorspace, quiet );
    }

    // Wait for threads to finish
    for( int k = 0; k < threads; ++k )
    {
      ret2[k].get();
    }
  }

  if( !quiet ) changingColorspace->Finish();

  // Create list of numbers in thread block
  vector< vector< int > > indices;

  if( dither)
  {
    indices.resize(threads);
    for( int threadIdx = 0; threadIdx < threads; ++threadIdx )
    {
      indices.push_back( vector<int>() );
      for( int i = threadIdx; i < numVertical; i += threads )
      {
        for( int j = 0; j < numHorizontal; ++j )
        {
          indices[threadIdx].push_back( i*numHorizontal+j );
        }
      }
    }
  }
  else
  {
    vector< int > indices2(total);
    iota( indices2.begin(), indices2.end(), 0 );

    // Shuffle the points so that patterens do not form
    shuffle( indices2.begin(), indices2.end(), default_random_engine(time(NULL)) );

    for( int k = 0; k < threads; ++k )
    {
      indices.push_back( vector< int >( indices2.begin() + k*total/threads, indices2.begin() + (k+1)*total/threads ) );
    }
  }

  condition_variable *cv = new condition_variable[threads];

  ProgressBar *buildingMosaic = new ProgressBar(indices[0].size(), "Building mosaic");

  // Break mosaic into blocks and find best matching tiles inside each block on a different thread
  future< int > ret[threads];

  for( int k = 0; k < threads; ++k )
  {
    ret[k] = async( launch::async, &generateMosaicThread, ref(mat_index), ref(mosaic), ref(indices[k]), ref(used), repeat, c2, cv, k, threads, edgeData, tileWidth, tileHeight, width, numHorizontal, numVertical, dither, buildingMosaic, quiet );
  }

  // Wait for threads to finish
  for( int k = 0; k < threads; ++k )
  {
    ret[k].get();
  }

  if( !quiet ) buildingMosaic->Finish();

  // Get total number of different images used
  return accumulate(used.begin(), used.end(), 0);
}