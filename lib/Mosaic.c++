#include "Mosaic.h"

void buildImage( vector< vector< unsigned char > > &imageData, vector< vector< int > > &mosaic, string outputImage, int tileWidth, int tileHeight )
{
  cout << "Generating image " << outputImage << endl;

  // Dimensions of mosaic
  int width = mosaic[0].size();
  int height = mosaic.size();

  // Create output image data 
  unsigned char *data = new unsigned char[width*height*tileWidth*tileHeight*3];

  // Itereate through everty tile
  for( int i = 0; i < height; ++i )
  {
    for( int j = 0; j < width; ++j )
    {
      // Iterate through every pixel in tile
      for( int y = 0; y < tileHeight; ++y )
      {
        for( int x = 0; x < tileWidth; ++x )
        {
          // Save the image data to the output tile
          data[ 3 * ( width * tileWidth * ( i * tileHeight + y ) + j * tileWidth + x ) ] = imageData[mosaic[i][j]][3*(y*tileWidth+x)];
          data[ 3 * ( width * tileWidth * ( i * tileHeight + y ) + j * tileWidth + x ) + 1 ] = imageData[mosaic[i][j]][3*(y*tileWidth+x)+1];
          data[ 3 * ( width * tileWidth * ( i * tileHeight + y ) + j * tileWidth + x ) + 2 ] = imageData[mosaic[i][j]][3*(y*tileWidth+x)+2];
        }
      }
    }
  }

  VImage::new_from_memory( data, width*height*tileWidth*tileHeight*3, width*tileWidth, height*tileHeight, 3, VIPS_FORMAT_UCHAR ).vipssave((char *)outputImage.c_str());

  // Free the data
  delete [] data;
}

int gcd(int a, int b) {
    return b == 0 ? a : gcd(b, a % b);
}

