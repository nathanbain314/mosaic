#include "mosaic.h"

using namespace TCLAP;

int main( int argc, char **argv )
{
  try
  {
    CmdLine cmd("Creates an image mosaic.", ' ', "1.0");

    ValueArg<string> directoryArg( "d", "directory", "Output directory name", false, "output", "string", cmd);

    SwitchArg buildSwitch("b","build","Build directory of zoomable images", cmd, false);

    ValueArg<double> shrinkArg( "s", "shrink", "Factor to shrink reference image by", false, 1, "double", cmd);

    ValueArg<string> outputArg( "o", "output", "Output name", true, "out", "string", cmd);

    ValueArg<string> imagesArg( "i", "images", "Directory of images to use to build mosaic", true, "images", "string", cmd);

    ValueArg<string> pictureArg( "p", "picture", "Reference picture to create mosaic from", true, "in", "string", cmd);

    cmd.parse( argc, argv );

    string input_image      = pictureArg.getValue();
    string output_image     = outputArg.getValue();
    string image_directory  = imagesArg.getValue();
    string output_directory = directoryArg.getValue();
    bool build              = buildSwitch.getValue();
    double shrink           = shrinkArg.getValue();

    if( vips_init( argv[0] ) )
      vips_error_exit( NULL ); 
    vips_cache_set_max(0);
    vips_cache_set_max_mem( 10 );
    //vips_leak_set(TRUE);

    // Directory has to end in a slash in order to work
    if( image_directory.back() != '/' ) image_directory += '/';
    if( output_directory.back() != '/' ) output_directory += '/';

    pair< PointCloud, vector< VImage > > both  = create_image_array ( image_directory, output_directory, build );
    PointCloud averages = both.first;
    vector< VImage > thumbnails = both.second;

    if( vips_foreign_find_save((char *)output_image.c_str()) == NULL )
      generate_zoomable_image( averages, thumbnails, input_image, output_image, shrink);
    else
      generate_image( averages, thumbnails, input_image, output_image, shrink);

    vips_shutdown();
  }
  catch (ArgException &e)  // catch any exceptions
  {
    cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
  }

  return 0;
}