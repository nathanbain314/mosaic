#include "Tessellate.h"
#include "shapeIndices.h"
#include "mosaicLocations.h"

void buildTopLevel( string outputImage, int start, int end, int outputWidth, int outputHeight, vector< vector< cropType > > &cropData, vector< int > &mosaic, vector< vector< int > > &mosaicLocations, vector< vector< int > > &edgeLocations, vector< vector< int > > &shapeIndices, vector< int > &tileWidth2, vector< int > &tileHeight2, ProgressBar *topLevel, int maxTileWidth, int maxTileHeight, bool singleImage )
{
  // Rotation angles
  VipsAngle rotAngle[4] = {VIPS_ANGLE_D0,VIPS_ANGLE_D90,VIPS_ANGLE_D180,VIPS_ANGLE_D270};

  for( int tileOffsetY = start; tileOffsetY < end; tileOffsetY += maxTileHeight )
  {
    int tileHeight = min( outputHeight - tileOffsetY, maxTileHeight );

    for( int tileOffsetX = 0; tileOffsetX < outputWidth; tileOffsetX += maxTileWidth )
    {
      if( start == 0 && !singleImage ) topLevel->Increment();

      int tileWidth = min( outputWidth - tileOffsetX, maxTileWidth );

      unsigned char * tileData = ( unsigned char * )calloc (tileHeight*tileWidth*3,sizeof(unsigned char));

      for( int k = 0; k < mosaicLocations.size(); ++k )
      {
        if( singleImage ) topLevel->Increment();

        int j = mosaicLocations[k][0];
        int i = mosaicLocations[k][1];
        int j2 = mosaicLocations[k][2];
        int i2 = mosaicLocations[k][3];
        int t = mosaicLocations[k][4];

        if( j >= tileOffsetX + tileWidth || j2 < tileOffsetX || i >= tileOffsetY + tileHeight || i2 < tileOffsetY ) continue;

        int current = mosaic[k];

        VImage image = VImage::vipsload( (char *)(get<0>(cropData[t][current])).c_str() ).colourspace(VIPS_INTERPRETATION_sRGB).autorot().rot(rotAngle[get<3>(cropData[t][current])&1]);

        // Convert to a three band image
        if( image.bands() == 2 )
        {
          image = image.flatten();
        }

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
        
        float newSize = max((float)tileWidth2[t]/(float)(width-get<1>(cropData[t][current])),(float)tileHeight2[t]/(float)(height-get<2>(cropData[t][current])));

        // Extract square, flip, and rotate based on cropdata, and then save as a deep zoom image
        image = image.resize(newSize).extract_area(get<1>(cropData[t][current])*newSize, get<2>(cropData[t][current])*newSize, tileWidth2[t], tileHeight2[t]);

        if( get<4>(cropData[t][current]) )
        {
          image = image.flip(VIPS_DIRECTION_HORIZONTAL);
        }

        unsigned char *data = get<3>(cropData[t][current])&2 ? (unsigned char *)image.rot(rotAngle[get<3>(cropData[t][current])&2]).data() : (unsigned char *)image.data();

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

      for( int k = 0; k < edgeLocations.size(); ++k )
      {
        if( singleImage ) topLevel->Increment();

        int j = edgeLocations[k][0];
        int i = edgeLocations[k][1];
        int j2 = edgeLocations[k][2];
        int i2 = edgeLocations[k][3];
        int t = edgeLocations[k][4];
        
        if( j >= tileOffsetX + tileWidth || j2 < tileOffsetX || i >= tileOffsetY + tileHeight || i2 < tileOffsetY ) continue;

        int current = mosaic[k+mosaicLocations.size()];

        VImage image = VImage::vipsload( (char *)(get<0>(cropData[t][current])).c_str() ).colourspace(VIPS_INTERPRETATION_sRGB).autorot().rot(rotAngle[get<3>(cropData[t][current])&1]);

        // Convert to a three band image
        if( image.bands() == 2 )
        {
          image = image.flatten();
        }

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
        
        float newSize = max((float)tileWidth2[t]/(float)(width-get<1>(cropData[t][current])),(float)tileHeight2[t]/(float)(height-get<2>(cropData[t][current])));

        // Extract square, flip, and rotate based on cropdata, and then save as a deep zoom image
        image = image.resize(newSize).extract_area(get<1>(cropData[t][current])*newSize, get<2>(cropData[t][current])*newSize, tileWidth2[t], tileHeight2[t]);

        if( get<4>(cropData[t][current]) )
        {
          image = image.flip(VIPS_DIRECTION_HORIZONTAL);
        }

        unsigned char *data = get<3>(cropData[t][current])&2 ? (unsigned char *)image.rot(rotAngle[get<3>(cropData[t][current])&2]).data() : (unsigned char *)image.data();

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

      string tileOutputName = singleImage ? outputImage : string(outputImage).append(to_string(tileOffsetX/256)+"_"+to_string(tileOffsetY/256)+".jpeg");

      if( singleImage )
      {
        topLevel->Finish();

        cout << "Saving image" << endl;

        VImage::new_from_memory( tileData, tileHeight*tileWidth*3, tileWidth, tileHeight, 3, VIPS_FORMAT_UCHAR ).vipssave((char *)tileOutputName.c_str());
      }
      else
      {
        VImage::new_from_memory( tileData, tileHeight*tileWidth*3, tileWidth, tileHeight, 3, VIPS_FORMAT_UCHAR ).jpegsave((char *)tileOutputName.c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );
      }

      free( tileData );
    }
  }
}

void buildImage( vector< vector< cropType > > &cropData, vector< int > &mosaic, vector< vector< int > > &mosaicLocations, vector< vector< int > > &edgeLocations, vector< vector< int > > &shapeIndices, string outputImage, vector< int > &tileWidth, vector< int > &tileHeight, int outputWidth, int outputHeight )
{
  // Save image as static image or zoomable image
  if( vips_foreign_find_save( outputImage.c_str() ) != NULL )
  {
    ProgressBar *topLevel = new ProgressBar(mosaic.size(), "Generating image");

    buildTopLevel( outputImage, 0, outputHeight, outputWidth, outputHeight, cropData, mosaic, mosaicLocations, edgeLocations, shapeIndices, tileWidth, tileHeight, topLevel, outputWidth, outputHeight, true );
  }
  else
  {
    int level = (int)ceil(log2( max(outputWidth,outputHeight) ) );

    if( outputImage.back() == '/' ) outputImage = outputImage.substr(0, outputImage.size()-1);

    g_mkdir(string(outputImage).append("_files/").c_str(), 0777);
    g_mkdir(string(outputImage).append("_files/").append(to_string(level)).c_str(), 0777);

    int threads = numberOfCPUS();

    ProgressBar *topLevel = new ProgressBar(ceil((float)outputWidth/256.0)*ceil((float)outputHeight/((float)threads*256.0)), "Building top level");

    future< void > ret[threads];

    for( int k = 0; k < threads; ++k )
    {
      int start = k*outputHeight/threads;
      int end = (k+1)*outputHeight/threads;

      start = (start / 256 ) * 256;
      if( k+1 < threads ) end = (end / 256 ) * 256;

      ret[k] = async( launch::async, &buildTopLevel, string(outputImage).append("_files/"+to_string(level)+"/"), start, end, outputWidth, outputHeight, ref(cropData), ref(mosaic), ref(mosaicLocations), ref(edgeLocations), ref(shapeIndices), ref(tileWidth), ref(tileHeight), topLevel, 256, 256, false );
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
    for( int o = ceil((float)outputWidth/256.0); o > 1; o = ceil((float)o/2.0) ) numLevels += o;

    ProgressBar *lowerLevels = new ProgressBar(numLevels, "Building lower levels");

    outputWidth = (int)ceil((float)outputWidth/ 128.0 );
    outputHeight = (int)ceil((float)outputHeight/ 128.0 );

    for( ; level > 0; --level )
    {
      outputWidth = (int)ceil((float)outputWidth/ 2.0 );
      outputHeight = (int)ceil((float)outputHeight/ 2.0 );

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
        VImage::jpegload((char *)string(upper).append(to_string(outputWidth-1)+"_"+to_string(outputHeight-1)+".jpeg").c_str()).resize(0.5).
        jpegsave((char *)string(current).append(to_string(outputWidth>>1)+"_"+to_string(outputHeight>>1)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );  
      }
    }

    lowerLevels->Finish();

    // Generate html file to view deep zoom image
    ofstream htmlFile(string(outputImage).append(".html").c_str());
    htmlFile << "<!DOCTYPE html>\n<html>\n<head><script src=\"js/openseadragon.min.js\"></script></head>\n<body>\n<style>\nhtml,\nbody,\n#collage\n{\nposition: fixed;\nleft: 0;\ntop: 0;\nwidth: 100%;\nheight: 100%;\n}\n</style>\n\n<div id=\"collage\"></div>\n\n<script>\nvar viewer = OpenSeadragon({\nid: 'collage',\nprefixUrl: 'icons/',\ntileSources:   \"" + outputImage + ".dzi\",\nminZoomImageRatio: 0,\nmaxZoomImageRatio: 1\n});\n</script>\n</body>\n</html>";
    htmlFile.close();
  }
}

int generateRowThread( vector< unique_ptr< my_kd_tree_t > > &mat_index, vector< int > &mosaic, vector< vector< int > > &mosaicLocations, vector< vector< int > > shapeIndices, vector< int > &indices, int repeat, float * c, float * edgeData, int start, int end, vector< int > &tileWidth, vector< int > &tileHeight, int width, int height, ProgressBar* buildingMosaic, bool quiet )
{
  // Whether to update progressbar
  bool show = !quiet && !(start);

  // Color difference of images
  float out_dist_sqr;

  for( int i1 = start; i1 < end; ++i1 )
  {
    int i = indices[i1];
    // Update progressbar if necessary
    if(show) buildingMosaic->Increment();

    vector< int > allIndices( width+1, -1 );
    vector< int > allTypes( width+1, -1 );
    vector< float > allDifferences( width+1, -1 );
    vector< float > allMosaics( width+1, -1 );

    allDifferences[0] = 0;

    // For every index
    for( int j = 0; j < width; ++j )
    {
      if( allDifferences[j] < -0.5 ) continue;

      for( int t = 0; t < shapeIndices.size(); ++t )
      {
        if( j + tileWidth[t] > width ) continue;
        if( i + tileHeight[t] > height ) continue;
        vector< float > d(shapeIndices[t].size()*6);

        // Get rgb data about tile and save in vector
        for( int x = 0; x < shapeIndices[t].size(); ++x )
        {
          int l = 3 * ( ( i + shapeIndices[t][x]/tileWidth[t] ) * width + j + shapeIndices[t][x]%tileWidth[t] );

          d[3*x+0] = c[l];
          d[3*x+1] = c[l+1];
          d[3*x+2] = c[l+2];
          d[3*(x+shapeIndices[t].size())+0] = edgeData[l/3];
          d[3*(x+shapeIndices[t].size())+1] = 1.0f;//edgeData[l/3];
          d[3*(x+shapeIndices[t].size())+2] = 1.0f;//edgeData[l/3];
        }

        size_t ret_index;

        mat_index[t]->query( &d[0], 1, &ret_index, &out_dist_sqr );

        out_dist_sqr += allDifferences[j];

        if( allDifferences[j+tileWidth[t]] < -0.5 || out_dist_sqr < allDifferences[j+tileWidth[t]] )
        {
          allDifferences[j+tileWidth[t]] = out_dist_sqr;
          allMosaics[j+tileWidth[t]] = ret_index;
          allIndices[j+tileWidth[t]] = j;
          allTypes[j+tileWidth[t]] = t;
        }
      }
    }

    vector< int > ind;

    for( int k = width; k > 0; k = allIndices[k] )
    {
      ind.push_back(k);
    }
    ind.push_back(0);

    for( int k1 = ind.size()-1; k1 > 0; --k1 )
    {
      int ik1 = ind[k1];
      int ik2 = ind[k1-1];

      int length = ik2-ik1;

      int t = allTypes[ik2];

      mosaic.push_back(allMosaics[ik2]);

      mosaicLocations.push_back({ ik1, i, ik1+tileWidth[t], i+tileHeight[t], t } );
    }
  }

  return 0;
}

int generateRowMosaic( vector< unique_ptr< my_kd_tree_t > > &mat_index, vector< vector< int > > &mosaicLocations, vector< vector< int > > &edgeLocations, vector< vector< int > > &shapeIndices, vector< int > &mosaic, string inputImage, int repeat, bool square, int resize, bool quiet, vector< int > &tileWidth, vector< int > &tileHeight, float edgeWeight, bool smoothImage )
{
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

  int numVertical = height / tileHeight[0];

  // Get image data
  unsigned char * c = ( unsigned char * )image.data();
  float * c2 = new float[3*width*height];

  for( int p = 0; p < width*height; ++p )
  {
    rgbToLab( c[3*p+0], c[3*p+1], c[3*p+2], c2[3*p+0], c2[3*p+1], c2[3*p+2] );
  }

  float * edgeData = new float[width*height];

  generateEdgeWeights( image, edgeData, tileHeight[0], edgeWeight, smoothImage, quiet );

  // Number of threads for multithreading: needs to be a square number
  int threads = numberOfCPUS();

  // Create list of numbers in thread block
  vector< int > indices;

  for( int i = 0; i < height; i += tileHeight[0] )
  {
    indices.push_back(i);
  }

  // Break mosaic into blocks and find best matching tiles inside each block on a different thread
  future< int > ret[threads];

  ProgressBar *buildingMosaic = new ProgressBar(numVertical/threads+1, "Building mosaic");

  vector< vector< int > > mosaicLocationsThread[threads];
  vector< int > mosaicThread[threads];

  for( int i = 0; i < threads; ++i )
  {
    ret[i] = async( launch::async, &generateRowThread, ref(mat_index), ref(mosaicThread[i]), ref(mosaicLocationsThread[i]), ref(shapeIndices), ref(indices), repeat, c2, edgeData, i*indices.size()/threads, (i+1)*indices.size()/threads, ref(tileWidth), ref(tileHeight), width, height, buildingMosaic, quiet );
  }

  // Wait for threads to finish
  for( int k = 0; k < threads; ++k )
  {
    ret[k].get();

    mosaicLocations.insert(mosaicLocations.end(), mosaicLocationsThread[k].begin(), mosaicLocationsThread[k].end());
    mosaic.insert(mosaic.end(), mosaicThread[k].begin(), mosaicThread[k].end());
  }

  if( !quiet ) buildingMosaic->Finish();

/*
  vector< int > edgeIndices( edgeLocations.size() );
  iota( edgeIndices.begin(), edgeIndices.end(), 0 );

  // Shuffle the points so that patterns do not form
  shuffle( edgeIndices.begin(), edgeIndices.end(), default_random_engine(time(NULL)) );

  if( !quiet ) buildingMosaic->Finish();

  ProgressBar *buildingMosaicEdge = new ProgressBar(edgeLocations.size()/threads, "Building mosaic edge");

  for( int i = 0; i < threads; ++i )
  {
    ret[i] = async( launch::async, &generateBlockEdgeThread, ref(mat_index), ref(mosaic), mosaicLocations.size(), ref(edgeLocations), ref(shapeIndices), ref(edgeIndices), repeat, c2, edgeData, i*edgeIndices.size()/threads, (i+1)*edgeIndices.size()/threads, ref(tileWidth), width, height, buildingMosaicEdge, quiet );
  }

  // Wait for threads to finish
  for( int k = 0; k < threads; ++k )
  {
    ret[k].get();
  }

  if( !quiet ) buildingMosaicEdge->Finish();
*/
  return 0;
}


int generateBlockThread( vector< unique_ptr< my_kd_tree_t > > &mat_index, vector< int > &mosaic, vector< vector< int > > &mosaicLocations, vector< vector< int > > shapeIndices, vector< int > &indices, int repeat, float * c, float * edgeData, int start, int end, vector< int > &tileWidth, vector< int > &tileHeight, int width, ProgressBar* buildingMosaic, bool quiet )
{
  // Whether to update progressbar
  bool show = !quiet && !(start);

  // Color difference of images
  float out_dist_sqr;

  typedef KDTreeSingleIndexDynamicAdaptor< L2_Simple_Adaptor<float, my_kd_tree_t>, my_kd_tree_t> dynamic_kdtree;

  vector< unique_ptr< dynamic_kdtree > > mat_index2;//(tileArea, d, 10 );

  if( repeat > 0 )
  {
    for( int i = 0; i < mat_index.size(); ++i )
    {
      unique_ptr< dynamic_kdtree > ptr(new dynamic_kdtree(mat_index[i]->dimensions, *mat_index[i], KDTreeSingleIndexAdaptorParams(10)));
      mat_index2.push_back( move(ptr) );//3*shapeIndices[i].size(),d[i],10);
    }
  }

  // For every index
  for( int p = start; p < end; ++p )
  {
    int k = indices[p];
    int j = mosaicLocations[k][0];
    int i = mosaicLocations[k][1];
    int t = mosaicLocations[k][4];

    // Update progressbar if necessary
    if(show) buildingMosaic->Increment();

    vector< float > d(shapeIndices[t].size()*6);

    // Get rgb data about tile and save in vector
    for( int x = 0; x < shapeIndices[t].size(); ++x )
    {
      int l = 3 * ( ( i + shapeIndices[t][x]/tileWidth[t] ) * width + j + shapeIndices[t][x]%tileWidth[t] );

      d[3*x+0] = c[l];
      d[3*x+1] = c[l+1];
      d[3*x+2] = c[l+2];
      d[3*(x+shapeIndices[t].size())+0] = edgeData[l/3];
      d[3*(x+shapeIndices[t].size())+1] = 1.0f;//edgeData[l/3];
      d[3*(x+shapeIndices[t].size())+2] = 1.0f;//edgeData[l/3];
    }

    size_t ret_index;

    if( repeat > 0 )
    {
      int imagesLeft = mat_index[t]->kdtree_get_point_count();;

      // Outside of square so that closer images are not the same
      for( int r = 1; r <= repeat; ++r )
      {
        for( int k2 = 0; imagesLeft > 1 && k2 < mosaicLocations.size(); ++k2 )
        {
          int j2 = mosaicLocations[k2][0];
          int i2 = mosaicLocations[k2][1];
          int t2 = mosaicLocations[k2][4];

          if( t != t2 || mosaic[k2] < 0 ) continue;

          if( j2 >= j - r*tileWidth[t] && j2 <= j + r*tileWidth[t] && i2 >= i - r*tileHeight[t] && i2 <= i + r*tileHeight[t] )
          {
            mat_index2[t]->removePoint(mosaic[k2]);
            --imagesLeft;
          }
        }
      }

      nanoflann::KNNResultSet<float> resultSet(1);
      resultSet.init(&ret_index, &out_dist_sqr );
      mat_index2[t]->findNeighbors(resultSet, &d[0], nanoflann::SearchParams(10));

      mat_index2[t]->addPointsBack();
    }
    else
    {
      mat_index[t]->query( &d[0], 1, &ret_index, &out_dist_sqr );
    }

    // Set mosaic tile data
    mosaic[k] = ret_index;
  }

  return 0;
}

int generateBlockEdgeThread( vector< unique_ptr< my_kd_tree_t > > &mat_index, vector< int > &mosaic, int mosaicOffset, vector< vector< int > > &edgeLocations, vector< vector< int > > shapeIndices, vector< int > &indices, int repeat, float * c, float * edgeData, int start, int end, vector< int > tileWidth, int width, int height, ProgressBar* buildingMosaic, bool quiet )
{
  // Whether to update progressbar
  bool show = !quiet && !(start);

  // Color difference of images
  int out_dist_sqr;

  // For every index
  for( int p = start; p < end; ++p )
  {
    int k = indices[p];
    int j = edgeLocations[k][0];
    int i = edgeLocations[k][1];
    int t = edgeLocations[k][4];

    // Update progressbar if necessary
    if(show) buildingMosaic->Increment();

    vector< int > edgeShape;

    vector< int > d;

    // Get rgb data about tile and save in vector
    for( int x = 0; x < shapeIndices[t].size(); ++x )
    {
      int x1 = j + shapeIndices[t][x]%tileWidth[t];
      int y1 = i + shapeIndices[t][x]/tileWidth[t];
      int l = 3 * ( y1 * width + x1 );

      if( x1 >= 0 && x1 < width && y1 >= 0 && y1 < height )
      {
        edgeShape.push_back(x);

        d.push_back(edgeData[l/3]);

        d.push_back(c[l+0]);
        d.push_back(c[l+1]);
        d.push_back(c[l+2]);
      }
    }

    int bestSum = INT_MAX;
    int best = 0;

    for( int l = 0; l < mat_index[t]->kdtree_get_point_count(); ++l )
    {
      int sum = 0;

      for( int j1 = 0; j1 < edgeShape.size(); ++j1 )
      {
        int x = 3*edgeShape[j1];
        for( int k = 0; k < 3; ++k )
        {
          float diff0 = d[4*j1+1+k]-mat_index[t]->kdtree_get_pt(l,x+k); //d[4*j1] * //std::max( abs( d[4*j1+1+k]-mat_index[t]->kdtree_get_pt(l,x+k) ) - 15, 0.0f );
          if( k == 0 ) diff0 *= d[4*j1];
          sum += diff0 * diff0;
        }
      }

      if( sum < bestSum )
      {
        best = l;
        bestSum = sum;
      }
    }

    // Set mosaic tile data
    mosaic[k+mosaicOffset] = best;
  }

  return 0;
}

int generateMosaic( vector< unique_ptr< my_kd_tree_t > > &mat_index, vector< vector< int > > &mosaicLocations, vector< vector< int > > &edgeLocations, vector< vector< int > > &shapeIndices, vector< int > &mosaic, string inputImage, int repeat, bool square, int resize, bool quiet, vector< int > &tileWidth, vector< int > &tileHeight, float edgeWeight, bool smoothImage )
{
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

  for( int p = 0; p < width*height; ++p )
  {
    rgbToLab( c[3*p+0], c[3*p+1], c[3*p+2], c2[3*p+0], c2[3*p+1], c2[3*p+2] );
  }

  float * edgeData = new float[width*height];

  generateEdgeWeights( image, edgeData, tileHeight[0], edgeWeight, smoothImage, quiet );

  // Number of threads for multithreading: needs to be a square number
  int threads = numberOfCPUS();

  // Create list of numbers in thread block
  vector< int > indices( mosaicLocations.size() );
  iota( indices.begin(), indices.end(), 0 );

  // Shuffle the points so that patterns do not form
  shuffle( indices.begin(), indices.end(), default_random_engine(time(NULL)) );

  // Break mosaic into blocks and find best matching tiles inside each block on a different thread
  future< int > ret[threads];

  ProgressBar *buildingMosaic = new ProgressBar(mosaicLocations.size()/threads, "Building mosaic");

  for( int i = 0; i < threads; ++i )
  {
    ret[i] = async( launch::async, &generateBlockThread, ref(mat_index), ref(mosaic), ref(mosaicLocations), ref(shapeIndices), ref(indices), repeat, c2, edgeData, i*indices.size()/threads, (i+1)*indices.size()/threads, ref(tileWidth), ref(tileHeight), width, buildingMosaic, quiet );
  }

  // Wait for threads to finish
  for( int k = 0; k < threads; ++k )
  {
    ret[k].get();
  }

  if( !quiet ) buildingMosaic->Finish();

  vector< int > edgeIndices( edgeLocations.size() );
  iota( edgeIndices.begin(), edgeIndices.end(), 0 );

  // Shuffle the points so that patterns do not form
  shuffle( edgeIndices.begin(), edgeIndices.end(), default_random_engine(time(NULL)) );

  if( !quiet ) buildingMosaic->Finish();

  ProgressBar *buildingMosaicEdge = new ProgressBar(edgeLocations.size()/threads, "Building mosaic edge");

  for( int i = 0; i < threads; ++i )
  {
    ret[i] = async( launch::async, &generateBlockEdgeThread, ref(mat_index), ref(mosaic), mosaicLocations.size(), ref(edgeLocations), ref(shapeIndices), ref(edgeIndices), repeat, c2, edgeData, i*edgeIndices.size()/threads, (i+1)*edgeIndices.size()/threads, ref(tileWidth), width, height, buildingMosaicEdge, quiet );
  }

  // Wait for threads to finish
  for( int k = 0; k < threads; ++k )
  {
    ret[k].get();
  }

  if( !quiet ) buildingMosaicEdge->Finish();

  return 0;
}

void RunTessellate( string inputName, string outputName, vector< string > inputDirectory, int numHorizontal, int cropStyle, bool flip, bool spin, int mosaicTileWidth, int mosaicTileHeight, int imageTileWidth, int repeat, string fileName, bool quiet, bool recursiveSearch, string tessellateType, float edgeWeight, bool smoothImage )
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

  int width = VImage::new_memory().vipsload( (char *)inputImages[0].c_str() ).autorot().width();
  int height = VImage::new_memory().vipsload( (char *)inputImages[0].c_str() ).autorot().height();

  if( mosaicTileWidth == 0 ) mosaicTileWidth = width/numHorizontal;
  if( imageTileWidth == 0 ) imageTileWidth = mosaicTileWidth;
  if( mosaicTileHeight == 0 ) mosaicTileHeight = mosaicTileWidth;

  int imageTileHeight = imageTileWidth*mosaicTileHeight/mosaicTileWidth;

  int numImages;
  vector< vector< cropType > > cropData;
  vector< vector< vector< unsigned char > > > mosaicTileData;
  vector< vector< vector< unsigned char > > > imageTileData;

  bool loadData = (fileName != " ");

  vector< vector< float > > dimensions;

  vector< vector< vector< float > > > shapes;

  vector< vector< float > > offsets;

  ifstream tessellationFile( tessellateType );

  string str;

  while( getline(tessellationFile, str) && str != "dimensions:" );

  while( getline(tessellationFile, str) && str != "" )
  {
    istringstream iss(str);

    float w, h;

    iss >> w >> h;

    dimensions.push_back(vector< float >({w,h}));
  }

  shapes.resize( dimensions.size() );

  while( getline(tessellationFile, str) && str != "shapes:" && str != "offsets:" );

  if( str == "shapes:" )
  {
    for( int i = 0; i < dimensions.size(); ++i )
    {
      while( getline(tessellationFile, str) && str != "shape:" );

      while( getline(tessellationFile, str) && str != "" )
      {
        istringstream iss(str);

        float x1, x2, by1, by2, ty1, ty2;

        iss >> x1 >> x2 >> by1 >> by2 >> ty1 >> ty2;

        shapes[i].push_back(vector< float >({x1, x2, by1, by2, ty1, ty2}));
      }
    }

    while( getline(tessellationFile, str) && str != "offsets:" );
  }

  while( getline(tessellationFile, str) && str != "" )
  {
    istringstream iss(str);

    float x, y, xHorizontalOffset, yHorizontalOffset, xVerticalOffset, yVerticalOffset, shape;

    iss >> x >> y >> xHorizontalOffset >> yHorizontalOffset >> xVerticalOffset >> yVerticalOffset >> shape;

    offsets.push_back(vector< float >({x, y, xHorizontalOffset, yHorizontalOffset, xVerticalOffset, yVerticalOffset, shape}));
  }

  vector< int > tileWidth, tileHeight, tileWidth2, tileHeight2;

  for( int i = 0; i < dimensions.size(); ++i )
  {
    tileWidth.push_back(ceil((float)mosaicTileWidth*dimensions[i][0]));
    tileHeight.push_back(ceil((float)mosaicTileHeight*dimensions[i][1]));

    tileWidth2.push_back(ceil((float)imageTileWidth*dimensions[i][0]));
    tileHeight2.push_back(ceil((float)imageTileHeight*dimensions[i][1]));
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
        bool found = false;
        for( int k = 0; k < j; ++k )
        {
          if( dimensions[j] == dimensions[k] )
          {
            cropData[j] = cropData[k];
            mosaicTileData[j] = mosaicTileData[k];
            found = true;
            break;
          }
        }
        if(!found)
        {
          generateThumbnails( cropData[j], mosaicTileData[j], imageTileData[j], j == 0 ? inputDirectory : inputDirectoryBlank, imageDirectory, tileWidth[j], tileHeight[j], tileWidth2[j], tileHeight2[j], false, spin, cropStyle, flip, quiet, recursiveSearch );
        }
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

  vector< vector< int > > shapeIndices(shapes.size()), shapeIndices2(shapes.size()), mosaicLocations, mosaicLocations2, edgeLocations, edgeLocations2;

  int tileArea = mosaicTileWidth*mosaicTileHeight*3;
  int numVertical = int( (float)height / (float)width * (float)numHorizontal * (float)mosaicTileWidth/(float)mosaicTileHeight );
  int numUnique = 0;

  string tessellateTypeName = tessellateType.substr(tessellateType.find_last_of("/") + 1);


  if( tessellateTypeName == "circles.t" )
  {
    createCircleIndices( shapeIndices[0], mosaicTileWidth, mosaicTileHeight, tileWidth[0], tileHeight[0] );

    if( mosaicTileWidth != imageTileWidth )
    {
      createCircleIndices( shapeIndices2[0], imageTileWidth, imageTileHeight, tileWidth2[0], tileHeight2[0] );
    }
  }
  else if( tessellateTypeName == "jigsaw.t" )
  {
    createJigsawIndices( shapeIndices, mosaicTileWidth, mosaicTileHeight, tileWidth, tileHeight );

    if( mosaicTileWidth != imageTileWidth )
    {
      createJigsawIndices( shapeIndices2, imageTileWidth, imageTileHeight, tileWidth2, tileHeight2 );
    }
  }
  else
  {
    for( int i = 0; i < shapes.size(); ++i )
    {
      Tessellation tess(shapes[i]);
      tess.createIndices( shapeIndices[i], mosaicTileWidth, mosaicTileHeight, tileWidth[i], tileHeight[i] );

      if( mosaicTileWidth != imageTileWidth )
      {
        tess.createIndices( shapeIndices2[i], imageTileWidth, imageTileHeight, tileWidth2[i], tileHeight2[i] );
      }
    }
  }



  if( tessellateTypeName == "tetris.t" )
  {
    createTetrisLocations( mosaicLocations, mosaicLocations2, edgeLocations, edgeLocations2, offsets, dimensions, mosaicTileWidth, mosaicTileHeight, imageTileWidth, imageTileHeight, numHorizontal * mosaicTileWidth, numVertical * mosaicTileHeight, numHorizontal, numVertical );
  }
  else if( tessellateTypeName == "multisizeSquares.t" )
  {
    createMultisizeSquaresLocations( inputImages[0], shapeIndices, mosaicLocations, mosaicLocations2, edgeLocations, edgeLocations2, offsets, dimensions, tileWidth, tileHeight, tileWidth2, tileHeight2, mosaicTileWidth, mosaicTileHeight, imageTileWidth, imageTileHeight, numHorizontal * mosaicTileWidth, numVertical * mosaicTileHeight );

  }
  else if( tessellateTypeName == "multisizeTriangles.t" )
  {
    createMultisizeTrianglesLocations( inputImages[0], shapeIndices, mosaicLocations, mosaicLocations2, edgeLocations, edgeLocations2, offsets, dimensions, tileWidth, tileHeight, tileWidth2, tileHeight2, mosaicTileWidth, mosaicTileHeight, imageTileWidth, imageTileHeight, numHorizontal * mosaicTileWidth, numVertical * mosaicTileHeight );
  }
  else if( tessellateTypeName == "jigsaw.t" )
  {
    createJigsawLocations( mosaicLocations, mosaicLocations2, edgeLocations, edgeLocations2, mosaicTileWidth, mosaicTileHeight, imageTileWidth, imageTileHeight, numHorizontal * mosaicTileWidth, numVertical * mosaicTileHeight );
  }
  else if( tessellateTypeName == "offsets.t" || tessellateTypeName == "offsets2.t" )
  {
  }
  else
  {
    createLocations( mosaicLocations, mosaicLocations2, edgeLocations, edgeLocations2, offsets, dimensions, mosaicTileWidth, mosaicTileHeight, imageTileWidth, imageTileHeight, numHorizontal * mosaicTileWidth, numVertical * mosaicTileHeight );
  }

  int imageWidth = numHorizontal * imageTileWidth, imageHeight = numVertical * imageTileHeight;

  float **d = (float **)malloc(shapeIndices.size()*sizeof(float*));

  for( int i = 0; i < shapeIndices.size(); ++i )
  {
    tileArea = shapeIndices[i].size();

    d[i] = (float *)malloc(mosaicTileData[i].size()*3*tileArea*sizeof(float));

    for( int j = 0, index = 0; j < mosaicTileData[i].size(); ++j )
    {
      for( int p = 0; p < tileArea; ++p, ++index )
      {
        rgbToLab( mosaicTileData[i][j][3*shapeIndices[i][p]+0], mosaicTileData[i][j][3*shapeIndices[i][p]+1], mosaicTileData[i][j][3*shapeIndices[i][p]+2], d[i][3*index+0], d[i][3*index+1], d[i][3*index+2] );
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
    unique_ptr< my_kd_tree_t > ptr(new my_kd_tree_t(3*shapeIndices[i].size(),mosaicTileData[i].size(),d[i],10));
    mat_index.push_back( move(ptr) );//3*shapeIndices[i].size(),d[i],10);
  }

  for( int i = 0; i < inputImages.size(); ++i )
  {
    vector< int > mosaic( mosaicLocations.size() + edgeLocations.size() );

//    ProgressBar *buildingMosaic = new ProgressBar(mosaicLocations.size()/threads, "Building mosaic");

    if( tessellateTypeName == "offsets.t" || tessellateTypeName == "offsets2.t" )
    {
      numUnique = generateRowMosaic( mat_index, mosaicLocations, edgeLocations, shapeIndices, mosaic, inputImages[i], repeat, false, numHorizontal * mosaicTileWidth, quiet, tileWidth, tileHeight, edgeWeight, smoothImage );
    }
    else
    {
      numUnique = generateMosaic( mat_index, mosaicLocations, edgeLocations, shapeIndices, mosaic, inputImages[i], repeat, false, numHorizontal * mosaicTileWidth, quiet, tileWidth, tileHeight, edgeWeight, smoothImage );
    }

    for( int m1 = 0; m1 < mosaicLocations.size(); ++m1 )
    {
      mosaicLocations2.push_back({ mosaicLocations[m1][0]*imageTileWidth/mosaicTileWidth, mosaicLocations[m1][1]*imageTileHeight/mosaicTileHeight, mosaicLocations[m1][2]*imageTileWidth/mosaicTileWidth, mosaicLocations[m1][3]*imageTileHeight/mosaicTileHeight, mosaicLocations[m1][4] });
    }

    if( mosaicTileWidth == imageTileWidth )
    {
      buildImage( cropData, mosaic, mosaicLocations, edgeLocations, shapeIndices, outputImages[i], tileWidth, tileHeight, imageWidth, imageHeight );
    }
    else
    {
      buildImage( cropData, mosaic, mosaicLocations2, edgeLocations2, shapeIndices2, outputImages[i], tileWidth2, tileHeight2, imageWidth, imageHeight );
    }
  }

  for( int i = 0; i < shapeIndices.size(); ++i )
  {
    free(d[i]);
  }
}