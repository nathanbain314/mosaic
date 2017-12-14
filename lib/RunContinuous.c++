#include "mosaic.h"
#include <tclap/CmdLine.h>

using namespace TCLAP;

int main( int argc, char **argv )
{
  typedef KDTreeVectorOfVectorsAdaptor< vector< vector< int > >, int >  my_kd_tree_t;
  
  try
  {
    CmdLine cmd("Creates an image mosaic.", ' ', "2.0");

    ValueArg<int> mosaicTileArg( "m", "mosaicTileSize", "Maximum tile size for generating mosaic", false, 0, "int", cmd);

    SwitchArg colorArg( "t", "trueColor", "Use de00 for color difference", cmd, false );

    ValueArg<int> repeatArg( "r", "repeat", "Closest distance to repeat image", false, 0, "int", cmd);

    ValueArg<string> outputArg( "o", "output", "Output name", true, "out", "string", cmd);

    ValueArg<string> imagesArg( "i", "images", "Directory of images to use to build mosaic", true, "images", "string", cmd);

    cmd.parse( argc, argv );

    string inputDirectory     = imagesArg.getValue();
    string outputDirectory    = outputArg.getValue();
    int repeat                = repeatArg.getValue();
    bool trueColor            = colorArg.getValue();
    int mosaicTileSize        = mosaicTileArg.getValue();

    int tileWidth = ( mosaicTileSize > 0 && mosaicTileSize < 64 ) ? mosaicTileSize : 64;
    int tileWidthSqr = tileWidth*tileWidth;
    int tileArea = tileWidthSqr*3;

    if( inputDirectory.back() != '/' ) inputDirectory += '/';
    if( outputDirectory.back() != '/' ) outputDirectory += '/';

    g_mkdir(outputDirectory.c_str(), 0777);

    if( VIPS_INIT( argv[0] ) ) return( -1 );

    vector< vector< unsigned char > > startData, averages2, averages;
    vector< cropType > cropData;
    vector< VImage > images;

    generateThumbnails( cropData, startData, startData, inputDirectory, tileWidth, tileWidth );

    int numImages = cropData.size();

    vector< vector< vector< int > > > mosaics( numImages, vector< vector< int > >( 64, vector< int >( 64 ) ) );

    unsigned char **data = new unsigned char*[numImages];

    for( int n = 0; n < numImages; ++n )
    {
      int rv = 0, gv = 0, bv = 0;
      for( int p = 0; p < tileArea; p+=3 )
      {
        rv += startData[n][p];
        gv += startData[n][p+1];
        bv += startData[n][p+2];
      }
      averages.push_back( { (unsigned char)(rv/tileWidthSqr), (unsigned char)(gv/tileWidthSqr), (unsigned char)(bv/tileWidthSqr) } );
    }

    progressbar *buildingMosaic = progressbar_new("Building mosaics", numImages );

    if( trueColor )
    {
      vector< vector< float > > lab( numImages, vector< float >(tileArea) );

      float r,g,b;

      for( int j = 0; j < numImages; ++j )
      {
        for( int p = 0; p < tileArea; p+=3 )
        {
          vips_col_sRGB2scRGB_16(startData[j][p],startData[j][p+1],startData[j][p+2], &r,&g,&b );
          vips_col_scRGB2XYZ( r, g, b, &r, &g, &b );
          vips_col_XYZ2Lab( r, g, b, &lab[j][p], &lab[j][p+1], &lab[j][p+2] );
        }
      }

      for( int n = 0; n < numImages; ++n )
      {
        generateMosaic( lab, mosaics[n], get<0>(cropData[n]), NULL, repeat, true, tileWidth*64 );
        progressbar_inc( buildingMosaic );
      }
    }
    else
    {
      vector< vector< int > > d( numImages, vector< int >(tileArea) );

      for( int j = 0; j < numImages; ++j )
      {
        for( int p = 0; p < tileArea; ++p )
        {
          d[j][p] = startData[j][p];
        }
      }

      my_kd_tree_t mat_index(tileArea, d, 10 );

      for( int n = 0; n < numImages; ++n )
      {
        generateMosaic( mat_index, mosaics[n], get<0>(cropData[n]), NULL, repeat, true, tileWidth*64 );
        progressbar_inc( buildingMosaic );
      }
    }

    progressbar_finish( buildingMosaic );
    progressbar *generateAverages = progressbar_new("Generating averages", numImages * 2 );

    for( int n = 0; n < numImages; ++n )
    {
      vector< int > sum( 12, 0 );
      vector< unsigned char > avg( 12, 0 );

      for( int i1 = 0, i2 = 32; i1 < 32; ++i1, ++i2 )
      {
        for( int j1 = 0, j2 = 32; j1 < 32; ++j1, ++j2 )
        {
          sum[0] += averages[mosaics[n][i1][j1]][0];
          sum[1] += averages[mosaics[n][i1][j1]][1];
          sum[2] += averages[mosaics[n][i1][j1]][2];
          sum[3] += averages[mosaics[n][i1][j2]][0];
          sum[4] += averages[mosaics[n][i1][j2]][1];
          sum[5] += averages[mosaics[n][i1][j2]][2];
          sum[6] += averages[mosaics[n][i2][j1]][0];
          sum[7] += averages[mosaics[n][i2][j1]][1];
          sum[8] += averages[mosaics[n][i2][j1]][2];
          sum[9] += averages[mosaics[n][i2][j2]][0];
          sum[10] += averages[mosaics[n][i2][j2]][1];
          sum[11] += averages[mosaics[n][i2][j2]][2];
        }
      }

      for( int i = 0; i < 12; ++i )
      {
        avg[i] = (unsigned char)(sum[i]>>10);
      }

      averages2.push_back( avg );

      progressbar_inc( generateAverages );
    }

    for( int n = 0; n < numImages; ++n )
    {
      data[n] = new unsigned char[49152];

      for( int i = 0; i < 64; ++i )
      {
        for( int j = 0; j < 64; ++j )
        {
          for( int k = 0; k < 6; ++k )
          {
            data[n][i*768+j*6+k] = (unsigned char)averages2[mosaics[n][i][j]][k];
            data[n][i*768+j*6+384+k] = (unsigned char)averages2[mosaics[n][i][j]][k+6];
          }
        }
      }

      images.push_back( VImage::new_from_memory( data[n], 49152, 128, 128, 3, VIPS_FORMAT_UCHAR ) );
      
      progressbar_inc( generateAverages );
    }
    progressbar_finish( generateAverages );

    ofstream htmlFile(outputDirectory.substr(0, outputDirectory.size()-1).append(".html").c_str());

    htmlFile << "<!DOCTYPE html>\n<html>\n<head>\n<script src=\"js/openseadragon.min.js\"></script>\n<link rel=\"stylesheet\" href=\"https://code.jquery.com/ui/1.11.4/themes/smoothness/jquery-ui.css\">\n<script src=\"https://code.jquery.com/jquery-1.11.3.min.js\"></script>\n<script src=\"https://code.jquery.com/ui/1.11.4/jquery-ui.min.js\"></script>\n</head>\n<body>\n<div id=\"slider\" style=\"margin: 10px\"></div>\n<div id=\"continuous\" style=\"width: 500px; height: 500px;\"></div>\n<script type=\"text/javascript\">\nvar outputDirectory = \"" << outputDirectory << "\";\nvar topLevel = [[0,0],[0,0]];\n";

    buildContinuousMosaic( mosaics, images, outputDirectory, htmlFile );

    htmlFile << "</script>\n<script src=\"js/continuous.js\"></script>\n</body>\n</html>";

    htmlFile.close();
  }
  catch (ArgException &e)  // catch any exceptions
  {
    cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
  }
  return 0;
}