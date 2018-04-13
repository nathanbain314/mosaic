#include "Continuous.h"

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

    ProgressBar *generatingLevels = new ProgressBar(numImages, "Generating level 5");

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
      generatingLevels->Increment();
    }

    generatingLevels->Finish();

    currentMosaic << "]\n";

    mosaicStrings.push_back( currentMosaic.str() );
  }

  for( int l = 4; l >= 0 ; --l )
  {
    ostringstream currentMosaic;
    currentMosaic << "[";

    string progressString = "Generating level "+ to_string(l);
    ProgressBar *generatingLevels = new ProgressBar(numImages, progressString.c_str());

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
      generatingLevels->Increment();
    }

    generatingLevels->Finish();

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

void RunContinuous( string inputDirectory, string outputDirectory, int mosaicTileSize, bool trueColor, int repeat )
{
  int tileWidth = ( mosaicTileSize > 0 && mosaicTileSize < 64 ) ? mosaicTileSize : 64;
  int tileWidthSqr = tileWidth*tileWidth;
  int tileArea = tileWidthSqr*3;

  if( inputDirectory.back() != '/' ) inputDirectory += '/';
  if( outputDirectory.back() != '/' ) outputDirectory += '/';

  g_mkdir(outputDirectory.c_str(), 0777);

  vector< vector< unsigned char > > startData, averages2, averages;
  vector< cropType > cropData;
  vector< VImage > images;

  generateThumbnails( cropData, startData, startData, inputDirectory, tileWidth, tileWidth, tileWidth, tileWidth );

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

  ProgressBar *buildingMosaic = new ProgressBar(numImages, "Building mosaic");

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
      buildingMosaic->Increment();
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
      buildingMosaic->Increment();
    }
  }

  buildingMosaic->Finish();

  ProgressBar *generateAverages = new ProgressBar(numImages * 2, "Generating averages");

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

    generateAverages->Increment();
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
    
    generateAverages->Increment();
  }

  generateAverages->Finish();

  ofstream htmlFile(outputDirectory.substr(0, outputDirectory.size()-1).append(".html").c_str());

  htmlFile << "<!DOCTYPE html>\n<html>\n<head>\n<script src=\"js/openseadragon.min.js\"></script>\n<link rel=\"stylesheet\" href=\"https://code.jquery.com/ui/1.11.4/themes/smoothness/jquery-ui.css\">\n<script src=\"https://code.jquery.com/jquery-1.11.3.min.js\"></script>\n<script src=\"https://code.jquery.com/ui/1.11.4/jquery-ui.min.js\"></script>\n</head>\n<body>\n<div id=\"slider\" style=\"margin: 10px\"></div>\n<div id=\"continuous\" style=\"width: 500px; height: 500px;\"></div>\n<script type=\"text/javascript\">\nvar outputDirectory = \"" << outputDirectory << "\";\nvar topLevel = [[0,0],[0,0]];\n";

  buildContinuousMosaic( mosaics, images, outputDirectory, htmlFile );

  htmlFile << "</script>\n<script src=\"js/continuous.js\"></script>\n</body>\n</html>";

  htmlFile.close();
}