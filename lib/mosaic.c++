#include "mosaic.h"

void generateThumbnails( vector< string > &names, vector< vector< unsigned char > > &mosaicTileData, vector< vector< unsigned char > > &imageTileData, string imageDirectory, int mosaicTileWidth, int imageTileWidth, bool exclude )
{
  DIR *dir;
  struct dirent *ent;
  string str;

  int size, width, height, mosaicTileArea = mosaicTileWidth*mosaicTileWidth*3, imageTileArea = imageTileWidth*imageTileWidth*3;
  bool different = mosaicTileWidth != imageTileWidth;

  int num_images = 0;
  int i = names.size();

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
  for( num_images += i; i < num_images; ++i )
  {
    try
    {
      str = names[i];
      //cout << str << endl;
      VImage image = VImage::thumbnail((char *)str.c_str(),mosaicTileWidth,VImage::option()->set( "crop", true )->set( "size", VIPS_SIZE_DOWN ));

      if( image.bands() == 1 )
      {
        image = image.bandjoin(image).bandjoin(image);
      }
      if( image.bands() == 4 )
      {
        image = image.flatten();
      }

      unsigned char * c1 = (unsigned char *)image.data();

      if( different )
      {
        VImage image = VImage::thumbnail((char *)str.c_str(),imageTileWidth,VImage::option()->set( "crop", true )->set( "size", VIPS_SIZE_DOWN ));

        if( image.bands() == 1 )
        {
          image = image.bandjoin(image).bandjoin(image);
        }
        if( image.bands() == 4 )
        {
          image = image.flatten();
        }

        unsigned char * c2 = (unsigned char *)image.data();

        imageTileData.push_back( vector< unsigned char >(c2, c2 + imageTileArea) );
      }
      mosaicTileData.push_back( vector< unsigned char >(c1, c1 + mosaicTileArea) );
    }
    catch (...)
    {
      names.erase(names.begin() + i);
      --num_images;
      --i;
    }

    progressbar_inc( processing_images );
  }

  progressbar_finish( processing_images );
}

pair< int, double > computeBest( vector< vector< float > > &imageData, vector< float > &d, int start, int end, int tileWidth, vector< vector< int > > &mosaic, int i, int j, int repeat )
{
  int best = 0; 
  double difference = DBL_MAX;

  int xEnd = ( (j+repeat+1<(int)mosaic[0].size()) ? j+repeat+1 : mosaic[0].size() );
  int yEnd = ( (i+repeat+1<(int)mosaic.size()) ? i+repeat+1 : mosaic.size() );

  for( int k = start; k < end; ++k )
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

  return pair< int, double >( best, difference );
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

  int total = numVertical*numHorizontal;
  vector< int > indices( total );
  iota( indices.begin(), indices.end(), 0 );
  if( repeat > 0 ) shuffle( indices.begin(), indices.end(), default_random_engine(time(NULL)) );

  for( int p = 0; p < total; ++p )
  {
    int i = indices[p] / numHorizontal;
    int j = indices[p] % numHorizontal;
    int best = 0; 
    double difference = DBL_MAX;

    vector< float > d(tileArea);

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

    future< pair< int, double > > ret[20];

    for( double k = 0; k < 20; ++k )
    {
      ret[int(k)] = async( launch::async, &computeBest, ref(imageData), ref(d), k/20.0 * imageData.size(), (k+1)/20.0 * imageData.size(), tileWidth, ref(mosaic), i, j, repeat );
    }
    for( int k = 0; k < 20; ++k )
    {
      pair< int, double > b = ret[k].get();
      if( b.second < difference )
      {
        best = b.first;
        difference = b.second;
      } 
    }

    mosaic[i][j] = best;
    used[ best ] = true;

    if( show ) progressbar_inc( buildingMosaic );
  }

  return accumulate(used.begin(), used.end(), 0);
}

