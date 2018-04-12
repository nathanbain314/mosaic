#include "Continuous.h"
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

    if( VIPS_INIT( argv[0] ) ) return( -1 );

    RunContinuous( inputDirectory, outputDirectory, mosaicTileSize, trueColor, repeat );
  }
  catch (ArgException &e)  // catch any exceptions
  {
    cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
  }
  return 0;
}