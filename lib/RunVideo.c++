#include "Video.h"
#include <tclap/CmdLine.h>

using namespace TCLAP;

int main( int argc, char **argv )
{
  try
  {
    CmdLine cmd("Creates an image mosaic.", ' ', "2.0");

    ValueArg<string> fileArg( "", "file", "File of image data to save or load", false, " ", "string", cmd);

    SwitchArg recursiveArg( "", "recursive", "Recursive search directories", cmd, false );

    ValueArg<int> skipTileArg( "s", "skip", "Number of frames to skip in between each video", false, 30, "int", cmd);

    ValueArg<int> framesTileArg( "f", "frames", "Number of frames in each video", false, 30, "int", cmd);

    ValueArg<int> imageTileArg( "i", "imageTileWidth", "Tile width for generating image", false, 0, "int", cmd);

    ValueArg<int> mosaicTileHeightArg( "l", "mosaicTileHeight", "Maximum tile height for generating mosaic", false, 0, "int", cmd);

    ValueArg<int> mosaicTileWidthArg( "m", "mosaicTileWidth", "Maximum tile width for generating mosaic", false, 0, "int", cmd);

    SwitchArg edgeWeightsArg( "", "edgeWeights", "Weight edge pixels more than other pixels", cmd, false );

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
    bool useEdgeWeights               = edgeWeightsArg.getValue();
    bool recursiveSearch              = recursiveArg.getValue();
    int mosaicTileWidth               = mosaicTileWidthArg.getValue();
    int mosaicTileHeight              = mosaicTileHeightArg.getValue();
    int imageTileWidth                = imageTileArg.getValue();
    int frames                        = framesTileArg.getValue();
    int skip                          = skipTileArg.getValue();
    string fileName                   = fileArg.getValue();

    if( VIPS_INIT( argv[0] ) ) return( -1 );

    RunVideo( inputName, outputName, inputDirectory, numHorizontal, mosaicTileWidth, mosaicTileHeight, imageTileWidth, repeat, fileName, frames, skip, recursiveSearch, useEdgeWeights );
  }
  catch (ArgException &e)  // catch any exceptions
  {
    cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
  }
  return 0;
}