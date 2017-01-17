#include "mosaic.h"
#include "progressbar.h"

using namespace TCLAP;

int main( int argc, char **argv )
{
  try
  {
    CmdLine cmd("Creates a continuous image mosaic.", ' ', "1.0");

    ValueArg<string> directoryArg( "d", "directory", "Output directory name", true, "output", "string", cmd);

    ValueArg<string> imagesArg( "i", "images", "Directory of images to use to build mosaic", true, "images", "string", cmd);

    cmd.parse( argc, argv );

    string image_directory  = imagesArg.getValue();
    string output_directory = directoryArg.getValue();

    // Directory has to end in a slash in order to work
    if( image_directory.back() != '/' ) image_directory += '/';
    if( output_directory.back() != '/' ) output_directory += '/';

    if( vips_init( argv[0] ) )
      vips_error_exit( NULL ); 
    vips_cache_set_max(0);
    //vips_leak_set(TRUE);

    pair< PointCloud, vector< VImage > > both  = create_image_array ( image_directory, output_directory );
    PointCloud averages = both.first;

    DIR *dir;
    struct dirent *ent;
    string str;
    int n = 0;
    int total_r, total_g, total_b;

    int num_images = 0;

    if ((dir = opendir (image_directory.c_str())) != NULL) 
    {
      while ((ent = readdir (dir)) != NULL)
      {   
        str = image_directory;
        if( ent->d_name[0] != '.' )
        {
          ++num_images;
        }
      }
    }

    int p = 0;
    int size, size_sqr, width, height;
    int offset;
    vector< VImage > thumbnails;
    progressbar *generating_thumbnails = progressbar_new("Generating thumbnails", num_images);

    // Iterate through all images in directory
    if ((dir = opendir (image_directory.c_str())) != NULL) 
    {
      while ((ent = readdir (dir)) != NULL)
      {
        str = image_directory;
        if( ent->d_name[0] != '.' )
        {
          // Load the image
          str.append(ent->d_name);
          
          VImage image = VImage::vipsload( (char *)str.c_str() );
          width = image.width();
          height = image.height();
          size = ( width < height ) ? width : height;

          // Generate new thumbnails based on image averages
          if(size >= 64 && image.bands() == 3)
          {
            stringstream ss;
            ss << "thumbnails/" << p << ".v";
            generate_thumbnail( averages, str, ss.str(), (double)size/64.0, true);
            thumbnails.push_back(VImage::vipsload((char *)ss.str().c_str()));
       
            ++p;
            progressbar_inc( generating_thumbnails );
          }
        }
      }
      closedir (dir);
    }
    progressbar_finish( generating_thumbnails );

    progressbar *building_mosaics = progressbar_new("Building mosaics", num_images);
    p = 0;

    // Iterate through all images in directory
    if ((dir = opendir (image_directory.c_str())) != NULL) 
    {
      while ((ent = readdir (dir)) != NULL)
      {
        str = image_directory;
        if( ent->d_name[0] != '.' )
        {
          // Load the image
          str.append(ent->d_name);
          
          VImage image = VImage::vipsload( (char *)str.c_str() );
          width = image.width();
          height = image.height();
          size = ( width < height ) ? width : height;

          // Create zoomable mosaics
          if(size >= 64 && image.bands() == 3)
          {
            stringstream ss;
            ss << "mosaics/" << p;
            generate_zoomable_image( averages, thumbnails, str, ss.str(), size/64, true);
       
            ++p;
          }

          progressbar_inc( building_mosaics );
        }
      }
      closedir (dir);
    }
    progressbar_finish( building_mosaics );

    vips_shutdown();
  }
  catch (ArgException &e)  // catch any exceptions
  {
    cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
  }

  return 0;
}