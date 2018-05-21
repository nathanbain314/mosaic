#include "Collage.h"
#include <tclap/CmdLine.h>

using namespace TCLAP;

int main( int argc, char **argv )
{
  try
  {
    CmdLine cmd("Creates a collage.", ' ', "2.0");

    ValueArg<string> fileArg( "", "file", "File of image data to save or load", false, " ", "string", cmd);

    SwitchArg recursiveArg( "", "recursive", "Recursive search directories", cmd, false );

    SwitchArg colorArg( "t", "trueColor", "Use de00 for color difference", cmd, false );

    ValueArg<int> maxSizeArg( "", "max", "Max size of image before processing", false, -1, "int", cmd);

    ValueArg<int> minSizeArg( "", "min", "Min size of image before processing", false, -1, "int", cmd);

    ValueArg<int> skipArg( "s", "skip", "Number of times to skip over an image before using it again", false, 0, "int", cmd);

    ValueArg<int> fillPercentageArg( "f", "fillPercentage", "Percentage of output to fill", false, 100, "int", cmd);

    ValueArg<int> angleOffsetArg( "a", "angleOffset", "Angle offset", false, 90, "int", cmd);

    ValueArg<double> renderScaleArg( "r", "renderScale", "Render scale", false, 1.0, "double", cmd);

    ValueArg<double> imageScaleArg( "i", "imageScale", "Image scale", false, 1.0, "double", cmd);

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
    int fillPercentage                = fillPercentageArg.getValue();
    int skip                          = skipArg.getValue();
    int maxSize                       = maxSizeArg.getValue();
    int minSize                       = minSizeArg.getValue();
    bool trueColor                    = colorArg.getValue();
    bool recursiveSearch              = recursiveArg.getValue();
    string fileName                   = fileArg.getValue();

    if( VIPS_INIT( argv[0] ) ) return( -1 );

    RunCollage( inputName, outputName, inputDirectory, angleOffset, imageScale, renderScale, fillPercentage, trueColor, fileName, minSize, maxSize, skip, recursiveSearch );
  }
  catch (ArgException &e)  // catch any exceptions
  {
    cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
  }
  return 0;
}