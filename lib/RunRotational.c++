#include "Rotational.h"
#include <tclap/CmdLine.h>

using namespace TCLAP;

int main( int argc, char **argv )
{
  try
  {
    CmdLine cmd("Creates an image mosaic.", ' ', "2.0");

    ValueArg<int> numSkipArg( "s", "skip", "Number of opaque pixels random points to skip", false, 1000000, "int", cmd);

    ValueArg<int> angleOffsetArg( "a", "angleOffset", "Angle offset", true, 90, "int", cmd);

    ValueArg<double> renderScaleArg( "r", "renderScale", "Render scale", true, 1.0, "double", cmd);

    ValueArg<double> imageScaleArg( "i", "imageScale", "Image scale", true, 1.0, "double", cmd);

    ValueArg<int> numIterArg( "n", "numIter", "Number of iterations", true, 1000, "int", cmd);

    ValueArg<string> outputArg( "o", "output", "Output name", true, "out", "string", cmd);

    MultiArg<string> directoryArg( "d", "directory", "Directory of images to use to build mosaic", true, "string", cmd);

    ValueArg<string> pictureArg( "p", "picture", "Reference picture to create mosaic from", true, "in", "string", cmd);

    cmd.parse( argc, argv );

    string inputName                  = pictureArg.getValue();
    vector< string > inputDirectory   = directoryArg.getValue();
    string outputName                 = outputArg.getValue();
    double imageScale                 = imageScaleArg.getValue();
    double renderScale                = renderScaleArg.getValue();
    int angleOffset                   = angleOffsetArg.getValue();
    int numIter                       = numIterArg.getValue();
    int numSkip                       = numSkipArg.getValue();

    if( VIPS_INIT( argv[0] ) ) return( -1 );

    RunRotational( inputName, outputName, inputDirectory, numIter, angleOffset, imageScale, renderScale, numSkip );
  }
  catch (ArgException &e)  // catch any exceptions
  {
    cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
  }
  return 0;
}