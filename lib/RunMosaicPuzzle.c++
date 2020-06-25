#include "MosaicPuzzle.h"
#include <tclap/CmdLine.h>

using namespace TCLAP;

int main( int argc, char **argv )
{
  try
  {
    CmdLine cmd("Creates an image mosaic.", ' ', "2.0");

    SwitchArg recursiveArg( "", "recursive", "Recursive search directories", cmd, false );

    ValueArg<string> fileArg( "", "file", "File of voronoi diagram", false, " ", "string", cmd);

    SwitchArg skipNearByArg( "s", "skipNearBy", "Skip images if they nearby cells used them", cmd, false );

    SwitchArg secondPassArg( "g", "gapfill", "Do another pass to fill the gaps", cmd, false );

    ValueArg<float> edgeWeightArg( "e", "edgeWeight", "Weight edge pixels more than other pixels", false, 1.0, "float", cmd);

    ValueArg<float> fillWeightArg( "f", "fillWeight", "Weight of cell fill value", false, 1.0, "float", cmd);

    ValueArg<float> renderScaleArg( "r", "renderScale", "Scale for rendering the mosaic", false, 1.0, "float", cmd);

    ValueArg<float> buildScaleArg( "b", "buildScale", "Scale for building the mosaic", false, 1.0, "float", cmd);

    ValueArg<float> angleOffsetArg( "a", "angleOffset", "Angle to offset images by to find best rotation", false, 360.0, "float", cmd);

    ValueArg<int> numCellsArg( "n", "numCells", "Number of cells to use", true, 100, "int", cmd);

    ValueArg<string> outputArg( "o", "output", "Output name", true, "out", "string", cmd);

    MultiArg<string> directoryArg( "d", "directory", "Directory of images to use to build mosaic", true, "string", cmd);

    ValueArg<string> pictureArg( "p", "picture", "Reference picture to create mosaic from", true, "in", "string", cmd);

    cmd.parse( argc, argv );

    string inputName                  = pictureArg.getValue();
    vector< string > inputDirectory   = directoryArg.getValue();
    string outputName                 = outputArg.getValue();
    bool skipNearBy                   = skipNearByArg.getValue();
    bool recursiveSearch              = recursiveArg.getValue();
    bool secondPass                   = secondPassArg.getValue();
    int numCells                      = numCellsArg.getValue();
    float sizePower                   = fillWeightArg.getValue();
    float renderScale                 = renderScaleArg.getValue();
    float buildScale                  = buildScaleArg.getValue();
    float edgeWeight                  = edgeWeightArg.getValue();
    float angleOffset                 = angleOffsetArg.getValue();
    string fileName                   = fileArg.getValue();

    renderScale /= buildScale;

    if( VIPS_INIT( argv[0] ) ) return( -1 );

    RunMosaicPuzzle( inputName, outputName, inputDirectory, numCells, secondPass, renderScale, buildScale, angleOffset, edgeWeight, skipNearBy, sizePower, recursiveSearch, fileName );
  }
  catch (ArgException &e)  // catch any exceptions
  {
    cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
  }
  return 0;
}