#include "mosaic.h"
#include <tclap/CmdLine.h>

using namespace TCLAP;

int main( int argc, char **argv )
{
  try
  {
    CmdLine cmd("Creates an image mosaic.", ' ', "2.0");

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

    if( VIPS_INIT( argv[0] ) ) return( -1 );

    vector< vector< unsigned char > > images, masks;
    vector< pair< int, int > > dimensions;

    for( int i = 0; i < inputDirectory.size(); ++i )
    {
      string imageDirectory = inputDirectory[i];
      if( imageDirectory.back() != '/' ) imageDirectory += '/';
      generateRotationalThumbnails( imageDirectory, images, masks, dimensions, imageScale, renderScale );
    }

    buildRotationalImage( inputName, outputName, images, masks, dimensions, renderScale/imageScale, numIter, angleOffset );
  }
  catch (ArgException &e)  // catch any exceptions
  {
    cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
  }
  return 0;
}