int generateMosaic( KDTreeVectorOfVectorsAdaptor< vector< vector< int > >, int > &mat_index, vector< vector< int > > &mosaic, string inputImage, progressbar *buildingMosaic, int repeat, bool square, int resize )
{
  bool show = ( buildingMosaic != NULL );

  int searchSize = max( 1, 4*repeat*(repeat+1)+1 );
  int *out_dist_sqr = new int[searchSize];

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

  int total = numVertical*numHorizontal;
  vector< int > indices( total );
  iota( indices.begin(), indices.end(), 0 );
  if( repeat > 0 ) shuffle( indices.begin(), indices.end(), default_random_engine(time(NULL)) );

  for( int p = 0; p < total; ++p )
  {
    int i = indices[p] / numHorizontal;
    int j = indices[p] % numHorizontal;

    vector< int > d(tileArea);

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

    if( show ) progressbar_inc( buildingMosaic );
  }

  return accumulate(used.begin(), used.end(), 0);
}

void buildImage( vector< vector< unsigned char > > &imageData, vector< vector< int > > &mosaic, string outputImage, int tileWidth )
{
  cout << "Generating image ..." << endl;

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
}

void buildDeepZoomImage( vector< vector< int > > &mosaic, vector< string > imageNames, int numUnique, string outputImage, ofstream& htmlFile )
{
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

          VImage image = VImage::vipsload( (char *)imageNames[current].c_str() );

          // Find the width of the largest square
          int width = image.width();
          int height = image.height();
          int size = ( width < height ) ? width : height;
            
          int log = round( log2( size ) );

          if( numTiles > 1 ) levelData << ","; 
          levelData << (log-8);

          maxZoomablesLevel = max( maxZoomablesLevel, (log-8) );
          minZoomablesLevel = min( minZoomablesLevel, (log-8) );

          int newSize = 1 << log;

          // Build the zoomable image if necessary 
          string zoomableName = outputDirectory + to_string(mosaic[i][j]);
          if(width < height)
          { 
            image.extract_area(0, (height-size)/2, size, size).resize((double)newSize/(double)size).dzsave((char *)zoomableName.c_str(), VImage::option()->set( "depth", VIPS_FOREIGN_DZ_DEPTH_ONETILE )->set( "tile-size", 256 )->set( "overlap", false )->set( "strip", true ) );
          }
          else
          {
            image.extract_area( (width-size)/2, 0, size, size).resize((double)newSize/(double)size).dzsave((char *)zoomableName.c_str(), VImage::option()->set( "depth", VIPS_FOREIGN_DZ_DEPTH_ONETILE )->set( "tile-size", 256 )->set( "overlap", false )->set( "strip", true ) );
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
      alreadyDone.clear();
      for(int j = 1; j < height; j+=2)
      {
        vector< int > current = { mosaic[j-1][width-1], mosaic[j][width-1] };

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
            (VImage::jpegload((char *)directory.str().append(to_string(mosaic[j-1][width-1])+"_files/0/0_0.jpeg").c_str()).
            join(VImage::jpegload((char *)directory.str().append(to_string(mosaic[j][width-1])+"_files/0/0_0.jpeg").c_str()),VIPS_DIRECTION_VERTICAL)).
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
      alreadyDone.clear();
      for(int j = 1; j < width; j+=2)
      {
        vector< int > current = { mosaic[height-1][j-1], mosaic[height-1][j] };

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
            (VImage::jpegload((char *)directory.str().append(to_string(mosaic[height-1][j-1])+"_files/0/0_0.jpeg").c_str()).
            join(VImage::jpegload((char *)directory.str().append(to_string(mosaic[height-1][j])+"_files/0/0_0.jpeg").c_str()),VIPS_DIRECTION_HORIZONTAL)).
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
        VImage::jpegload((char *)directory.str().append(to_string(mosaic[height-1][width-1])+"_files/0/0_0.jpeg").c_str()).
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