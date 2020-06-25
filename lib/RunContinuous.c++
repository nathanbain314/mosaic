#include "Continuous.h"
#include <tclap/CmdLine.h>

using namespace TCLAP;

int main( int argc, char **argv )
{  
  try
  {
    CmdLine cmd("Creates an image mosaic.", ' ', "2.0");

    ValueArg<int> mosaicTileArg( "m", "mosaicTileSize", "Maximum tile size for generating mosaic", false, 0, "int", cmd);

    ValueArg<int> repeatArg( "r", "repeat", "Closest distance to repeat image", false, 0, "int", cmd);

    ValueArg<string> outputArg( "o", "output", "Output name", true, "out", "string", cmd);

    MultiArg<string> directoryArg( "d", "directory", "Directory of images to use to build mosaic", true, "string", cmd);

    cmd.parse( argc, argv );

    vector< string > inputDirectory   = directoryArg.getValue();
    string outputDirectory            = outputArg.getValue();
    int repeat                        = repeatArg.getValue();
    int mosaicTileSize                = mosaicTileArg.getValue();

    if( VIPS_INIT( argv[0] ) ) return( -1 );

    RunContinuous( inputDirectory, outputDirectory, mosaicTileSize, repeat );
  }
  catch (ArgException &e)  // catch any exceptions
  {
    cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
  }
  return 0;
}