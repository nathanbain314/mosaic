#include "mosaic.h"
#include "progressbar.h"
#include "nanoflann.hpp"

using namespace nanoflann;

// Takes string of image directory location and returns vector of average color for each image
pair< PointCloud, vector< VImage > > create_image_array( string image_directory, string output_directory, bool build )
{
  DIR *dir;
  struct dirent *ent;
  string str;
  int n = 0;
  int total_r, total_g, total_b;

  PointCloud cloud;
  vector< VImage > thumbnails;

  int num_images = 0;

  // Count the number of valid image files in the directory
  if ((dir = opendir (image_directory.c_str())) != NULL) 
  {
    while ((ent = readdir (dir)) != NULL)
    {   
      str = image_directory;
      if( ent->d_name[0] != '.' )
      {
        try
        {
          str.append(ent->d_name);
          VImage::vipsload( (char *)str.c_str() );
          ++num_images;
        }
        catch (...) {}
      }
    }
  }

  int p = 0;
  unsigned char * data;
  int size, size_sqr, width, height;
  int offset;

  progressbar *processing_images = progressbar_new("Processing images", num_images);

  // Create thumbnails direcoty
  g_mkdir("thumbnails", 0777);

  // Iterate through all images in directory
  if ((dir = opendir (image_directory.c_str())) != NULL) 
  {
    while ((ent = readdir (dir)) != NULL)
    {
      str = image_directory;

      if( ent->d_name[0] != '.' )
      {
        VImage image;
        try
        {
          // Load the image
          str.append(ent->d_name);
          
          image = VImage::vipsload( (char *)str.c_str() );
        }
        catch(...)
        {
          // If it is not a valid image then coninue
          continue;
        }

        // Find the width of the largest square
        width = image.width();
        height = image.height();
        size = ( width < height ) ? width : height;

        // Square must be at least 64 by 64 pixels
        if(size >= 64 && image.bands() >= 3 )
        {
          stringstream ss;
          ss << "thumbnails/" << p << ".v";

          // Build the zoomable image if necessary 
          if( build )
          {
            stringstream ss2;
            ss2 << output_directory << p;
            if(width < height) 
              image.extract_area(0, (height-size)/2, size, size).dzsave((char *)ss2.str().c_str());
            else
              image.extract_area((width-size)/2, 0, size, size).dzsave((char *)ss2.str().c_str());
          }

          // Create and save a 64 by 64 pixel thumbnail
          char * thumbname = (char *)ss.str().c_str();
          if(width < height) 
          {
            image.
            extract_area(0, (height-size)/2, size, size).
            shrink((double)size/64.0, (double)size/64.0).
            vipssave(thumbname);
          }
          else
          {
            image.
            extract_area( (width-size)/2, 0, size, size).
            shrink((double)size/64.0, (double)size/64.0).
            vipssave(thumbname);
          }

          // Push the thumbnail onto the thumbnail vector
          image = VImage::vipsload(thumbname);
          thumbnails.push_back(image);   

          // Save the average color data
          double *da = (double *)image.stats().data();

          PointCloud::Point pt;
          pt.r = (int)da[14];
          pt.g = (int)da[24];
          pt.b = (int)da[34];
          cloud.pts.push_back(pt);
          
          ++p;
        }
        progressbar_inc( processing_images );
      }
    }

    progressbar_finish( processing_images );

    closedir (dir);
  }
  return pair< PointCloud, vector< VImage > >(cloud, thumbnails);
}

// Generate thumbnails based on previous averages
void generate_thumbnail( PointCloud &averages, string input_image, string output_image, double shrink, bool square)
{
  // Shrink the image by an amount first
  VImage image = (shrink > 1) ? VImage::vipsload( (char *)input_image.c_str() ).shrink(shrink, shrink) : 
                                VImage::vipsload( (char *)input_image.c_str() );

  // Convert to a square
  if(square)
  {
    image = (image.width() < image.height()) ? image.extract_area(0, (image.height()-image.width())/2, image.width(), image.width()) :
                                               image.extract_area((image.width()-image.height())/2, 0, image.height(), image.height());
  }

  unsigned char * image_data = (unsigned char *)image.data();
  
  int width = image.width();
  int height = image.height();
  
  // Build kd tree
  typedef KDTreeSingleIndexAdaptor< L2_Simple_Adaptor<int, PointCloud >, PointCloud, 3 > my_kd_tree_t;

  my_kd_tree_t index(3 , averages, KDTreeSingleIndexAdaptorParams(20) );
  index.buildIndex();

  size_t ret_index;
  int out_dist_sqr;
  nanoflann::KNNResultSet<int> resultSet(1);

  // Get closest color from kd tree and recolor the image
  ofstream csv_data;
  for (int i = 0, p = 0; i < height; ++i)
  {
    for (int j = 0; j < width; ++j, p+=3)
    {
      resultSet.init(&ret_index, &out_dist_sqr );
      int query_pt[3] = { (int)image_data[p], (int)image_data[p+1], (int)image_data[p+2]};
      index.findNeighbors(resultSet, &query_pt[0], nanoflann::SearchParams(20));

      image_data[p] = averages.pts[ret_index].r;
      image_data[p+1] = averages.pts[ret_index].g;
      image_data[p+2] = averages.pts[ret_index].b;
    }
  }

  image.vipssave((char *)output_image.c_str());
}

