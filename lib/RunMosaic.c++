#include "mosaic.h"
#include <tclap/CmdLine.h>

using namespace TCLAP;

int main( int argc, char **argv )
{
  try
  {
    CmdLine cmd("Creates an image mosaic.", ' ', "2.0");

    ValueArg<int> imageTileArg( "i", "imageTileSize", "Tile size for generating image", false, 0, "int", cmd);

    ValueArg<int> mosaicTileArg( "m", "mosaicTileSize", "Maximum tile size for generating mosaic", false, 0, "int", cmd);

    SwitchArg squareArg( "e", "exclude", "Exclude images smaller than 256x256", cmd, false );

    SwitchArg excludeArg( "s", "square", "Generate square mosaic", cmd, false );

    SwitchArg colorArg( "c", "color", "Use de00 for color difference", cmd, false );

    ValueArg<int> repeatArg( "r", "repeat", "Closest distance to repeat image", false, 0, "int", cmd);

    ValueArg<int> numberArg( "n", "numHorizontal", "Number of tiles horizontally", true, 20, "int", cmd);

    ValueArg<string> outputArg( "o", "output", "Output name", true, "out", "string", cmd);

    ValueArg<string> directoryArg( "d", "directory", "Directory of images to use to build mosaic", true, "images", "string", cmd);

    ValueArg<string> pictureArg( "p", "picture", "Reference picture to create mosaic from", true, "in", "string", cmd);

    cmd.parse( argc, argv );

    string inputImage         = pictureArg.getValue();
    string inputDirectory     = directoryArg.getValue();
    string outputImage        = outputArg.getValue();
    int repeat                = repeatArg.getValue();
    int numHorizontal         = numberArg.getValue();
    bool trueColor            = colorArg.getValue();
    bool square               = squareArg.getValue();
    bool exclude              = excludeArg.getValue();
    int mosaicTileSize        = mosaicTileArg.getValue();
    int imageTileSize         = imageTileArg.getValue();

    if( inputDirectory.back() != '/' ) inputDirectory += '/';

    int width = VImage::new_memory().vipsload( (char *)inputImage.c_str() ).width();
    int height = VImage::new_memory().vipsload( (char *)inputImage.c_str() ).height();

    if( mosaicTileSize > 0 )
    {
      mosaicTileSize = min( mosaicTileSize, square ? int( min( width, height ) )/numHorizontal : width/numHorizontal );
    }
    else
    {
      mosaicTileSize = square ? int( min( width, height ) )/numHorizontal : width/numHorizontal;
    }
    if( imageTileSize == 0 ) imageTileSize = mosaicTileSize;
    int tileArea = mosaicTileSize*mosaicTileSize*3;

    int resize = numHorizontal * mosaicTileSize;

    if( VIPS_INIT( argv[0] ) ) return( -1 );

    vector< string > names;
    vector< vector< unsigned char > > mosaicTileData;
    vector< vector< unsigned char > > imageTileData;

    generateThumbnails( names, mosaicTileData, imageTileData, inputDirectory, mosaicTileSize, imageTileSize, exclude );
    int numImages = mosaicTileData.size();
    int numVertical = int( (double)height / (double)width * (double)numHorizontal );
    int numUnique = 0;

    vector< vector< int > > mosaic( numVertical, vector< int >( numHorizontal, -1 ) );

    progressbar *buildingMosaic = progressbar_new("Building mosaic", numVertical*numHorizontal );

    if( trueColor  )
    {
      vector< vector< float > > lab( numImages, vector< float >(tileArea) );

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

      numUnique = generateMosaic( lab, mosaic, inputImage, buildingMosaic, repeat, square, resize );
    }
    else
    {
      vector< vector< int > > d( numImages, vector< int >(tileArea) );

      for( int j = 0; j < numImages; ++j )
      {
        for( int p = 0; p < tileArea; ++p )
        {
          d[j][p] = mosaicTileData[j][p];
        }
      }

      typedef KDTreeVectorOfVectorsAdaptor< vector< vector< int > >, int >  my_kd_tree_t;

      my_kd_tree_t mat_index(tileArea, d, 10 );

      numUnique = generateMosaic( mat_index, mosaic, inputImage, buildingMosaic, repeat, square, resize );
    }

    progressbar_finish( buildingMosaic );

    if( vips_foreign_find_save( outputImage.c_str() ) == NULL )
    {
      if( outputImage.back() != '/' ) outputImage += '/';

      g_mkdir(outputImage.c_str(), 0777);

      ofstream htmlFile(outputImage.substr(0, outputImage.size()-1).append(".html").c_str());

      htmlFile << "<!DOCTYPE html>\n<html>\n<head><script src=\"js/openseadragon.min.js\"></script></head>\n<body>\n<div id=\"mosaic\" style=\"width: 1000px; height: 600px;\"></div>\n<script type=\"text/javascript\">\nvar outputName = \"" << outputImage << "\";\nvar outputDirectory = \"" << outputImage << "zoom/\";\n";

      buildDeepZoomImage( mosaic, names, numUnique, outputImage, htmlFile );

      htmlFile << "</script>\n<script src=\"js/mosaic.js\"></script>\n</body>\n</html>";

      htmlFile.close();
    }
    else
    {
      if( mosaicTileSize == imageTileSize )
      {
        buildImage( mosaicTileData, mosaic, outputImage, mosaicTileSize );
      }
      else
      {
        buildImage( imageTileData, mosaic, outputImage, imageTileSize );
      }
    }
  }
  catch (ArgException &e)  // catch any exceptions
  {
    cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
  }
  return 0;
}