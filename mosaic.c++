#include "CImg/CImg.h"
#include <stdio.h>
#include <string>
#include <iostream>
#include <vector>
#include <sys/ioctl.h>

using namespace cimg_library;
using namespace std;

int size;

class Set
{
  public:
    int ***condensed_image;
    int average_color[3];
    Set ( int s )
    {
      condensed_image = new int**[s];
      for ( int i = 0; i < s; ++i)
      {
          condensed_image[i] = new int*[s];
          for ( int j = 0; j < s; ++j)
          {
            condensed_image[i][j] = new int[3];
          }
      }
    }
};

vector< Set > create_image_array( string image_directory )
{
  CImg<unsigned char> image;
  DIR *dir;
  struct dirent *ent;
  string str;
  int n = 0;
  int sum_r, sum_g, sum_b, s, s_sqr, size_sqr;
  int total_r, total_g, total_b;

  vector< Set > set_vector = vector< Set >();

	int num_images = 0;

  size_sqr = size * size;
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

	int progress_bar_size = num_images / 40;
	int pos = 0;

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
        image.load(str.c_str());

        // Get the length of the shortest side
        s = image.width() < image.height() ? image.width() : image.height();

        // If the image is at least large enough to make the array
        if( s >= size && image.spectrum() == 3)
        {
          s = s/size;
          s_sqr = s * s;

          total_r = 0; total_g = 0; total_b = 0;

          set_vector.push_back( Set( size ) );

          for ( int i = 0; i < size; ++i)
          {
            for ( int j = 0; j < size; ++j)
            {
              sum_r = 0; sum_g = 0; sum_b = 0;
              for(int y = 0; y < s; y++)
              {
                for(int x = 0; x < s; x++)
                {
                  sum_r += (int)*image.data( j*s+x, i*s+y, 0, 0);
                  sum_g += (int)*image.data( j*s+x, i*s+y, 0, 1);
                  sum_b += (int)*image.data( j*s+x, i*s+y, 0, 2);
                }
              }
              total_r += sum_r;
              total_g += sum_g;
              total_b += sum_b;

              set_vector[n].condensed_image[i][j][0] = sum_r / s_sqr;
              set_vector[n].condensed_image[i][j][1] = sum_g / s_sqr;
              set_vector[n].condensed_image[i][j][2] = sum_b / s_sqr;
            }
          }
          set_vector[n].average_color[0] = total_r / (size_sqr*s_sqr);
          set_vector[n].average_color[1] = total_g / (size_sqr*s_sqr);
          set_vector[n++].average_color[2] = total_b / (size_sqr*s_sqr);
        }
				++pos;
			  struct winsize ww; 
			  ioctl(STDOUT_FILENO, TIOCGWINSZ, &ww);
			  int space_left = ww.ws_col - 26;
				fprintf(stderr,"\rProcessing images: %d%%  ", (100*pos)/num_images);
				for(int ii = 0; ii < (space_left*pos)/num_images; ii++) fprintf(stderr, "#");
				for(int ii = (space_left*pos)/num_images; ii < space_left; ii++) fprintf(stderr, " ");
      }
    }
		printf("\n");
    closedir (dir);
  }
  return set_vector;
}

int best_match ( int i, int j, CImg<unsigned char> image, vector< Set > set_vector, int factor)
{
  double min_difference = DBL_MAX;
  int min = 0;
  double total_difference = 0;
  int r = 0, g = 0, b = 0;
  int size_sqr = size * size;
  int n = set_vector.size();
  int width = image.width();
  int height = image.height();

  int number = 0;

  for(int y = 0, h = i; y < size/factor && h < height; ++y, ++h)
  {
    for(int x = 0, w = j; x < size/factor && w < width; ++x, ++w, ++number)
    {
      r += (int)*image.data(w, h,0,0);
      g += (int)*image.data(w, h,0,1);
      b += (int)*image.data(w, h,0,2);
    }
  }
  for (int v = 0; v < n; v++)
  {
    total_difference = sqrt(pow(r/number-set_vector[v].average_color[0],2)+pow(g/number-set_vector[v].average_color[1],2)+pow(b/number-set_vector[v].average_color[2],2));
    if( total_difference < min_difference )
    {
      min_difference = total_difference;
      min = v;
    }
  }
  return min;
}

int main( int argc, char **argv )
{
  char * input_image = argv[1];
  char * output_image = argv[2];
  string image_directory = argv[3];
  int factor = atoi(argv[4]);
	size = atoi(argv[5]);

  CImg<unsigned char> image( input_image );
	CImg<unsigned char> output(image.width()*factor,image.height()*factor,1,3,0);

  vector< Set > set_vector = create_image_array ( image_directory );

  int width = image.width();
  int height = image.height();
	int pos = 0;
  int total = width*height*factor*factor / (size*size);

  for (int i = 0; i < height; i+=size/factor)
  {
    for (int j = 0; j < width; j+=size/factor)
    {
      int match = best_match ( i, j, image, set_vector, factor);
			++pos;
    	for(int y = 0, h = i*factor; y < size && h < height*factor; ++y, ++h)
    	{
      	for(int x = 0, w = j*factor; x < size && w < width*factor; ++x, ++w)
      	{
          output(w, h, 0, 0) = set_vector[match].condensed_image[y][x][0];
          output(w, h, 0, 1) = set_vector[match].condensed_image[y][x][1];
          output(w, h, 0, 2) = set_vector[match].condensed_image[y][x][2];
      	}
    	}
      struct winsize ww; 
      ioctl(STDOUT_FILENO, TIOCGWINSZ, &ww);
      int space_left = ww.ws_col - 26; 
      fprintf(stderr,"\rBuilding mosaic:   %d%%  ", (100*pos)/total);
      for(int ii = 0; ii < (space_left*pos)/total; ii++) fprintf(stderr, "#");
      for(int ii = (space_left*pos)/total; ii < space_left; ii++) fprintf(stderr, " ");
  	}
  }
	printf("\n");
  
  output.save(output_image);

  return 0;
}