void buildDeepZoomImage( vector< vector< int > > &mosaic, vector< cropType > cropData, int numUnique, int tileWidth, int tileHeight, string outputImage, ofstream& htmlFile, bool quiet )
{
  // Rotation angles
  VipsAngle rotAngle[4] = {VIPS_ANGLE_D0,VIPS_ANGLE_D90,VIPS_ANGLE_D180,VIPS_ANGLE_D270};

  ostringstream levelData;

  int div = gcd(tileWidth,tileHeight);
  tileWidth /= div;
  tileHeight /= div;

  int multiplier = min(256/tileWidth,256/tileHeight);
  tileWidth *= multiplier;
  tileHeight *= multiplier;

  // Mosaic dimension data
  int width = mosaic[0].size();
  int height = mosaic.size();

  int numHorizontal = width;
  int numVertical = height;

  // Number of levels required to generate
  int maxZoomablesLevel = INT_MIN;
  int minZoomablesLevel = INT_MAX;

  // How many contructed levels there will be
  int level = (int)ceil( log2((width > height) ? width : height ) ) + 6;

  // Used for first pass
  bool first = true;

  // Output zoomable directory name
  string outputDirectory = string( outputImage + "zoom/" );

  // Create output zoomable directory
  g_mkdir(outputDirectory.c_str(), 0777);

  // Vector for output information for html file
  vector< string > mosaicStrings;

  ProgressBar *generatingZoomables = new ProgressBar(numUnique, "Generating zoomables");

  levelData << "var levelData = [";

  // Generate deep zoom images for each used image
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

        // Skip this image if it has already been processed
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

        // If the image has not been processed then convert it to a deep zoom image
        if( !skip )
        {
          // Update that this image has been processed
          alreadyDone.push_back( current );

          // Update the mosaic to have the ID for this image
          mosaic[i][j] = numTiles++;

          // Load image
          VImage image = VImage::vipsload( (char *)(get<0>(cropData[current])).c_str() ).rot(rotAngle[get<3>(cropData[current])]);

          // Find the width of the largest square inside image
          int width = image.width();
          int height = image.height();
          
          double minDim, minTileDim;

          if( (double)width/(double)tileWidth < (double)height/(double)tileHeight )
          {
            minDim = width;
            minTileDim = tileWidth;
          }
          else
          {
            minDim = height;
            minTileDim = tileHeight;
          }

          double size = minDim/minTileDim;

          // Logarithm of square size
          int log = round( log2( size ) );

          // Number of levels to generate
          if( numTiles > 1 ) levelData << ","; 
          levelData << log;

          maxZoomablesLevel = max( maxZoomablesLevel, log );
          minZoomablesLevel = min( minZoomablesLevel, log );

          int newSize = 1 << log;

          // Build the zoomable image if necessary 
          string zoomableName = outputDirectory + to_string(mosaic[i][j]);

          // Extract square, flip, and rotate based on cropdata, and then save as a deep zoom image
          image = image.extract_area(get<1>(cropData[current]), get<2>(cropData[current]), ceil(minDim*tileWidth/minTileDim), ceil(minDim*tileHeight/minTileDim)).resize((double)newSize*minTileDim/minDim);

          if( get<4>(cropData[current]) )
          {
            image = image.flip(VIPS_DIRECTION_HORIZONTAL);
          }

          g_mkdir(string(zoomableName).append("_files/").c_str(), 0777);
          g_mkdir(string(zoomableName).append("_files/").append(to_string(log)).c_str(), 0777);

          for( int y = 0; y < newSize; ++y )
          {
            for( int x = 0; x < newSize; ++x )
            {
              image.extract_area(x*tileWidth,y*tileHeight,tileWidth,tileHeight).jpegsave((char *)string(zoomableName).append("_files/"+to_string(log)+"/"+to_string(x)+"_"+to_string(y)+".jpeg").c_str(),VImage::option()->set( "optimize_coding", true )->set( "strip", true ));
            }
          }

          for( ; log > 0; --log )
          {
            string current = string(zoomableName).append("_files/"+to_string(log-1)+"/");
            string upper = string(zoomableName).append("_files/"+to_string(log)+"/");

            newSize = 1 << log;

            g_mkdir((char *)current.c_str(), 0777);

            for(int i = 1; i < newSize; i+=2)
            {
              for(int j = 1; j < newSize; j+=2)
              {
                (VImage::jpegload((char *)string(upper).append(to_string(i-1)+"_"+to_string(j-1)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
                join(VImage::jpegload((char *)string(upper).append(to_string(i)+"_"+to_string(j-1)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL)).
                join(VImage::jpegload((char *)string(upper).append(to_string(i-1)+"_"+to_string(j)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
                join(VImage::jpegload((char *)string(upper).append(to_string(i)+"_"+to_string(j)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL),VIPS_DIRECTION_VERTICAL).
                jpegsave((char *)string(current).append(to_string(i>>1)+"_"+to_string(j>>1)+".jpeg").c_str() );
              }
            }
          }

          if( !quiet ) generatingZoomables->Increment();
        }
        // Otherwise set the mosaic value to the ID for the image
        else
        {
          mosaic[i][j] = k;
        }
        // Save the mosaic ID for the html file
        currentMosaic << mosaic[i][j];
        // Put a comma if there are more
        if( j < width - 1 ) currentMosaic << ","; 
      }
      currentMosaic << "]";
      if( i < height - 1 ) currentMosaic << ",";
    }
    currentMosaic << "]";
    mosaicStrings.push_back(currentMosaic.str());
  }

  if( !quiet ) generatingZoomables->Finish();

  ProgressBar *generatingLevels = new ProgressBar(height, "Generating levels");

  levelData << "];";

  // Grab four tiles, join them together, shrink by two in each dimension until the output is only a pixel big
  for( ; level >= 0; --level )
  {
    // Vector for whether these four images have been processed yet
    vector< vector< int > > alreadyDone;
    int numTiles = 0;

    ostringstream image, directory, input, currentMosaic;

    // Make the directory for the zoomable image
    image << outputImage << level << "/";
    input << outputImage << level + 1 << "/";
    directory << outputDirectory;
    g_mkdir(image.str().c_str(), 0777);

    // Go through every 2 by 2 images
    for( int i = 1; i < height; i+=2 )
    {
      for( int j = 1; j < width; j+=2 )
      {
        // Vector of current images
        vector< int > current = { mosaic[i-1][j-1], mosaic[i-1][j], mosaic[i][j-1], mosaic[i][j] };

        // Check whether 4 images have been processed yet
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

        // Join four images, shrink my two, and save
        if( !skip )
        {
          alreadyDone.push_back( current );

          // If first pass load from zoomable image directory
          if( first )
          {
            (VImage::jpegload((char *)directory.str().append(to_string(mosaic[i-1][j-1])+"_files/0/0_0.jpeg").c_str(), VImage::option()->set( "shrink", 2)).
            join(VImage::jpegload((char *)directory.str().append(to_string(mosaic[i-1][j])+"_files/0/0_0.jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL)).
            join(VImage::jpegload((char *)directory.str().append(to_string(mosaic[i][j-1])+"_files/0/0_0.jpeg").c_str(), VImage::option()->set( "shrink", 2)).
            join(VImage::jpegload((char *)directory.str().append(to_string(mosaic[i][j])+"_files/0/0_0.jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL),VIPS_DIRECTION_VERTICAL).
            jpegsave((char *)image.str().append(to_string(numTiles)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );
          }
          // Otherwise load from tile level directory
          else
          {
            (VImage::jpegload((char *)input.str().append(to_string(mosaic[i-1][j-1])+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
            join(VImage::jpegload((char *)input.str().append(to_string(mosaic[i-1][j])+".jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL)).
            join(VImage::jpegload((char *)input.str().append(to_string(mosaic[i][j-1])+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
            join(VImage::jpegload((char *)input.str().append(to_string(mosaic[i][j])+".jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL),VIPS_DIRECTION_VERTICAL).
            jpegsave((char *)image.str().append(to_string(numTiles)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );
          }

          // The mosaic shrinks as well
          mosaic[i/2][j/2] = numTiles++;
        }
        else
        {
          // Save the ID of the image if it has been shrunk already
          mosaic[i/2][j/2] = k;
        }
      }
      if( !quiet ) generatingLevels->Increment();
    }

    // Edge cases with only two images
    if(width%2 == 1)
    {
      // Clear list of images already processed incase edge cases create incorrect duplicates
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

    // Edge case for bottom corner
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

  if( !quiet ) generatingLevels->Finish();

  htmlFile << levelData.str() << endl;

  htmlFile << "var data = [ \n";

  for( int i = mosaicStrings.size()-1; i>=0; --i )
  {
    htmlFile << mosaicStrings[i] << "," << endl;
  }

  htmlFile << "];";

  htmlFile << "\nvar mosaicTileWidth = " << tileWidth << ";\nvar mosaicTileHeight = " << tileHeight << ";\nvar mosaicWidth = " << numHorizontal << ";\nvar mosaicHeight = " << numVertical << ";\nvar mosaicLevel = " << mosaicStrings.size()-1 << ";\nvar minZoomablesLevel = " << minZoomablesLevel << ";\nvar maxZoomablesLevel = " << maxZoomablesLevel << ";\n";
}

void RunMosaic( string inputName, string outputName, vector< string > inputDirectory, int numHorizontal, bool trueColor, int cropStyle, bool flip, bool spin, int mosaicTileWidth, int mosaicTileHeight, int imageTileWidth, int repeat, string fileName, bool quiet, bool recursiveSearch )
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
  vector< vector< unsigned char > > mosaicTileData;
  vector< vector< unsigned char > > imageTileData;

  bool loadData = (fileName != " ");

  if( loadData )
  {
    ifstream data( fileName, ios::binary );

    if(data.is_open())
    {
      data.read( (char *)&numImages, sizeof(int) );
      data.read( (char *)&mosaicTileWidth, sizeof(int) );
      data.read( (char *)&mosaicTileHeight, sizeof(int) );
      data.read( (char *)&imageTileWidth, sizeof(int) );
      data.read( (char *)&imageTileHeight, sizeof(int) );

      for( int i = 0; i < numImages; ++i )
      {
        int strLen, cropX, cropY, rot;
        bool flip;
        string str;
        data.read((char *)&strLen, sizeof(int));
        char* temp = new char[strLen+1];
        data.read(temp, strLen);
        temp[strLen] = '\0';
        str = temp;
        delete [] temp;
        data.read((char *)&cropX, sizeof(int));
        data.read((char *)&cropY, sizeof(int));
        data.read((char *)&rot, sizeof(int));
        data.read((char *)&flip, sizeof(bool));

        cropData.push_back(make_tuple(str,cropX,cropY,rot,flip));
      }
    
      mosaicTileData.resize( numImages );
      
      for( int i = 0; i < numImages; ++i )
      {
        mosaicTileData[i].resize( mosaicTileWidth*mosaicTileHeight*3 );
        data.read((char *)mosaicTileData[i].data(), mosaicTileData[i].size()*sizeof(unsigned char));
      }

      if( imageTileHeight != mosaicTileHeight )
      {
        imageTileData.resize( numImages );
        
        for( int i = 0; i < numImages; ++i )
        {
          imageTileData[i].resize( imageTileWidth*imageTileHeight*3 );
          data.read((char *)imageTileData[i].data(), imageTileData[i].size()*sizeof(unsigned char));
        } 
      }
    }
    else
    {
      loadData = false;
    }
  }
  if( !loadData )
  {
    for( int i = 0; i < inputDirectory.size(); ++i )
    {
      string imageDirectory = inputDirectory[i];
      if( imageDirectory.back() != '/' ) imageDirectory += '/';
      generateThumbnails( cropData, mosaicTileData, imageTileData, inputDirectory, imageDirectory, mosaicTileWidth, mosaicTileHeight, imageTileWidth, imageTileHeight, isDeepZoom, spin, cropStyle, flip, quiet, recursiveSearch );
    }

    numImages = mosaicTileData.size();

    if( numImages == 0 ) 
    {
      cout << "No valid images found. The directory might not contain any images, or they might be too small, or they might not be a valid format." << endl;
      return;
    }

    if( fileName != " " )
    {
      ofstream data( fileName, ios::binary );
      data.write( (char *)&numImages, sizeof(int) );

      data.write( (char *)&mosaicTileWidth, sizeof(int) );

      data.write( (char *)&mosaicTileHeight, sizeof(int) );

      data.write( (char *)&imageTileWidth, sizeof(int) );

      data.write( (char *)&imageTileHeight, sizeof(int) );

      for( int i = 0; i < numImages; ++i )
      {
        string str = get<0>(cropData[i]);
        int strLen = str.length();
        data.write((char *)&strLen,sizeof(strLen));
        data.write((char *)str.c_str(), strLen);
        data.write((char *)&get<1>(cropData[i]),sizeof(int));
        data.write((char *)&get<2>(cropData[i]),sizeof(int));
        data.write((char *)&get<3>(cropData[i]),sizeof(int));
        data.write((char *)&get<4>(cropData[i]),sizeof(bool));
      }

      for( int i = 0; i < numImages; ++i )
      {
        data.write((char *)&mosaicTileData[i].front(), mosaicTileData[i].size() * sizeof(unsigned char)); 
      }

      if( imageTileWidth != mosaicTileWidth )
      {
        for( int i = 0; i < numImages; ++i )
        {
          data.write((char *)&imageTileData[i].front(), imageTileData[i].size() * sizeof(unsigned char)); 
        }
      }
    }
  }

  int tileArea = mosaicTileWidth*mosaicTileHeight*3;
  int numVertical = int( (double)height / (double)width * (double)numHorizontal * (double)mosaicTileWidth/(double)mosaicTileHeight );
  int numUnique = 0;

  vector< vector< int > > d;
  vector< vector< float > > lab;

  if( trueColor  )
  {
    d.push_back( vector< int >(tileArea) );
    lab = vector< vector< float > >( numImages, vector< float >(tileArea) );

    float r,g,b;

    for( int j = 0; j < numImages; ++j )
    {
      for( int p = 0; p < tileArea; p+=3 )
      {
        vips_col_sRGB2scRGB_16(mosaicTileData[j][p],mosaicTileData[j][p+1],mosaicTileData[j][p+2], &r,&g,&b );
        vips_col_scRGB2XYZ( r, g, b, &r, &g, &b );
        vips_col_XYZ2Lab( r, g, b, &lab[j][p], &lab[j][p+1], &lab[j][p+2] );
      }
    }
  }
  else
  {
    d = vector< vector< int > >( numImages, vector< int >(tileArea) );

    for( int j = 0; j < numImages; ++j )
    {
      for( int p = 0; p < tileArea; ++p )
      {
        d[j][p] = mosaicTileData[j][p];
      }
    }
  }

  if( inputIsDirectory )
  {
    g_mkdir(outputName.c_str(), 0777);
  }

  my_kd_tree_t mat_index(tileArea, d, 10 );

  for( int i = 0; i < inputImages.size(); ++i )
  {
    vector< vector< int > > mosaic( numVertical, vector< int >( numHorizontal, -1 ) );

    int sqrtThreads = ceil(sqrt(sysconf(_SC_NPROCESSORS_ONLN)));

    ProgressBar *buildingMosaic = new ProgressBar((numVertical/sqrtThreads)*(numHorizontal/sqrtThreads), "Building mosaic");

    if( trueColor  )
    {
      numUnique = generateMosaic( lab, mosaic, inputImages[i], buildingMosaic, repeat, false, numHorizontal * mosaicTileWidth, quiet );
    }
    else
    {
      numUnique = generateMosaic( mat_index, mosaic, inputImages[i], buildingMosaic, repeat, false, numHorizontal * mosaicTileWidth, quiet );
    }

    if( !quiet ) buildingMosaic->Finish();

    if( isDeepZoom )
    {
      if( outputImages[i].back() != '/' ) outputImages[i] += '/';

      g_mkdir(outputImages[i].c_str(), 0777);

      ofstream htmlFile(outputImages[i].substr(0, outputImages[i].size()-1).append(".html").c_str());

      htmlFile << "<!DOCTYPE html>\n<html>\n<head><script src=\"js/openseadragon.min.js\"></script></head>\n<body>\n<style>\nhtml,\nbody,\n#mosaic\n{\nposition: fixed;\nleft: 0;\ntop: 0;\nwidth: 100%;\nheight: 100%;\n}\n</style>\n\n<div id=\"mosaic\" ></div>\n<script type=\"text/javascript\">\nvar outputName = \"" << outputImages[i] << "\";\nvar outputDirectory = \"" << outputImages[i] << "zoom/\";\n";

      buildDeepZoomImage( mosaic, cropData, numUnique, mosaicTileWidth, mosaicTileHeight, outputImages[i], htmlFile, quiet );

      htmlFile << "</script>\n<script src=\"js/mosaic.js\"></script>\n</body>\n</html>";

      htmlFile.close();
    }
    else
    {
      if( mosaicTileWidth == imageTileWidth )
      {
        buildImage( mosaicTileData, mosaic, outputImages[i], mosaicTileWidth, mosaicTileHeight );
      }
      else
      {
        buildImage( imageTileData, mosaic, outputImages[i], imageTileWidth, imageTileHeight );
      }
    }
  }
}