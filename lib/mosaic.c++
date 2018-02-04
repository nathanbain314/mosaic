#include "mosaic.h"

void generateThumbnails( vector< cropType > &cropData, vector< vector< unsigned char > > &mosaicTileData, vector< vector< unsigned char > > &imageTileData, string imageDirectory, int mosaicTileWidth, int imageTileWidth, bool exclude, bool spin, int cropStyle, bool flip )
{
  DIR *dir;
  struct dirent *ent;
  string str;

  int size, xOffset, yOffset, width, height, mosaicTileArea = mosaicTileWidth*mosaicTileWidth*3, imageTileArea = imageTileWidth*imageTileWidth*3;
  bool different = mosaicTileWidth != imageTileWidth;

  unsigned char *c1, *c2;

  int minAllow = exclude ? max(256,max(mosaicTileWidth,imageTileWidth)) : max(mosaicTileWidth,imageTileWidth);

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
    }
  }

  cout << endl;

  progressbar *processing_images = progressbar_new("Processing images", num_images);

  // Iterate through all images in directory
  for( int i = 0; i < num_images; ++i )
  {
    progressbar_inc( processing_images );
    try
    {
      str = names[i];

      width = VImage::new_memory().vipsload( (char *)str.c_str() ).width();
      height = VImage::new_memory().vipsload( (char *)str.c_str() ).height();
      int minSize = min(width,height);
      int maxSize = max(width,height);
      bool vertical = width < height;

      if( minSize < minAllow )
      {
        continue;
      }

      int sw = floor((double)maxSize*((double)mosaicTileWidth/(double)minSize));
      int lw = floor((double)maxSize*((double)imageTileWidth/(double)minSize));

      VImage image2, image;
      
      switch( cropStyle )
      {
        case 0:
          image = VImage::thumbnail((char *)str.c_str(),mosaicTileWidth,VImage::option()->set( "crop", VIPS_INTERESTING_CENTRE )->set( "size", VIPS_SIZE_DOWN ));
          break;
        case 1:
          image = VImage::thumbnail((char *)str.c_str(),mosaicTileWidth,VImage::option()->set( "crop", VIPS_INTERESTING_ENTROPY )->set( "size", VIPS_SIZE_DOWN ));
          break;
        case 2:
          image = VImage::thumbnail((char *)str.c_str(),mosaicTileWidth,VImage::option()->set( "crop", VIPS_INTERESTING_ATTENTION )->set( "size", VIPS_SIZE_DOWN ));
          break;
        case 3:
          image = vips_foreign_is_a("gifload",(char *)str.c_str()) ? 
                  VImage::thumbnail((char *)str.c_str(),ceil((double)maxSize*((double)mosaicTileWidth/(double)minSize)),VImage::option()->set( "size", VIPS_SIZE_DOWN )).cache(VImage::option()->set("tile_width",16)->set("tile_height",16)) : 
                  VImage::thumbnail((char *)str.c_str(),ceil((double)maxSize*((double)mosaicTileWidth/(double)minSize)),VImage::option()->set( "size", VIPS_SIZE_DOWN )).cache(VImage::option()->set("tile_width",ceil((double)width*((double)mosaicTileWidth/(double)minSize)))->set("tile_height",ceil((double)height*((double)mosaicTileWidth/(double)minSize))));
          break;
      }

      xOffset = min(-(int)image.xoffset(),sw - mosaicTileWidth);
      yOffset = min(-(int)image.yoffset(),sw - mosaicTileWidth);

      if( image.bands() == 1 )
      {
        image = image.bandjoin(image).bandjoin(image);
      }
      else if( image.bands() == 4 )
      {
        image = image.flatten();
      }

      if( different )
      {
        switch( cropStyle )
        {
          case 0:
            image2 = VImage::thumbnail((char *)str.c_str(),imageTileWidth,VImage::option()->set( "crop", VIPS_INTERESTING_CENTRE )->set( "size", VIPS_SIZE_DOWN ));
            break;
          case 1:
            image2 = VImage::thumbnail((char *)str.c_str(),imageTileWidth,VImage::option()->set( "crop", VIPS_INTERESTING_ENTROPY )->set( "size", VIPS_SIZE_DOWN ));
            break;
          case 2:
            image2 = VImage::thumbnail((char *)str.c_str(),imageTileWidth,VImage::option()->set( "crop", VIPS_INTERESTING_ATTENTION )->set( "size", VIPS_SIZE_DOWN ));
            break;
          case 3:
            image2 = vips_foreign_is_a("gifload",(char *)str.c_str()) ? 
                  VImage::thumbnail((char *)str.c_str(),ceil((double)maxSize*((double)imageTileWidth/(double)minSize)),VImage::option()->set( "size", VIPS_SIZE_DOWN )).cache(VImage::option()->set("tile_width",16)->set("tile_height",16)) : 
                  VImage::thumbnail((char *)str.c_str(),ceil((double)maxSize*((double)imageTileWidth/(double)minSize)),VImage::option()->set( "size", VIPS_SIZE_DOWN )).cache(VImage::option()->set("tile_width",ceil((double)width*((double)imageTileWidth/(double)minSize)))->set("tile_height",ceil((double)height*((double)imageTileWidth/(double)minSize))));
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

      if( cropStyle == 3 )
      {
        for( int ii = sw - mosaicTileWidth; ii >= 0; --ii )
        {
          VImage newImage, newImage2;

          if( vertical )
          {
            newImage = image.extract_area(0,ii,mosaicTileWidth,mosaicTileWidth);
            if(different) newImage2 = image2.extract_area(0,ii*imageTileWidth/mosaicTileWidth,imageTileWidth,imageTileWidth);
          }
          else
          {
            newImage = image.extract_area(ii,0,mosaicTileWidth,mosaicTileWidth);
            if(different) newImage2 = image2.extract_area(ii*imageTileWidth/mosaicTileWidth,0,imageTileWidth,imageTileWidth);
          }

          for( int ff = flip; ff >= 0; --ff)
          {
            if(flip)
            {
              newImage = newImage.flip(VIPS_DIRECTION_HORIZONTAL);
              if( different ) newImage2 = newImage2.flip(VIPS_DIRECTION_HORIZONTAL);
            }

            for( int jj = (spin ? 3:0); jj >= 0; --jj )
            {
              if(spin) newImage = newImage.rot90();
              c1 = (unsigned char *)newImage.data();
              mosaicTileData.push_back( vector< unsigned char >(c1, c1 + mosaicTileArea) );

              if( different )
              {
                if(spin) newImage2 = newImage2.rot90();
                c2 = (unsigned char *)newImage2.data();
                imageTileData.push_back( vector< unsigned char >(c2, c2 + imageTileArea) );
              }

              cropData.push_back( make_tuple(str,vertical?0:ii*minSize/mosaicTileWidth,vertical?ii*minSize/mosaicTileWidth:0,jj,ff) );
            }
          }
        }
      }
      else
      {
        for( int ff = flip; ff >= 0; --ff)
        {
          if(flip)
          {
            image = image.flip(VIPS_DIRECTION_HORIZONTAL);
            if( different ) image2 = image2.flip(VIPS_DIRECTION_HORIZONTAL);
          }

          for( int jj = (spin ? 3:0); jj >= 0; --jj )
          {
            if(spin) image = image.rot90();
            c1 = (unsigned char *)image.data();
            mosaicTileData.push_back( vector< unsigned char >(c1, c1 + mosaicTileArea) );

            if( different )
            {
              if(spin) image2 = image2.rot90();
              c2 = (unsigned char *)image2.data();
              imageTileData.push_back( vector< unsigned char >(c2, c2 + imageTileArea) );
            }

            cropData.push_back( make_tuple(str,xOffset*minSize/mosaicTileWidth,yOffset*minSize/mosaicTileWidth,jj,ff) );
          }
        }
      }
    }
    catch (...)
    {
    }
  }

  progressbar_finish( processing_images );
}

int generateLABBlock( vector< vector< float > > &imageData, vector< vector< int > > &mosaic, vector< int > &indices, vector< bool > &used, int repeat, unsigned char * c, int blockX, int blockY, int blockWidth, int blockHeight, int tileWidth, int width, int numHorizontal, int numVertical, progressbar* buildingMosaic )
{
  bool show = !(blockX+blockY);

  vector< float > d(tileWidth*tileWidth*3);

  int num_images = imageData.size();

  for( int p = 0; p < indices.size(); ++p )
  {
    if(show) progressbar_inc( buildingMosaic );
    int i = indices[p] / blockWidth;
    int j = indices[p] % blockWidth;
    
    if( i >= blockHeight || j >= blockWidth ) continue;
    
    i += blockY;
    j += blockX;

    int xEnd = ( (j+repeat+1<(int)mosaic[0].size()) ? j+repeat+1 : mosaic[0].size() );
    int yEnd = ( (i+repeat+1<(int)mosaic.size()) ? i+repeat+1 : mosaic.size() );

    for( int y = 0, n = 0; y < tileWidth; ++y )
    {
      for( int x = 0; x < tileWidth; ++x, n+=3 )
      {
        int l = ( ( i * tileWidth + y ) * width + j * tileWidth + x ) * 3;
        
        float r,g,b;
        vips_col_sRGB2scRGB_16(c[l],c[l+1],c[l+2], &r,&g,&b );
        vips_col_scRGB2XYZ( r, g, b, &r, &g, &b );
        vips_col_XYZ2Lab( r, g, b, &d[n], &d[n+1], &d[n+2] );
      }
    }

    int best = 0;
    double difference = DBL_MAX;

    for( int k = 0; k < num_images; ++k )
    {
      if( repeat > 0 )
      {
        bool valid = true;

        for( int y = (i-repeat > 0) ? i-repeat : 0; valid && y < yEnd; ++y )
        {
          for( int x = (j-repeat > 0) ? j-repeat : 0; valid && x < xEnd; ++x )
          {
            if(  y == i && x == j ) continue;
            if( mosaic[y][x] == k )
            {
              valid = false;
            }
          }
        }

        if( !valid ) continue;
      }

      double sum = 0;
      for( int l = 0; l < tileWidth*tileWidth*3; l+=3 )
      {
        sum += vips_col_dE00( imageData[k][l], imageData[k][l+1], imageData[k][l+2], d[l], d[l+1], d[l+2] );
      }

      if( sum < difference )
      {
        difference = sum;
        best = k;
      }
    }
    mosaic[i][j] = best;

    used[ best ] = true;
  }

  return 0;
}

// Takes a vector of image thumbnails, input image and number of images per row and creates output image
int generateMosaic( vector< vector< float > > &imageData, vector< vector< int > > &mosaic, string inputImage, progressbar *buildingMosaic, int repeat, bool square, int resize )
{
  bool show = ( buildingMosaic != NULL );

  VImage image = VImage::new_memory().vipsload( (char *)inputImage.c_str() );//.colourspace( VIPS_INTERPRETATION_LAB );

  if( image.bands() == 1 )
  {
    image = image.bandjoin(image).bandjoin(image);
  }
  if( image.bands() == 4 )
  {
    image = image.flatten();
  }

  if(square)
  {
    image = (image.width() < image.height()) ? image.extract_area(0, (image.height()-image.width())/2, image.width(), image.width()) :
                                               image.extract_area((image.width()-image.height())/2, 0, image.height(), image.height());
  }

  if( resize != 0 ) image = image.resize( (double)resize / (double)(image.width()) );

  int width = image.width();

  unsigned char * c = ( unsigned char * )image.data();

  int numHorizontal = mosaic[0].size();
  int numVertical = mosaic.size();

  int tileWidth = width / numHorizontal;
  int tileArea = tileWidth*tileWidth*3;

  int num_images = imageData.size();

  vector< bool > used( num_images, false );

  int threads = 9;
  int sqrtThreads = sqrt(threads);

  int total = ceil((double)numVertical/(double)sqrtThreads)*ceil((double)numHorizontal/(double)sqrtThreads);

  vector< int > indices( total );
  iota( indices.begin(), indices.end(), 0 );
  if( repeat > 0 ) shuffle( indices.begin(), indices.end(), default_random_engine(time(NULL)) );

  future< int > ret[threads];

  for( int i = 0, k = 0; i < sqrtThreads; ++i )
  {
    for( int j = 0; j < sqrtThreads; ++j, ++k )
    {
      ret[k] = async( launch::async, &generateLABBlock, ref(imageData), ref(mosaic), ref(indices), ref(used), repeat, c, j*numHorizontal/sqrtThreads, i*numVertical/sqrtThreads, (j+1)*numHorizontal/sqrtThreads-j*numHorizontal/sqrtThreads, (i+1)*numVertical/sqrtThreads-i*numVertical/sqrtThreads, tileWidth, width, numHorizontal, numVertical, buildingMosaic );
    }
  }

  for( int k = 0; k < threads; ++k )
  {
    ret[k].get();
  }

  return accumulate(used.begin(), used.end(), 0);
}

int generateRGBBlock( my_kd_tree_t &mat_index, vector< vector< int > > &mosaic, vector< int > &indices, vector< bool > &used, int repeat, unsigned char * c, int blockX, int blockY, int blockWidth, int blockHeight, int tileWidth, int width, int numHorizontal, int numVertical, progressbar* buildingMosaic )
{
  bool show = !(blockX+blockY);

  int searchSize = max( 1, 4*repeat*(repeat+1)+1 );
  int *out_dist_sqr = new int[searchSize];

  vector< int > d(tileWidth*tileWidth*3);

  for( int p = 0; p < indices.size(); ++p )
  {
    if(show) progressbar_inc( buildingMosaic );
    int i = indices[p] / blockWidth;
    int j = indices[p] % blockWidth;
    
    if( i >= blockHeight || j >= blockWidth ) continue;
    
    i += blockY;
    j += blockX;

    for( int y = 0, n = 0; y < tileWidth; ++y )
    {
      for( int x = 0; x < tileWidth; ++x, n+=3 )
      {
        int l = ( ( i * tileWidth + y ) * width + j * tileWidth + x ) * 3;
        d[n] = c[l];
        d[n+1] = c[l+1];
        d[n+2] = c[l+2];
      }
    }

    int xEnd = ( (j+repeat+1<numHorizontal) ? j+repeat+1 : numHorizontal );
    int yEnd = ( (i+repeat+1<numVertical) ? i+repeat+1 : numVertical );

    vector< size_t > ret_index( searchSize );

    mat_index.query( &d[0], searchSize, ret_index.data(), &out_dist_sqr[0] );

    if( searchSize > 1 )
    {
      for( int y = (i-repeat > 0) ? i-repeat : 0; y < yEnd; ++y )
      {
        for( int x = (j-repeat > 0) ? j-repeat : 0; x < xEnd; ++x )
        {
          if(  y == i && x == j ) continue;
          vector< size_t >::iterator position = find(ret_index.begin(), ret_index.end(), mosaic[y][x]);
          if (position != ret_index.end()) ret_index.erase(position);
        }
      }
    }
    mosaic[i][j] = ret_index[0];

    used[ ret_index[0] ] = true;
  }

  delete [] out_dist_sqr;

  return 0;
}

int generateMosaic( my_kd_tree_t &mat_index, vector< vector< int > > &mosaic, string inputImage, progressbar *buildingMosaic, int repeat, bool square, int resize )
{
  bool show = ( buildingMosaic != NULL );

  int num_images = mat_index.kdtree_get_point_count();

  VImage image = VImage::vipsload( (char *)inputImage.c_str() );

  if( image.bands() == 1 )
  {
    image = image.bandjoin(image).bandjoin(image);
  }
  if( image.bands() == 4 )
  {
    image = image.flatten();
  }

  if(square)
  {
    image = (image.width() < image.height()) ? image.extract_area(0, (image.height()-image.width())/2, image.width(), image.width()) :
                                               image.extract_area((image.width()-image.height())/2, 0, image.height(), image.height());
  }

  if( resize != 0 ) image = image.resize( (double)resize / (double)(image.width()) );

  int width = image.width();

  unsigned char * c = ( unsigned char * )image.data();

  int numHorizontal = mosaic[0].size();
  int numVertical = mosaic.size();

  int tileWidth = width / numHorizontal;
  int tileArea = tileWidth*tileWidth*3;

  vector< bool > used( num_images, false );

  int threads = 9;
  int sqrtThreads = sqrt(threads);

  int total = ceil((double)numVertical/(double)sqrtThreads)*ceil((double)numHorizontal/(double)sqrtThreads);

  vector< int > indices( total );
  iota( indices.begin(), indices.end(), 0 );
  if( repeat > 0 ) shuffle( indices.begin(), indices.end(), default_random_engine(time(NULL)) );

  future< int > ret[threads];

  for( int i = 0, k = 0; i < sqrtThreads; ++i )
  {
    for( int j = 0; j < sqrtThreads; ++j, ++k )
    {
      ret[k] = async( launch::async, &generateRGBBlock, ref(mat_index), ref(mosaic), ref(indices), ref(used), repeat, c, j*numHorizontal/sqrtThreads, i*numVertical/sqrtThreads, (j+1)*numHorizontal/sqrtThreads-j*numHorizontal/sqrtThreads, (i+1)*numVertical/sqrtThreads-i*numVertical/sqrtThreads, tileWidth, width, numHorizontal, numVertical, buildingMosaic );
    }
  }

  for( int k = 0; k < threads; ++k )
  {
    ret[k].get();
  }

  return accumulate(used.begin(), used.end(), 0);
}

void buildImage( vector< vector< unsigned char > > &imageData, vector< vector< int > > &mosaic, string outputImage, int tileWidth )
{
  cout << "Generating image " << outputImage << endl;

  int width = mosaic[0].size();
  int height = mosaic.size(); 
  unsigned char *data = new unsigned char[width*height*tileWidth*tileWidth*3];

  for( int i = 0; i < height; ++i )
  {
    for( int j = 0; j < width; ++j )
    {
      for( int y = 0; y < tileWidth; ++y )
      {
        for( int x = 0; x < tileWidth; ++x )
        {
          data[ 3 * ( width * tileWidth * ( i * tileWidth + y ) + j * tileWidth + x ) ] = imageData[mosaic[i][j]][3*(y*tileWidth+x)];
          data[ 3 * ( width * tileWidth * ( i * tileWidth + y ) + j * tileWidth + x ) + 1 ] = imageData[mosaic[i][j]][3*(y*tileWidth+x)+1];
          data[ 3 * ( width * tileWidth * ( i * tileWidth + y ) + j * tileWidth + x ) + 2 ] = imageData[mosaic[i][j]][3*(y*tileWidth+x)+2];
        }
      }
    }
  }

  VImage::new_from_memory( data, width*height*tileWidth*tileWidth*3, width*tileWidth, height*tileWidth, 3, VIPS_FORMAT_UCHAR ).vipssave((char *)outputImage.c_str());

  delete [] data;
}

void buildDeepZoomImage( vector< vector< int > > &mosaic, vector< cropType > cropData, int numUnique, string outputImage, ofstream& htmlFile )
{
  VipsAngle rotAngle[4] = {VIPS_ANGLE_D0,VIPS_ANGLE_D270,VIPS_ANGLE_D180,VIPS_ANGLE_D90};

  ostringstream levelData;

  int width = mosaic[0].size();
  int height = mosaic.size();

  int numHorizontal = width;
  int numVertical = height;
  int maxZoomablesLevel = INT_MIN;
  int minZoomablesLevel = INT_MAX;

  int level = (int)ceil( log2((width > height) ? width : height ) ) + 6;
  bool first = true;

  string outputDirectory = string( outputImage + "zoom/" );

  g_mkdir(outputDirectory.c_str(), 0777);

  vector< string > mosaicStrings;

  progressbar *generatingZoomables = progressbar_new("Generating zoomables", numUnique);

  levelData << "var levelData = [";

  {
    ostringstream currentMosaic;
    vector< int > alreadyDone;
    int numTiles = 0;

    currentMosaic << "[";
    for( int i = 0; i < height; ++i )
    {
      currentMosaic << "[";
      for( int j = 0; j < width; ++j )
      {
        int current = mosaic[i][j];

        bool skip = false;
        int k = 0;
        for( ; k < numTiles; ++k )
        {
          if( alreadyDone[k] == current )
          {
            skip = true;
            break;
          }
        }

        if( !skip )
        {
          alreadyDone.push_back( current );
          mosaic[i][j] = numTiles++;

          VImage image = VImage::vipsload( (char *)(get<0>(cropData[current])).c_str() );

          // Find the width of the largest square
          int width = image.width();
          int height = image.height();
          int size = min( width, height );
            
          int log = round( log2( size ) );

          if( numTiles > 1 ) levelData << ","; 
          levelData << (log-8);

          maxZoomablesLevel = max( maxZoomablesLevel, (log-8) );
          minZoomablesLevel = min( minZoomablesLevel, (log-8) );

          int newSize = 1 << log;

          // Build the zoomable image if necessary 
          string zoomableName = outputDirectory + to_string(mosaic[i][j]);

          if( get<4>(cropData[current]) )
          {
            image.extract_area(get<1>(cropData[current]), get<2>(cropData[current]), size, size).resize((double)newSize/(double)size).flip(VIPS_DIRECTION_HORIZONTAL).rot(rotAngle[get<3>(cropData[current])]).dzsave((char *)zoomableName.c_str(), VImage::option()->set( "depth", VIPS_FOREIGN_DZ_DEPTH_ONETILE )->set( "tile-size", 256 )->set( "overlap", false )->set( "strip", true ) );
          }
          else
          {
            image.extract_area(get<1>(cropData[current]), get<2>(cropData[current]), size, size).resize((double)newSize/(double)size).rot(rotAngle[get<3>(cropData[current])]).dzsave((char *)zoomableName.c_str(), VImage::option()->set( "depth", VIPS_FOREIGN_DZ_DEPTH_ONETILE )->set( "tile-size", 256 )->set( "overlap", false )->set( "strip", true ) );
          }
          progressbar_inc( generatingZoomables );
        }
        else
        {
          mosaic[i][j] = k;
        }
        currentMosaic << mosaic[i][j];
        if( j < width - 1 ) currentMosaic << ","; 
      }
      currentMosaic << "]";
      if( i < height - 1 ) currentMosaic << ",";
    }
    currentMosaic << "]";
    mosaicStrings.push_back(currentMosaic.str());
  }

  progressbar_finish( generatingZoomables );
  progressbar *generatingLevels = progressbar_new("Generating levels", height);

  levelData << "];";

  for( ; level >= 0; --level )
  { 
    vector< vector< int > > alreadyDone;
    int numTiles = 0;

    ostringstream image, directory, input, currentMosaic;

    // Make the directory for the zoomable image
    image << outputImage << level << "/";
    input << outputImage << level + 1 << "/";
    directory << outputDirectory;
    g_mkdir(image.str().c_str(), 0777);

    for( int i = 1; i < height; i+=2 )
    {
      for( int j = 1; j < width; j+=2 )
      {
        vector< int > current = { mosaic[i-1][j-1], mosaic[i-1][j], mosaic[i][j-1], mosaic[i][j] };

        bool skip = false;
        int k = 0;
        for( ; k < numTiles; ++k )
        {
          if( alreadyDone[k] == current )
          {
            skip = true;
            break;
          }
        }

        if( !skip )
        {
          alreadyDone.push_back( current );

          if( first )
          {
            (VImage::jpegload((char *)directory.str().append(to_string(mosaic[i-1][j-1])+"_files/0/0_0.jpeg").c_str(), VImage::option()->set( "shrink", 2)).
            join(VImage::jpegload((char *)directory.str().append(to_string(mosaic[i-1][j])+"_files/0/0_0.jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL)).
            join(VImage::jpegload((char *)directory.str().append(to_string(mosaic[i][j-1])+"_files/0/0_0.jpeg").c_str(), VImage::option()->set( "shrink", 2)).
            join(VImage::jpegload((char *)directory.str().append(to_string(mosaic[i][j])+"_files/0/0_0.jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL),VIPS_DIRECTION_VERTICAL).
            jpegsave((char *)image.str().append(to_string(numTiles)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );
          }
          else
          {
            (VImage::jpegload((char *)input.str().append(to_string(mosaic[i-1][j-1])+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
            join(VImage::jpegload((char *)input.str().append(to_string(mosaic[i-1][j])+".jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL)).
            join(VImage::jpegload((char *)input.str().append(to_string(mosaic[i][j-1])+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
            join(VImage::jpegload((char *)input.str().append(to_string(mosaic[i][j])+".jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL),VIPS_DIRECTION_VERTICAL).
            jpegsave((char *)image.str().append(to_string(numTiles)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );
          }

          mosaic[i/2][j/2] = numTiles++;
        }
        else
        {
          mosaic[i/2][j/2] = k;
        }
      }
      progressbar_inc( generatingLevels );
    }

    if(width%2 == 1)
    {
      int prevTileSize = numTiles;
      alreadyDone.clear();
      for(int j = 1; j < height; j+=2)
      {
        vector< int > current = { mosaic[j-1][width-1], mosaic[j][width-1] };

        bool skip = false;
        int k = prevTileSize;
        for( ; k < numTiles; ++k )
        {
          if( alreadyDone[k-prevTileSize] == current )
          {
            skip = true;
            break;
          }
        }

        if( !skip )
        {
          alreadyDone.push_back( current );

          if( first )
          {
            (VImage::jpegload((char *)directory.str().append(to_string(mosaic[j-1][width-1])+"_files/0/0_0.jpeg").c_str(), VImage::option()->set( "shrink", 2)).
            join(VImage::jpegload((char *)directory.str().append(to_string(mosaic[j][width-1])+"_files/0/0_0.jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_VERTICAL)).
            jpegsave((char *)image.str().append(to_string(numTiles)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );
          }
          else
          {
            (VImage::jpegload((char *)input.str().append(to_string(mosaic[j-1][width-1])+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
            join(VImage::jpegload((char *)input.str().append(to_string(mosaic[j][width-1])+".jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_VERTICAL)).
            jpegsave((char *)image.str().append(to_string(numTiles)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );
          }

          mosaic[j/2][width/2] = numTiles++;
        }
        else
        {
          mosaic[j/2][width/2] = k;
        }
      }
    }

    if(height%2 == 1)
    {
      int prevTileSize = numTiles;
      alreadyDone.clear();
      for(int j = 1; j < width; j+=2)
      {
        vector< int > current = { mosaic[height-1][j-1], mosaic[height-1][j] };

        bool skip = false;
        int k = prevTileSize;
        for( ; k < numTiles; ++k )
        {
          if( alreadyDone[k-prevTileSize] == current )
          {
            skip = true;
            break;
          }
        }

        if( !skip )
        {
          alreadyDone.push_back( current );

          if( first )
          {
            (VImage::jpegload((char *)directory.str().append(to_string(mosaic[height-1][j-1])+"_files/0/0_0.jpeg").c_str(), VImage::option()->set( "shrink", 2)).
            join(VImage::jpegload((char *)directory.str().append(to_string(mosaic[height-1][j])+"_files/0/0_0.jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL)).
            jpegsave((char *)image.str().append(to_string(numTiles)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );
          }
          else
          {
            (VImage::jpegload((char *)input.str().append(to_string(mosaic[height-1][j-1])+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
            join(VImage::jpegload((char *)input.str().append(to_string(mosaic[height-1][j])+".jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL)).
            jpegsave((char *)image.str().append(to_string(numTiles)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );
          }

          mosaic[height/2][j/2] = numTiles++;
        }
        else
        {
          mosaic[height/2][j/2] = k;
        }
      }
    }

    if(width%2 == 1 && height%2 == 1)
    {
      mosaic[height/2][width/2] = numTiles;
      if( first )
      {
        VImage::jpegload((char *)directory.str().append(to_string(mosaic[height-1][width-1])+"_files/0/0_0.jpeg").c_str(), VImage::option()->set( "shrink", 2)).
        jpegsave((char *)image.str().append(to_string(numTiles)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );  
      }
      else
      {
        VImage::jpegload((char *)input.str().append(to_string(mosaic[height-1][width-1])+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
        jpegsave((char *)image.str().append(to_string(numTiles)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );  
      }
    }

    width = (int)ceil((double)width/2.0);
    height = (int)ceil((double)height/2.0);

    currentMosaic << "[";

    for( int i = 0; i < height; ++i )
    {
      currentMosaic << "[";
      for( int j = 0; j < width; ++j )
      {
        currentMosaic << mosaic[i][j];
        if( j < width-1 ) currentMosaic << ",";
      }
      currentMosaic << "]";
      if( i < height-1 ) currentMosaic << ",";
    }
    currentMosaic << "]";

    mosaicStrings.push_back(currentMosaic.str());

    first = false;
  }

  progressbar_finish( generatingLevels );

  htmlFile << levelData.str() << endl;

  htmlFile << "var data = [ \n";

  for( int i = mosaicStrings.size()-1; i>=0; --i )
  {
    htmlFile << mosaicStrings[i] << "," << endl;
  }

  htmlFile << "];";

  htmlFile << "var mosaicWidth = " << numHorizontal << ";\nvar mosaicHeight = " << numVertical << ";\nvar mosaicLevel = " << mosaicStrings.size()-1 << ";\nvar minZoomablesLevel = " << minZoomablesLevel << ";\nvar maxZoomablesLevel = " << maxZoomablesLevel << ";\n";
}

void buildContinuousMosaic( vector< vector< vector< int > > > mosaic, vector< VImage > &images, string outputDirectory, ofstream& htmlFile )
{
  vector< string > mosaicStrings;
  vector< vector< vector< int > > > alreadyDone( 6 );

  g_mkdir( outputDirectory.c_str(), 0777);
  g_mkdir( string( outputDirectory ).append( "0/" ).c_str(), 0777);
  g_mkdir( string( outputDirectory ).append( "1/" ).c_str(), 0777);
  g_mkdir( string( outputDirectory ).append( "2/" ).c_str(), 0777);
  g_mkdir( string( outputDirectory ).append( "3/" ).c_str(), 0777);
  g_mkdir( string( outputDirectory ).append( "4/" ).c_str(), 0777);
  g_mkdir( string( outputDirectory ).append( "5/" ).c_str(), 0777);

  int numImages = mosaic.size();

  int size = 64;

  {
    ostringstream currentMosaic;
    currentMosaic << "[";

    progressbar *generatingLevels = progressbar_new("Generating level 5", numImages);

    for( int n = 0; n < numImages; ++n )
    {
      currentMosaic << "[";
      for( int i = 0; i < size; ++i )
      {
        currentMosaic << "[";
        for( int j = 0; j < size; ++j )
        {
          currentMosaic << mosaic[n][i][j];
          if( j < size-1 ) currentMosaic << ",";
        }
        currentMosaic << "]";
        if( i < size-1 ) currentMosaic << ",";
      }
      currentMosaic << "]";
      if( n < numImages-1 ) currentMosaic << ",";

      for( int i = 1; i < size; i+=2 )
      {
        for( int j = 1; j < size; j+=2 )
        {
          vector< int > current = { mosaic[n][i-1][j-1], mosaic[n][i-1][j], mosaic[n][i][j-1], mosaic[n][i][j] };

          bool skip = false;
          int k = 0;
          for( ; k < (int)alreadyDone[5].size(); ++k )
          {
            if( alreadyDone[5][k] == current )
            {
              skip = true;
              break;
            }
          }

          if( !skip )
          {
            alreadyDone[5].push_back( current );

            mosaic[n][i>>1][j>>1] = alreadyDone[5].size()-1;

            (images[current[0]].
            join(images[current[1]],VIPS_DIRECTION_HORIZONTAL)).
            join(images[current[2]].
            join(images[current[3]],VIPS_DIRECTION_HORIZONTAL),VIPS_DIRECTION_VERTICAL).
            jpegsave((char *)string( outputDirectory ).append( "5/" + to_string(mosaic[n][i>>1][j>>1]) + ".jpeg" ).c_str() /*, VImage::option()->set( "optimize_coding", true )->set( "strip", true ) */ ); 
          }
          else
          {
            mosaic[n][i>>1][j>>1] = k;
          }
        }
      }
      progressbar_inc( generatingLevels );
    }

    progressbar_finish( generatingLevels );

    currentMosaic << "]\n";

    mosaicStrings.push_back( currentMosaic.str() );
  }

  for( int l = 4; l >= 0 ; --l )
  {
    ostringstream currentMosaic;
    currentMosaic << "[";

    string progressString = "Generating level "+ to_string(l);
    progressbar *generatingLevels = progressbar_new(progressString.c_str(), numImages);

    size = size >> 1;
    for( int n = 0; n < numImages; ++n )
    {
      currentMosaic << "[";
      for( int i = 0; i < size; ++i )
      {
        currentMosaic << "[";
        for( int j = 0; j < size; ++j )
        {
          currentMosaic << mosaic[n][i][j];
          if( j < size-1 ) currentMosaic << ",";
        }
        currentMosaic << "]";
        if( i < size-1 ) currentMosaic << ",";
      }
      currentMosaic << "]";
      if( n < numImages-1 ) currentMosaic << ",";

      for( int i = 1; i < size; i+=2 )
      {
        for( int j = 1; j < size; j+=2 )
        {
          vector< int > current = { mosaic[n][i-1][j-1], mosaic[n][i-1][j], mosaic[n][i][j-1], mosaic[n][i][j] };

          bool skip = false;
          int k = 0;
          for( ; k < (int)alreadyDone[l].size(); ++k )
          {
            if( alreadyDone[l][k] == current )
            {
              skip = true;
              break;
            }
          }

          if( !skip )
          {
            alreadyDone[l].push_back( current );
            mosaic[n][i>>1][j>>1] = alreadyDone[l].size()-1;

            (VImage::jpegload((char *)string( outputDirectory ).append(to_string(l+1) + "/" + to_string(current[0])+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
            join(VImage::jpegload((char *)string( outputDirectory ).append(to_string(l+1) + "/" + to_string(current[1])+".jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL)).
            join(VImage::jpegload((char *)string( outputDirectory ).append(to_string(l+1) + "/" + to_string(current[2])+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
            join(VImage::jpegload((char *)string( outputDirectory ).append(to_string(l+1) + "/" + to_string(current[3])+".jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL),VIPS_DIRECTION_VERTICAL).
            jpegsave((char *)string( outputDirectory ).append(to_string(l) + "/" + to_string(alreadyDone[l].size()-1)+".jpeg").c_str() /*, VImage::option()->set( "optimize_coding", true )->set( "strip", true )*/ );
          }
          else
          {
            mosaic[n][i>>1][j>>1] = k;
          }
        }
      }
      progressbar_inc( generatingLevels );
    }

    progressbar_finish( generatingLevels );

    currentMosaic << "]\n";
    mosaicStrings.push_back( currentMosaic.str() );
  }

  {
    ostringstream currentMosaic;
    currentMosaic << "[";

    size = size >> 1;
    for( int n = 0; n < numImages; ++n )
    {
      currentMosaic << "[";
      for( int i = 0; i < size; ++i )
      {
        currentMosaic << "[";
        for( int j = 0; j < size; ++j )
        {
          currentMosaic << mosaic[n][i][j];
          if( j < size-1 ) currentMosaic << ",";
        }
        currentMosaic << "]";
        if( i < size-1 ) currentMosaic << ",";
      }
      currentMosaic << "]";
      if( n < numImages-1 ) currentMosaic << ",";
    }

    currentMosaic << "]\n";
    mosaicStrings.push_back( currentMosaic.str() );
  }

  htmlFile << "var data = [ \n";

  for( int i = mosaicStrings.size()-1; i>=0; --i )
  {
    htmlFile << mosaicStrings[i] << "," << endl;
  }

  htmlFile << "];";
}