// Generate a zoomable mosaic
void generate_zoomable_image( PointCloud &q, vector< VImage > thumbnails, string input_image, string output_image, double shrink, bool square)
{
  // Shrink the input image is necessary
  VImage image = (shrink > 1) ? VImage::vipsload( (char *)input_image.c_str() ).shrink(shrink, shrink) : 
                                VImage::vipsload( (char *)input_image.c_str() );

  // Convert to a square
  if(square)
  {
    image = (image.width() < image.height()) ? image.extract_area(0, (image.height()-image.width())/2, image.width(), image.width()) :
                                               image.extract_area((image.width()-image.height())/2, 0, image.height(), image.height());
  }

  unsigned char * image_data = (unsigned char *)image.data();
  
  int width = image.width();
  int height = image.height();
  
  int p = 0;
  int best;

  int **mosaic = new int*[height];

  // Build kd tree
  typedef KDTreeSingleIndexAdaptor< L2_Simple_Adaptor<int, PointCloud >, PointCloud, 3 > my_kd_tree_t;

  my_kd_tree_t index(3 , q, KDTreeSingleIndexAdaptorParams(20) );
  index.buildIndex();

  size_t ret_index;
  int out_dist_sqr;
  nanoflann::KNNResultSet<int> resultSet(1);

  // Iterate through every pixel of image and find the closest color
  ofstream csv_data;
  csv_data.open(string(output_image).append(".csv").c_str());
  for (int i = 0; i < height; ++i)
  {
    mosaic[i] = new int[width];
    for (int j = 0; j < width; ++j, p+=3)
    {
      resultSet.init(&ret_index, &out_dist_sqr );
      int query_pt[3] = { (int)image_data[p], (int)image_data[p+1], (int)image_data[p+2]};
      index.findNeighbors(resultSet, &query_pt[0], nanoflann::SearchParams(20));

      // Set the value in the array and write to the csv file
      mosaic[i][j] = ret_index;
      csv_data << ret_index << ",";
    }
    csv_data << endl;
  }

  csv_data.close();

  // Create the dzi file 
  ofstream dzi_file;
  dzi_file.open(string(output_image).append(".dzi").c_str());
  dzi_file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
  dzi_file << "<Image xmlns=\"http://schemas.microsoft.com/deepzoom/2008\" Format=\"jpeg\" Overlap=\"0\" TileSize=\"256\">" << endl;
  dzi_file << "    <Size Height=\"" << height*64 << "\" Width=\"" << width*64 << "\"/>" << endl;
  dzi_file << "</Image>" << endl;
  dzi_file.close();

  int log_dzi = (int)ceil( log2((width > height) ? width : height ) );
  
  int num_tiles = 256 >> 6;
  int i_end = ceil(height / num_tiles);
  int j_end = ceil(width / num_tiles);
  ostringstream ss;
  ss << output_image << "_files/";

  // Make the directory for the zoomable image
  g_mkdir(ss.str().c_str(), 0777);
  ss << log_dzi+6 << "/";
  g_mkdir(ss.str().c_str(), 0777);

  // Build the lowest level by stitching images from the thumbnails
  for(int i = 0; i <= i_end; ++i)
  {
    int y_end = ((i+1)*num_tiles < height) ? (i+1)*num_tiles : height;
    for(int j = 0; j <= j_end; ++j)
    {
      int x_end = ((j+1)*num_tiles < width) ? (j+1)*num_tiles : width;
      int size_length = x_end - j*num_tiles;
      if(y_end - i*num_tiles > 0 && size_length > 0)
      {
        vector<VImage> lines;
        for(int y = i*num_tiles; y < y_end; ++y)
        {
          for( int x = j*num_tiles; x < x_end; ++x)
          {
            lines.push_back(thumbnails[mosaic[y][x]]);
          }
        }
        ostringstream ss2;
        ss2 << ss.str() << j << "_" << i << ".jpeg";
        VImage::arrayjoin(lines, VImage::option()->set( "across", size_length )).jpegsave((char *)ss2.str().c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );
      }
    }
  }
  
  // Progressively build the image pyramid from the bottom up
  for(int zoom = 5; zoom >= -log_dzi; --zoom )
  {
    int w = (int)ceil((double)width/ (double)(1 << (7-zoom)));
    int h = (int)ceil((double)height/ (double)(1 << (7-zoom)));
    ostringstream ss2, ss3;
    ss3 << output_image << "_files/" << log_dzi+zoom << "/";
    ss2 << output_image << "_files/" << log_dzi+zoom+1 << "/";
    g_mkdir(ss3.str().c_str(), 0777);

    // Take four images, shrink by a factor of two, and combine into a combination image
    for(int i = 1; i < w; i+=2)
    {
      for(int j = 1; j < h; j+=2)
      {
        (VImage::jpegload((char *)ss2.str().append(to_string(i-1)+"_"+to_string(j-1)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
        join(VImage::jpegload((char *)ss2.str().append(to_string(i)+"_"+to_string(j-1)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL)).
        join(VImage::jpegload((char *)ss2.str().append(to_string(i-1)+"_"+to_string(j)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
        join(VImage::jpegload((char *)ss2.str().append(to_string(i)+"_"+to_string(j)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL),VIPS_DIRECTION_VERTICAL).
        jpegsave((char *)ss3.str().append(to_string(i>>1)+"_"+to_string(j>>1)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );
      }
    }
    // Side cases
    if(w%2 == 1)
    {
      for(int j = 1; j < h; j+=2)
      {
        (VImage::jpegload((char *)ss2.str().append(to_string(w-1)+"_"+to_string(j-1)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
        join(VImage::jpegload((char *)ss2.str().append(to_string(w-1)+"_"+to_string(j)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_VERTICAL)).
        jpegsave((char *)ss3.str().append(to_string(w>>1)+"_"+to_string(j>>1)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );
      }
    }
    if(h%2 == 1)
    {
      for(int j = 1; j < w; j+=2)
      {
        (VImage::jpegload((char *)ss2.str().append(to_string(j-1)+"_"+to_string(h-1)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
        join(VImage::jpegload((char *)ss2.str().append(to_string(j)+"_"+to_string(h-1)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL)).
        jpegsave((char *)ss3.str().append(to_string(j>>1)+"_"+to_string(h>>1)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );
      }
    }
    // Corner cases
    if(w%2 == 1 && h%2 == 1)
    {
        VImage::jpegload((char *)ss2.str().append(to_string(w-1)+"_"+to_string(h-1)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
        jpegsave((char *)ss3.str().append(to_string(w>>1)+"_"+to_string(h>>1)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );  
    }
  }
}

// Build a static nonzoomable image
void generate_image( PointCloud &q, vector< VImage > thumbnails, string input_image, string output_image, double shrink)
{
  VImage image = (shrink > 1) ? VImage::vipsload( (char *)input_image.c_str() ).shrink(shrink, shrink) : 
                                VImage::vipsload( (char *)input_image.c_str() );

  unsigned char * image_data = (unsigned char *)image.data();
  
  int width = image.width();
  int height = image.height();
  
  vector<VImage> lines;

  typedef KDTreeSingleIndexAdaptor< L2_Simple_Adaptor<int, PointCloud >, PointCloud, 3 > my_kd_tree_t;

  my_kd_tree_t index( 3, q, KDTreeSingleIndexAdaptorParams(20) );
  index.buildIndex();

  size_t ret_index;
  int out_dist_sqr;
  nanoflann::KNNResultSet<int> resultSet(1);

  // Create a vector of all of the necessary images
  for (int i = 0, p = 0; i < height; ++i)
  {
    for (int j = 0; j < width; ++j, p+=3)
    {
      resultSet.init(&ret_index, &out_dist_sqr );
      int query_pt[3] = { (int)image_data[p], (int)image_data[p+1], (int)image_data[p+2]};
      index.findNeighbors(resultSet, &query_pt[0], nanoflann::SearchParams(20));

      lines.push_back(thumbnails[ret_index]);
    }
  }

  // Join the images into the final image
  VImage::arrayjoin(lines, VImage::option()->set( "across", width )).vipssave((char *)output_image.c_str());
}