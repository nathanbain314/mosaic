#include "mosaic.h"
#include <tclap/CmdLine.h>

using namespace TCLAP;

int main( int argc, char **argv )
{
  try
  {
    CmdLine cmd("Creates an image mosaic.", ' ', "2.0");

    ValueArg<string> fileArg( "", "file", "File of image data to save or load", false, " ", "string", cmd);

    ValueArg<int> imageTileArg( "i", "imageTileWidth", "Tile width for generating image", false, 0, "int", cmd);

    ValueArg<int> mosaicTileHeightArg( "l", "mosaicTileHeight", "Maximum tile height for generating mosaic", false, 0, "int", cmd);

    ValueArg<int> mosaicTileWidthArg( "m", "mosaicTileWidth", "Maximum tile width for generating mosaic", false, 0, "int", cmd);

    SwitchArg colorArg( "t", "trueColor", "Use de00 for color difference", cmd, false );

    SwitchArg flipArg( "f", "flip", "Flip each image to create twice as many images", cmd, false );

    SwitchArg spinArg( "s", "spin", "Rotate each image to create four times as many images", cmd, false );

    ValueArg<int> cropStyleArg( "c", "cropStyle", "Style to use for cropping images", false, 0, "int", cmd );

    ValueArg<int> repeatArg( "r", "repeat", "Closest distance to repeat image", false, 0, "int", cmd);

    ValueArg<int> numberArg( "n", "numHorizontal", "Number of tiles horizontally", true, 20, "int", cmd);

    ValueArg<string> outputArg( "o", "output", "Output name", true, "out", "string", cmd);

    MultiArg<string> directoryArg( "d", "directory", "Directory of images to use to build mosaic", true, "string", cmd);

    ValueArg<string> pictureArg( "p", "picture", "Reference picture to create mosaic from", true, "in", "string", cmd);

    cmd.parse( argc, argv );

    string inputName                  = pictureArg.getValue();
    vector< string > inputDirectory   = directoryArg.getValue();
    string outputName                 = outputArg.getValue();
    int repeat                        = repeatArg.getValue();
    int numHorizontal                 = numberArg.getValue();
    bool trueColor                    = colorArg.getValue();
    bool flip                         = flipArg.getValue();
    bool spin                         = spinArg.getValue();
    int cropStyle                     = cropStyleArg.getValue();
    int mosaicTileWidth               = mosaicTileWidthArg.getValue();
    int mosaicTileHeight              = mosaicTileHeightArg.getValue();
    int imageTileWidth                = imageTileArg.getValue();
    string fileName                   = fileArg.getValue();

    if( VIPS_INIT( argv[0] ) ) return( -1 );

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
        generateThumbnails( cropData, mosaicTileData, imageTileData, imageDirectory, mosaicTileWidth, mosaicTileHeight, imageTileWidth, imageTileHeight, isDeepZoom, spin, cropStyle, flip );
      }

      numImages = mosaicTileData.size();

      if( numImages == 0 ) 
      {
        cout << "No valid images found. The directory might not contain any images, or they might be too small, or they might not be a valid format." << endl;
        return 0;
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

      ProgressBar *buildingMosaic = new ProgressBar((numVertical/3)*(numHorizontal/3), "Building mosaic");

      if( trueColor  )
      {
        numUnique = generateMosaic( lab, mosaic, inputImages[i], buildingMosaic, repeat, false, numHorizontal * mosaicTileWidth );
      }
      else
      {
        numUnique = generateMosaic( mat_index, mosaic, inputImages[i], buildingMosaic, repeat, false, numHorizontal * mosaicTileWidth );
      }

      cout << endl;

      if( isDeepZoom )
      {
        if( outputImages[i].back() != '/' ) outputImages[i] += '/';

        g_mkdir(outputImages[i].c_str(), 0777);

        ofstream htmlFile(outputImages[i].substr(0, outputImages[i].size()-1).append(".html").c_str());

        htmlFile << "<!DOCTYPE html>\n<html>\n<head><script src=\"js/openseadragon.min.js\"></script></head>\n<body>\n<style>\nhtml,\nbody,\n#mosaic\n{\nposition: fixed;\nleft: 0;\ntop: 0;\nwidth: 100%;\nheight: 100%;\n}\n</style>\n\n<div id=\"mosaic\" ></div>\n<script type=\"text/javascript\">\nvar outputName = \"" << outputImages[i] << "\";\nvar outputDirectory = \"" << outputImages[i] << "zoom/\";\n";

        buildDeepZoomImage( mosaic, cropData, numUnique, mosaicTileWidth, mosaicTileHeight, outputImages[i], htmlFile );

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
  catch (ArgException &e)  // catch any exceptions
  {
    cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
  }
  return 0;
}