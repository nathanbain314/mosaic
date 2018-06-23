#include "Tessellate.h"

void buildTopLevel( string outputImage, int start, int end, int outputWidth, int outputHeight, vector< vector< cropType > > &cropData, vector< int > &mosaic, vector< vector< int > > mosaicLocations, vector< vector< int > > shapeIndices, vector< int > tileWidth2, vector< int > tileHeight2, ProgressBar *topLevel )
{
  // Rotation angles
  VipsAngle rotAngle[4] = {VIPS_ANGLE_D0,VIPS_ANGLE_D90,VIPS_ANGLE_D180,VIPS_ANGLE_D270};

  for( int tileOffsetY = start; tileOffsetY < end; tileOffsetY += 256 )
  {
    int tileHeight = min( outputHeight - tileOffsetY, 256 );// + ( tileOffsetY > 0 && tileOffsetY + 256 < outputHeight );

    for( int tileOffsetX = 0; tileOffsetX < outputWidth; tileOffsetX += 256 )
    {
      if( start == 0 ) topLevel->Increment();

      int tileWidth = min( outputWidth - tileOffsetX, 256 );// + ( tileOffsetX > 0 && tileOffsetX + 256 < outputWidth );

      unsigned char * tileData = ( unsigned char * )calloc (tileHeight*tileWidth*3,sizeof(unsigned char));

      for( int k = 0; k < mosaicLocations.size(); ++k )
      {
        int j = mosaicLocations[k][0];
        int i = mosaicLocations[k][1];
        int j2 = mosaicLocations[k][2];
        int i2 = mosaicLocations[k][3];
        int t = mosaicLocations[k][4];

        if( j >= tileOffsetX + tileWidth || j2 < tileOffsetX || i >= tileOffsetY + tileHeight || i2 < tileOffsetY ) continue;

        int current = mosaic[k];

        VImage image = VImage::vipsload( (char *)(get<0>(cropData[t][current])).c_str() ).rot(rotAngle[get<3>(cropData[t][current])]);

        // Convert to a three band image
        if( image.bands() == 1 )
        {
          image = image.bandjoin(image).bandjoin(image);
        }
        if( image.bands() == 4 )
        {
          image = image.flatten();
        }

        // Find the width of the largest square inside image
        int width = image.width();
        int height = image.height();
        
        double newSize = max((double)tileWidth2[t]/(double)width,(double)tileHeight2[t]/(double)height);

        // Extract square, flip, and rotate based on cropdata, and then save as a deep zoom image
        image = image.resize(newSize).extract_area(get<1>(cropData[t][current])*newSize, get<2>(cropData[t][current])*newSize, tileWidth2[t], tileHeight2[t]);

        if( get<4>(cropData[t][current]) )
        {
          image = image.flip(VIPS_DIRECTION_HORIZONTAL);
        }

        unsigned char *data = (unsigned char *)image.data();

        for( int p = 0; p < shapeIndices[t].size(); ++p )
        {
          int x = j + shapeIndices[t][p]%tileWidth2[t] - tileOffsetX;
          int y = i + shapeIndices[t][p]/tileWidth2[t] - tileOffsetY;

          if( !(x >= 0 && x < tileWidth && y >= 0 && y < tileHeight) ) continue;

          int l = 3 * ( y * tileWidth + x );

          tileData[l+0] = data[3*shapeIndices[t][p]+0];
          tileData[l+1] = data[3*shapeIndices[t][p]+1];
          tileData[l+2] = data[3*shapeIndices[t][p]+2];
        }
      }

      VImage::new_from_memory( tileData, tileHeight*tileWidth*3, tileWidth, tileHeight, 3, VIPS_FORMAT_UCHAR ).jpegsave((char *)string(outputImage).append(to_string(tileOffsetX/256)+"_"+to_string(tileOffsetY/256)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );
      
      free( tileData );
    }
  }
}

void buildImage( vector< vector< cropType > > &cropData, vector< int > &mosaic, vector< vector< int > > mosaicLocations, vector< vector< int > > shapeIndices, string outputImage, vector< int > tileWidth, vector< int > tileHeight, int outputWidth, int outputHeight )
{
  // Rotation angles
  VipsAngle rotAngle[4] = {VIPS_ANGLE_D0,VIPS_ANGLE_D90,VIPS_ANGLE_D180,VIPS_ANGLE_D270};

  // Create list of numbers in thread block
  vector< int > indices( mosaicLocations.size() );
  iota( indices.begin(), indices.end(), 0 );

  // Shuffle the points so that patterns do not form
  shuffle( indices.begin(), indices.end(), default_random_engine(time(NULL)) );

  string str = "Generating image " + outputImage;

  ProgressBar *generatingImage = new ProgressBar(mosaicLocations.size(), (char *)str.c_str());

  // Create output image data 
  unsigned char *data = ( unsigned char * )calloc (outputHeight*outputWidth*3,sizeof(unsigned char));//new unsigned char[outputWidth*outputHeight*3];

  for( int k = 0; k < mosaicLocations.size(); ++k )
  {
    int j = mosaicLocations[k][0];
    int i = mosaicLocations[k][1];
    int t = mosaicLocations[k][4];
    
    int current = mosaic[k];

    VImage image = VImage::vipsload( (char *)(get<0>(cropData[t][current])).c_str() ).rot(rotAngle[get<3>(cropData[t][current])]);

    // Convert to a three band image
    if( image.bands() == 1 )
    {
      image = image.bandjoin(image).bandjoin(image);
    }
    if( image.bands() == 4 )
    {
      image = image.flatten();
    }

    // Find the width of the largest square inside image
    int width = image.width();
    int height = image.height();
    
    double newSize = max((double)tileWidth[t]/(double)width,(double)tileHeight[t]/(double)height);

    // Extract square, flip, and rotate based on cropdata, and then save as a deep zoom image
    image = image.resize(newSize).extract_area(get<1>(cropData[t][current])*newSize, get<2>(cropData[t][current])*newSize, tileWidth[t], tileHeight[t]);

    if( get<4>(cropData[t][current]) )
    {
      image = image.flip(VIPS_DIRECTION_HORIZONTAL);
    }

    unsigned char *tileData = (unsigned char *)image.data();

    for( int x = 0; x < shapeIndices[t].size(); ++x )
    {
      int l = 3 * ( ( i + shapeIndices[t][x]/tileWidth[t] ) * outputWidth + j + shapeIndices[t][x]%tileWidth[t] );

      data[l+0] = tileData[3*shapeIndices[t][x]+0];
      data[l+1] = tileData[3*shapeIndices[t][x]+1];
      data[l+2] = tileData[3*shapeIndices[t][x]+2];
    }

    generatingImage->Increment();
  }

  generatingImage->Finish();

  cout << "Saving image " << outputImage << endl;

  VImage::new_from_memory( data, outputWidth*outputHeight*3, outputWidth, outputHeight, 3, VIPS_FORMAT_UCHAR ).vipssave((char *)outputImage.c_str());

  // Free the data
  delete [] data;
}

void buildImage2( vector< vector< cropType > > &cropData, vector< int > &mosaic, vector< vector< int > > mosaicLocations, vector< vector< int > > shapeIndices, string outputImage, vector< int > tileWidth, vector< int > tileHeight, int outputWidth, int outputHeight )
{
  int level = (int)ceil(log2( max(outputWidth,outputHeight) ) );

  if( outputImage.back() == '/' ) outputImage = outputImage.substr(0, outputImage.size()-1);

  g_mkdir(string(outputImage).append("_files/").c_str(), 0777);
  g_mkdir(string(outputImage).append("_files/").append(to_string(level)).c_str(), 0777);

  int threads = sysconf(_SC_NPROCESSORS_ONLN);

  ProgressBar *topLevel = new ProgressBar(ceil((double)outputWidth/256.0)*ceil((double)outputHeight/((double)threads*256.0)), "Building top level");

  future< void > ret[threads];

  for( int k = 0; k < threads; ++k )
  {
    int start = k*outputHeight/threads;
    int end = (k+1)*outputHeight/threads;

    start = (start / 256 ) * 256;
    if( k+1 < threads ) end = (end / 256 ) * 256;

    ret[k] = async( launch::async, &buildTopLevel, string(outputImage).append("_files/"+to_string(level)+"/"), start, end, outputWidth, outputHeight, ref(cropData), ref(mosaic), ref(mosaicLocations), ref(shapeIndices), ref(tileWidth), ref(tileHeight), topLevel );
  }

  // Wait for threads to finish
  for( int k = 0; k < threads; ++k )
  {
    ret[k].get();
  }
  
  topLevel->Finish();

  ofstream dzi_file;
  dzi_file.open(string(outputImage).append(".dzi").c_str());
  dzi_file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
  dzi_file << "<Image xmlns=\"http://schemas.microsoft.com/deepzoom/2008\" Format=\"jpeg\" Overlap=\"0\" TileSize=\"256\">" << endl;
  dzi_file << "    <Size Height=\"" << outputHeight << "\" Width=\"" << outputWidth << "\"/>" << endl;
  dzi_file << "</Image>" << endl;
  dzi_file.close();

  int numLevels = 0;
  for( int o = ceil((double)outputWidth/256.0); o > 1; o = ceil((double)o/2.0) ) numLevels += o;

  ProgressBar *lowerLevels = new ProgressBar(numLevels, "Building lower levels");

  outputWidth = (int)ceil((double)outputWidth/ 128.0 );
  outputHeight = (int)ceil((double)outputHeight/ 128.0 );

  for( ; level > 0; --level )
  {
    outputWidth = (int)ceil((double)outputWidth/ 2.0 );
    outputHeight = (int)ceil((double)outputHeight/ 2.0 );

    string current = string(outputImage).append("_files/"+to_string(level-1)+"/");
    string upper = string(outputImage).append("_files/"+to_string(level)+"/");

    g_mkdir((char *)current.c_str(), 0777);

    for(int i = 1; i < outputWidth; i+=2)
    {
      lowerLevels->Increment();

      for(int j = 1; j < outputHeight; j+=2)
      {
        (VImage::jpegload((char *)string(upper).append(to_string(i-1)+"_"+to_string(j-1)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
        join(VImage::jpegload((char *)string(upper).append(to_string(i)+"_"+to_string(j-1)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL)).
        join(VImage::jpegload((char *)string(upper).append(to_string(i-1)+"_"+to_string(j)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
        join(VImage::jpegload((char *)string(upper).append(to_string(i)+"_"+to_string(j)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL),VIPS_DIRECTION_VERTICAL).
        jpegsave((char *)string(current).append(to_string(i>>1)+"_"+to_string(j>>1)+".jpeg").c_str() );
      }
    }
    if(outputWidth%2 == 1)
    {
      for(int j = 1; j < outputHeight; j+=2)
      {
        (VImage::jpegload((char *)string(upper).append(to_string(outputWidth-1)+"_"+to_string(j-1)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
        join(VImage::jpegload((char *)string(upper).append(to_string(outputWidth-1)+"_"+to_string(j)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_VERTICAL)).
        jpegsave((char *)string(current).append(to_string(outputWidth>>1)+"_"+to_string(j>>1)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );
      }
    }
    if(outputHeight%2 == 1)
    {
      for(int j = 1; j < outputWidth; j+=2)
      {
        (VImage::jpegload((char *)string(upper).append(to_string(j-1)+"_"+to_string(outputHeight-1)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
        join(VImage::jpegload((char *)string(upper).append(to_string(j)+"_"+to_string(outputHeight-1)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL)).
        jpegsave((char *)string(current).append(to_string(j>>1)+"_"+to_string(outputHeight>>1)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );
      }
    }
    if(outputWidth%2 == 1 && outputHeight%2 == 1)
    {
      VImage::jpegload((char *)string(upper).append(to_string(outputWidth-1)+"_"+to_string(outputHeight-1)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
      jpegsave((char *)string(current).append(to_string(outputWidth>>1)+"_"+to_string(outputHeight>>1)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );  
    }
  }

  lowerLevels->Finish();

  // Generate html file to view deep zoom image
  ofstream htmlFile(string(outputImage).append(".html").c_str());
  htmlFile << "<!DOCTYPE html>\n<html>\n<head><script src=\"js/openseadragon.min.js\"></script></head>\n<body>\n<style>\nhtml,\nbody,\n#collage\n{\nposition: fixed;\nleft: 0;\ntop: 0;\nwidth: 100%;\nheight: 100%;\n}\n</style>\n\n<div id=\"collage\"></div>\n\n<script>\nvar viewer = OpenSeadragon({\nid: 'collage',\nprefixUrl: 'icons/',\ntileSources:   \"" + outputImage + ".dzi\",\nminZoomImageRatio: 0,\nmaxZoomImageRatio: 1\n});\n</script>\n</body>\n</html>";
  htmlFile.close();
}

int gcd(int a, int b) {
    return b == 0 ? a : gcd(b, a % b);
}

int generateRGBBlock( vector< unique_ptr< my_kd_tree_t > > &mat_index, vector< int > &mosaic, vector< vector< int > > &mosaicLocations, vector< vector< int > > shapeIndices, vector< int > &indices, int repeat, unsigned char * c, int start, int end, vector< int > tileWidth, int width, ProgressBar* buildingMosaic, bool quiet )
{
  // Whether to update progressbar
  bool show = !quiet && !(start);

  // Color difference of images
  int out_dist_sqr;

  // For every index
  for( int p = start; p < end; ++p )
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

  // Resize the image for correct mosaic tile size
  if( resize != 0 ) image = image.resize( (double)resize / (double)(image.width()) );

  int width = image.width();
  int height = image.height();

  // Get image data
  unsigned char * c = ( unsigned char * )image.data();

  // Number of threads for multithreading: needs to be a square number
  int threads = sysconf(_SC_NPROCESSORS_ONLN);

  // Create list of numbers in thread block
  vector< int > indices( mosaicLocations.size() );
  iota( indices.begin(), indices.end(), 0 );

  // Shuffle the points so that patterns do not form
  shuffle( indices.begin(), indices.end(), default_random_engine(time(NULL)) );

  // Break mosaic into blocks and find best matching tiles inside each block on a different thread
  future< int > ret[threads];

  for( int i = 0; i < threads; ++i )
  {
    ret[i] = async( launch::async, &generateRGBBlock, ref(mat_index), ref(mosaic), ref(mosaicLocations), ref(shapeIndices), ref(indices), repeat, c, i*indices.size()/threads, (i+1)*indices.size()/threads, tileWidth, width, buildingMosaic, quiet );
  }

  // Wait for threads to finish
  for( int k = 0; k < threads; ++k )
  {
    ret[k].get();
  }

  return 0;
}

void createLocations( vector< vector< int > > &mosaicLocations, vector< vector< int > > &mosaicLocations2, vector< vector< double > > &offsets, vector< vector< double > > &dimensions, int mosaicTileWidth, int mosaicTileHeight, int imageTileWidth, int imageTileHeight, int width, int height )
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

    double tileWidth = w * dimensions[offsets[i][4]][0];
    double tileHeight = h * dimensions[offsets[i][4]][1];

    double tileWidth2 = w2 * dimensions[offsets[i][4]][0];
    double tileHeight2 = h2 * dimensions[offsets[i][4]][1];

    for( double ySign = -1; ySign <= 1; ySign += 2 )
    {
      for( double xSign = -1; xSign <= 1; xSign += 2 )
      {
        double startX = w * offsets[i][0] + tileWidth;
        double startY = h * offsets[i][1] + tileHeight;

        double startX2 = w2 * offsets[i][0] + tileWidth2;
        double startY2 = h2 * offsets[i][1] + tileHeight2;

        for( double y = startY, y2 = startY2; (ySign > 0 && startY < height) || (ySign < 0 && startY > -height); startY += ySign*yVerticalOffset, startY2 += ySign*yVerticalOffset2, startX += ySign*xVerticalOffset, startX2 += ySign*xVerticalOffset2)// y += yOffset, y2 += yOffset2 )
        {
          for( double x = startX, x2 = startX2, y = startY, y2 = startY2; (xSign > 0 && x < width) || (xSign < 0 && x > -width); x += xSign*xHorizontalOffset, x2 += xSign*xHorizontalOffset2, y += xSign*yHorizontalOffset, y2 += xSign*yHorizontalOffset2 )
          {
            if( x - tileWidth >= 0 && y - tileHeight >= 0 && x < width && y < height )
            {
              mosaicLocations.push_back({ (int)round(x-tileWidth), (int)round(y-tileHeight), (int)round(x), (int)round(y), int(offsets[i][6]) } );

              mosaicLocations2.push_back({ (int)round(x2-tileWidth2), (int)round(y2-tileHeight2), (int)round(x2), (int)round(y2), int(offsets[i][6]) } );
            }
          }
        }
      }
    }
  }
}

void RunTessellate( string inputName, string outputName, vector< string > inputDirectory, int numHorizontal, bool trueColor, int cropStyle, bool flip, bool spin, int mosaicTileWidth, int mosaicTileHeight, int imageTileWidth, int repeat, string fileName, bool quiet, bool recursiveSearch, string tessellateType )
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
  if( imageTileWidth == 0 ) imageTileWidth = mosaicTileWidth;
  if( mosaicTileHeight == 0 ) mosaicTileHeight = mosaicTileWidth;

  int imageTileHeight = imageTileWidth*mosaicTileHeight/mosaicTileWidth;

  int numImages;
  vector< vector< cropType > > cropData;
  vector< vector< vector< unsigned char > > > mosaicTileData;
  vector< vector< vector< unsigned char > > > imageTileData;

  bool loadData = (fileName != " ");

  vector< vector< double > > dimensions;

  vector< vector< vector< double > > > shapes;

  vector< vector< double > > offsets;

  ifstream tessellationFile( tessellateType );

  string str;

  while( getline(tessellationFile, str) && str != "dimensions:" );

  while( getline(tessellationFile, str) && str != "" )
  {
    istringstream iss(str);

    double w, h;

    iss >> w >> h;

    dimensions.push_back(vector< double >({w,h}));
  }

  shapes.resize( dimensions.size() );

  while( getline(tessellationFile, str) && str != "shapes:" );

  for( int i = 0; i < dimensions.size(); ++i )
  {
    while( getline(tessellationFile, str) && str != "shape:" );

    while( getline(tessellationFile, str) && str != "" )
    {
      istringstream iss(str);

      double x1, x2, by1, by2, ty1, ty2;

      iss >> x1 >> x2 >> by1 >> by2 >> ty1 >> ty2;

      shapes[i].push_back(vector< double >({x1, x2, by1, by2, ty1, ty2}));
    }
  }

  while( getline(tessellationFile, str) && str != "offsets:" );

  while( getline(tessellationFile, str) && str != "" )
  {
    istringstream iss(str);

    double x, y, xHorizontalOffset, yHorizontalOffset, xVerticalOffset, yVerticalOffset, shape;

    iss >> x >> y >> xHorizontalOffset >> yHorizontalOffset >> xVerticalOffset >> yVerticalOffset >> shape;

    offsets.push_back(vector< double >({x, y, xHorizontalOffset, yHorizontalOffset, xVerticalOffset, yVerticalOffset, shape}));
  }

  vector< int > tileWidth, tileHeight, tileWidth2, tileHeight2;

  for( int i = 0; i < dimensions.size(); ++i )
  {
    tileWidth.push_back((double)mosaicTileWidth*dimensions[i][0]);
    tileHeight.push_back((double)mosaicTileHeight*dimensions[i][1]);

    tileWidth2.push_back((double)imageTileWidth*dimensions[i][0]);
    tileHeight2.push_back((double)imageTileHeight*dimensions[i][1]);
  }

  if( !loadData )
  {
    mosaicTileData.resize(dimensions.size());

    cropData.resize(dimensions.size());

    if( mosaicTileWidth != imageTileWidth ) imageTileData.resize(dimensions.size());

    vector< string > inputDirectoryBlank;

    for( int i = 0; i < inputDirectory.size(); ++i )
    {
      string imageDirectory = inputDirectory[i];
      if( imageDirectory.back() != '/' ) imageDirectory += '/';
      for( int j = 0; j < dimensions.size(); ++j )
      {
        generateThumbnails( cropData[j], mosaicTileData[j], imageTileData[j], j == 0 ? inputDirectory : inputDirectoryBlank, imageDirectory, tileWidth[j], tileHeight[j], tileWidth[j], tileHeight[j], isDeepZoom, spin, cropStyle, flip, quiet, recursiveSearch );
      }
    }

    for( int j = 0; j < dimensions.size(); ++j )
    {
      numImages = mosaicTileData[j].size();

      if( numImages == 0 ) 
      {
        cout << "No valid images found. The directory might not contain any images, or they might be too small, or they might not be a valid format." << endl;
        return;
      }
    }
  }

  vector< Tessellation > tessellations;

  vector< vector< int > > shapeIndices(shapes.size()), shapeIndices2(shapes.size()), mosaicLocations, mosaicLocations2;

  for( int i = 0; i < shapes.size(); ++i )
  {
    Tessellation tess(shapes[i]);
    tess.createIndices( shapeIndices[i], tileWidth[i], tileHeight[i] );

    if( mosaicTileWidth != imageTileWidth )
    {
      tess.createIndices( shapeIndices2[i], tileWidth2[i], tileHeight2[i] );
    }
  }

  int tileArea = mosaicTileWidth*mosaicTileHeight*3;
  int numVertical = int( (double)height / (double)width * (double)numHorizontal * (double)mosaicTileWidth/(double)mosaicTileHeight );
  int numUnique = 0;

  createLocations( mosaicLocations, mosaicLocations2, offsets, dimensions, mosaicTileWidth, mosaicTileHeight, imageTileWidth, imageTileHeight, numHorizontal * mosaicTileWidth, numVertical * mosaicTileHeight );

  int imageWidth = 0, imageHeight = 0;

  for( int i = 0; i < mosaicLocations2.size(); ++i )
  {
    imageWidth = max(imageWidth,mosaicLocations2[i][2]);
    imageHeight = max(imageHeight,mosaicLocations2[i][3]);
  }

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

    int threads = sysconf(_SC_NPROCESSORS_ONLN);

    ProgressBar *buildingMosaic = new ProgressBar(mosaicLocations.size()/threads, "Building mosaic");

    if( trueColor  )
    {
    }
    else
    {
      numUnique = generateMosaic( mat_index, mosaicLocations, shapeIndices, mosaic, inputImages[i], buildingMosaic, repeat, false, numHorizontal * mosaicTileWidth, quiet, tileWidth );
    }

    if( !quiet ) buildingMosaic->Finish();

    if( mosaicTileWidth == imageTileWidth )
    {
      //buildImage( mosaicTileData, mosaic, mosaicLocations, shapeIndices, outputImages[i], tileWidth, width, height );
    }
    else
    {
      // Save image as static image or zoomable image
      if( vips_foreign_find_save( outputImages[i].c_str() ) != NULL )
      {
        buildImage( cropData, mosaic, mosaicLocations2, shapeIndices2, outputImages[i], tileWidth2, tileHeight2, imageWidth, imageHeight );
      }
      else
      {
        buildImage2( cropData, mosaic, mosaicLocations2, shapeIndices2, outputImages[i], tileWidth2, tileHeight2, imageWidth, imageHeight );
      }
    }
  }
}