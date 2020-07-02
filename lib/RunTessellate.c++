#include "Tessellate.h"
#include <tclap/CmdLine.h>

using namespace TCLAP;

int main( int argc, char **argv )
{
  try
  {
    CmdLine cmd("Creates an tessellating mosaic.", ' ', "2.0");

    SwitchArg recursiveArg( "", "recursive", "Recursive search directories", cmd, false );

    SwitchArg quietArg( "q", "quiet", "Limit output", cmd, false );

    ValueArg<string> fileArg( "", "file", "File of image data to save or load", false, " ", "string", cmd);

    ValueArg<string> tessellateTypeArg( "", "type", "Tessellate type pattern file", false, "tessellation/1.t", "string", cmd);

    ValueArg<int> imageTileArg( "i", "imageTileWidth", "Tile width for generating image", false, 0, "int", cmd);

    ValueArg<int> mosaicTileHeightArg( "l", "mosaicTileHeight", "Maximum tile height for generating mosaic", false, 0, "int", cmd);

    ValueArg<int> mosaicTileWidthArg( "m", "mosaicTileWidth", "Maximum tile width for generating mosaic", false, 0, "int", cmd);

    SwitchArg smoothArg( "", "smooth", "Compute edge preserving smoothing before finding edges", cmd, false );

    ValueArg<float> edgeWeightArg( "e", "edgeWeight", "Weight edge pixels more than other pixels", false, 1.0, "float", cmd);

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
    bool smoothImage                  = smoothArg.getValue();
    float edgeWeight                  = edgeWeightArg.getValue();
    bool flip                         = flipArg.getValue();
    bool spin                         = spinArg.getValue();
    bool quiet                        = quietArg.getValue();
    bool recursiveSearch              = recursiveArg.getValue();
    int cropStyle                     = cropStyleArg.getValue();
    int mosaicTileWidth               = mosaicTileWidthArg.getValue();
    int mosaicTileHeight              = mosaicTileHeightArg.getValue();
    int imageTileWidth                = imageTileArg.getValue();
    string tessellateType             = tessellateTypeArg.getValue();
    string fileName                   = fileArg.getValue();

    if( VIPS_INIT( argv[0] ) ) return( -1 );

    RunTessellate( inputName, outputName, inputDirectory, numHorizontal, cropStyle, flip, spin, mosaicTileWidth, mosaicTileHeight, imageTileWidth, repeat, fileName, quiet, recursiveSearch, tessellateType, edgeWeight, smoothImage );
  }
  catch (ArgException &e)  // catch any exceptions
  {
    cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
  }
  return 0;
}