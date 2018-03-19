#include "mosaic.h"

void generateRotationalThumbnails( string imageDirectory, vector< vector< unsigned char > > &images, vector< vector< unsigned char > > &masks, vector< pair< int, int > > &dimensions, double scale, double renderScale )
{
  DIR *dir;
  struct dirent *ent;
  string str;

  int num_images = 0;

  vector< string > names;

  cout << "Reading directory " << imageDirectory << endl;

  // Count the number of valid image files in the directory
  if ((dir = opendir (imageDirectory.c_str())) != NULL) 
  {
    while ((ent = readdir (dir)) != NULL)
    {   
      if( ent->d_name[0] != '.' && vips_foreign_find_load( string( imageDirectory + ent->d_name ).c_str() ) != NULL )
      {
        // Saves the full directory position of valid images
        names.push_back( imageDirectory + ent->d_name );
        cout << "\rFound " << ++num_images << " images " << flush;
      }
    }
  }

  cout << endl;

  ProgressBar *processing_images = new ProgressBar(num_images, "Processing images");
  unsigned char *testData, *maskData;

  // Iterate through all images in directory
  for( int i = 0; i < num_images; ++i )
  {
    try
    {
      str = names[i];

      // Load image and create white one band image of the same size
      VImage image = VImage::vipsload( (char *)str.c_str() );
      VImage white = VImage::black(image.width(),image.height()).invert();

      // Turn grayscale images into 3 band or 4 band images
      if( image.bands() == 1 )
      {
        image = image.bandjoin(image).bandjoin(image);
      }
      else if( image.bands() == 2 )
      {
        VImage alpha = image.extract_band(3);
        image = image.flatten().bandjoin(image).bandjoin(image).bandjoin(alpha);
      }

      processing_images->Increment();

      int width = image.width();
      int height = image.height();

      // Create image and mask of scale size
      VImage testImage = image.similarity(VImage::option()->set("scale",scale));
      VImage mask = white.similarity(VImage::option()->set("scale",scale));

      width = testImage.width();
      height = testImage.height();

      // Extract mask of alpha image
      if( image.bands() == 4 )
      {
        mask = testImage.extract_band(3);
        
        testImage = testImage.flatten();
      }

      // Save data to vector
      testData = ( unsigned char * )testImage.data();
      maskData = ( unsigned char * )mask.data();

      images.push_back( vector< unsigned char >(testData, testData + (3*width*height)));
      masks.push_back( vector< unsigned char >(maskData, maskData + (width*height)));
      dimensions.push_back( pair< int, int >(width,height) );

      width = image.width();
      height = image.height();

      // Do the same for image of renderscale size
      testImage = image.similarity(VImage::option()->set("scale",renderScale));
      mask = white.similarity(VImage::option()->set("scale",renderScale));

      width = testImage.width();
      height = testImage.height();

      if( image.bands() == 4 )
      {
        mask = testImage.extract_band(3);
        
        testImage = testImage.flatten();
      }

      testData = ( unsigned char * )testImage.data();
      maskData = ( unsigned char * )mask.data();

      images.push_back( vector< unsigned char >(testData, testData + (3*width*height)));
      masks.push_back( vector< unsigned char >(maskData, maskData + (width*height)));
      dimensions.push_back( pair< int, int >(width,height) );
    }
    catch (...)
    {
    }
  }

  cout << endl;
}

double cosAngle, sinAngle, halfWidth, halfHeight;

int rotateX( double x, double y )
{
  return cosAngle*(x-halfWidth) - sinAngle*(y-halfHeight) + halfWidth;
}

int rotateY( double x, double y )
{
  return sinAngle*(x-halfWidth) + cosAngle*(y-halfHeight) + halfHeight;
}

void buildRotationalImage( string inputImage, string outputImage, vector< vector< unsigned char > > &images, vector< vector< unsigned char > > &masks, vector< pair< int, int > > &dimensions, double resize, int numIter, int angleOffset )
{
  VImage image = VImage::new_memory().vipsload( (char *)inputImage.c_str() );

  // Converts 1 and 4 band images to a 3 band image
  if( image.bands() == 1 )
  {
    image = image.bandjoin(image).bandjoin(image);
  }
  if( image.bands() == 4 )
  {
    image = image.flatten();
  }

  int imageWidth = image.width();
  int imageHeight = image.height();

  unsigned char * imageData = ( unsigned char * )image.data();

  // Scaled output sizes
  int outputWidth = ceil(imageWidth*resize);
  int outputHeight = ceil(imageHeight*resize);

  // Size of output data
  unsigned long long outputSize = (unsigned long long)outputWidth*(unsigned long long)outputHeight*3ULL;

  // Create black images of output and masks
  unsigned char * outputData = ( unsigned char * )calloc (outputSize,sizeof(unsigned char));
  unsigned char * computeMaskData = ( unsigned char * )calloc (imageWidth*imageHeight,sizeof(unsigned char));
  unsigned char * outputMaskData = ( unsigned char * )calloc ((unsigned long long)outputWidth*(unsigned long long)outputHeight,sizeof(unsigned char));

  ProgressBar *processing_images = new ProgressBar(numIter, "Building image");

  // Run for every iteration
  for( int inc = 0; inc < numIter; ++inc )
  {
    // Place image at random point
    int y = rand()%(imageHeight);
    int x = rand()%(imageWidth);
  
    processing_images->Increment();

    int bestImage = -1;
    double bestDifference = DBL_MAX, bestAngle = 0;

    // Run through every image 
    for( int k = 0; k < images.size(); k+=2 )
    {
      // Rotate image for every angleOffset angles
      for( double a = 0; a < 360; a += angleOffset )
      {
        // Convert to radians
        double angle = a * 3.14159265/180.0;
        int width = dimensions[k].first;
        int height = dimensions[k].second;

        // Compute data for point rotation
        cosAngle = cos(angle);
        sinAngle = sin(angle);
        halfWidth = (double)width/2.0;
        halfHeight = (double)height/2.0;

        // Conpute side offset of rotated image from regular image
        int xOffset = max( rotateX(0,0), max( rotateX(width,0), max( rotateX(0,height), rotateX(width,height) ) ) ) - width;
        int yOffset = max( rotateY(0,0), max( rotateY(width,0), max( rotateY(0,height), rotateY(width,height) ) ) ) - height;

        // New width and height of rotated image
        int newWidth = width + 2*xOffset;
        int newHeight = height + 2*yOffset;

        double difference = 0;
        double usedPixels = 0;

        // New data for point rotation in reverse direction
        cosAngle = cos(-angle);
        sinAngle = sin(-angle);
        halfWidth = (double)newWidth/2.0;
        halfHeight = (double)newHeight/2.0;

        // Traverse data for rotated image to find difference from input image
        for( int i = 0; i < newHeight; ++i )
        {
          for( int j = 0; j < newWidth; ++j )
          {
            // New x and y of rotated image point
            int newX = rotateX(j,i) - xOffset;
            int newY = rotateY(j,i) - yOffset;

            // Make sure that the point is within the image
            if( (newX < 0) || (newX > width-1) || (newY < 0) || (newY > height-1) ) continue;

            // Point in image
            int p = width*newY + newX;

            // Make sure that the point is not transparent
            if( masks[k][p] == 0 ) continue;

            // Point in input image
            int ix = x + j - newWidth/2;
            int iy = y + i - newHeight/2;

            // Make sure that the point is within the input image
            if( (ix < 0) || (ix > imageWidth-1) || (iy < 0) || (iy > imageHeight-1) ) continue;

            // Index of point in input image
            unsigned long long index = (unsigned long long)iy*(unsigned long long)imageWidth+(unsigned long long)ix;

            // Make sure the the point is not fully colored
            if( computeMaskData[index] == 255 ) continue;

            // Update the number of pixels that are used
            ++usedPixels;

            // Get the input image rgb values
            unsigned char ir = imageData[3ULL*index+0ULL];
            unsigned char ig = imageData[3ULL*index+1ULL];
            unsigned char ib = imageData[3ULL*index+2ULL];

            // Get the image rgb values
            unsigned char tr = images[k][3*p+0];
            unsigned char tg = images[k][3*p+1];
            unsigned char tb = images[k][3*p+2];

            // Compute the sum-of-squares for color difference
            difference += (ir-tr)*(ir-tr) + (ig-tg)*(ig-tg) + (ib-tb)*(ib-tb);
          }
        }

        // If the image is more similar then choose this one as the best image and angle
        if( difference/usedPixels < bestDifference )
        {
          bestDifference = difference/usedPixels;
          bestImage = k;
          bestAngle = angle;
        }
      }
    }

    // If no image was chosen then skip
    if( bestImage == -1 ) continue;

    // Set the angle and offsets like before
    int width = dimensions[bestImage].first;
    int height = dimensions[bestImage].second;

    cosAngle = cos(bestAngle);
    sinAngle = sin(bestAngle);
    halfWidth = (double)width/2.0;
    halfHeight = (double)height/2.0;

    int xOffset = max( rotateX(0,0), max( rotateX(width,0), max( rotateX(0,height), rotateX(width,height) ) ) ) - width;
    int yOffset = max( rotateY(0,0), max( rotateY(width,0), max( rotateY(0,height), rotateY(width,height) ) ) ) - height;

    int newWidth = width + 2*xOffset;
    int newHeight = height + 2*yOffset;

    cosAngle = cos(-bestAngle);
    sinAngle = sin(-bestAngle);
    halfWidth = (double)newWidth/2.0;
    halfHeight = (double)newHeight/2.0;

    // Set the mask value for the input image mask
    for( int i = 0; i < newHeight; ++i )
    {
      for( int j = 0; j < newWidth; ++j )
      {
        int newX = rotateX(j,i) - xOffset;
        int newY = rotateY(j,i) - yOffset;

        if( (newX < 0) || (newX > width-1) || (newY < 0) || (newY > height-1) ) continue;

        int p = width*newY + newX;

        if( masks[bestImage][p] == 0 ) continue;

        int ix = x + j - newWidth/2;
        int iy = y + i - newHeight/2;

        if( (ix < 0) || (ix > imageWidth-1) || (iy < 0) || (iy > imageHeight-1) ) continue;

        unsigned long long index = (unsigned long long)iy*(unsigned long long)imageWidth+(unsigned long long)ix;

        computeMaskData[index] = min((int)computeMaskData[index] + (int)masks[bestImage][p],255);
      }
    }

    // Set the values for the output image
    width = dimensions[bestImage+1].first;
    height = dimensions[bestImage+1].second;

    cosAngle = cos(bestAngle);
    sinAngle = sin(bestAngle);
    halfWidth = (double)width/2.0;
    halfHeight = (double)height/2.0;

    xOffset = max( rotateX(0,0), max( rotateX(width,0), max( rotateX(0,height), rotateX(width,height) ) ) ) - width;
    yOffset = max( rotateY(0,0), max( rotateY(width,0), max( rotateY(0,height), rotateY(width,height) ) ) ) - height;

    newWidth = width + 2*xOffset;
    newHeight = height + 2*yOffset;

    cosAngle = cos(-bestAngle);
    sinAngle = sin(-bestAngle);
    halfWidth = (double)newWidth/2.0;
    halfHeight = (double)newHeight/2.0;

    x*=resize;
    y*=resize;

    // Render the image onto the output image
    for( int i = 0; i < newHeight; ++i )
    {
      for( int j = 0; j < newWidth; ++j )
      {
        int r=0, g=0, b=0, m=0;
        int used = 0;

        //  Get the average of the rotated pixels 1/2 above, below, and to the sides for the average to get be a better color
        for( double k = -1; k <= 1; k += 2 )
        {
          int newX = rotateX((double)j+k/2.0,i) - xOffset;
          int newY = rotateY((double)j+k/2.0,i) - yOffset;

          if( (newX < 0) || (newX > width-1) || (newY < 0) || (newY > height-1) ) continue;

          int p = width*newY + newX;

          r+=images[bestImage+1][3*p+0];
          g+=images[bestImage+1][3*p+1];
          b+=images[bestImage+1][3*p+2];
          m+=masks[bestImage+1][p];
          ++used;

          newX = rotateX(j,(double)i+k/2.0) - xOffset;
          newY = rotateY(j,(double)i+k/2.0) - yOffset;

          if( (newX < 0) || (newX > width-1) || (newY < 0) || (newY > height-1) ) continue;

          p = width*newY + newX;

          r+=images[bestImage+1][3*p+0];
          g+=images[bestImage+1][3*p+1];
          b+=images[bestImage+1][3*p+2];
          m+=masks[bestImage+1][p];
          ++used;
        }

        int newX = rotateX(j,i) - xOffset;
        int newY = rotateY(j,i) - yOffset;

        if( (newX < 0) || (newX > width-1) || (newY < 0) || (newY > height-1) ) continue;

        int p = width*newY + newX;

        r+=images[bestImage+1][3*p+0];
        g+=images[bestImage+1][3*p+1];
        b+=images[bestImage+1][3*p+2];
        m+=masks[bestImage+1][p];
        ++used;

        // Divide by number of used pixels to get average color
        r/=used;
        g/=used;
        b/=used;
        m/=used;

        // Make sure that image pixel is not transparent
        if( masks[bestImage+1][p] == 0 ) continue;

        int ix = x + j - newWidth/2;
        int iy = y + i - newHeight/2;

        // Make sure that pixel is inside image
        if( (ix < 0) || (ix > outputWidth-1) || (iy < 0) || (iy > outputHeight-1) ) continue;

        // Set index
        unsigned long long index = (unsigned long long)iy*(unsigned long long)outputWidth+(unsigned long long)ix;

        // Make sure that output pixel is transparent
        if( outputMaskData[index] == 255 ) continue;

        // Set output colors
        outputData[3ULL*index+0ULL] = min(outputData[3ULL*index+0ULL]+int((double)(255-outputMaskData[index])*(double)r/255.0),255);
        outputData[3ULL*index+1ULL] = min(outputData[3ULL*index+1ULL]+int((double)(255-outputMaskData[index])*(double)g/255.0),255);
        outputData[3ULL*index+2ULL] = min(outputData[3ULL*index+2ULL]+int((double)(255-outputMaskData[index])*(double)b/255.0),255);
        
        // Set output transparency mask
        outputMaskData[index] = min(255,m+(int)outputMaskData[index]);

      }
    }
  }

  cout << endl;

  // Save image as static image or zoomable image
  if( vips_foreign_find_save( outputImage.c_str() ) != NULL )
  {
    VImage::new_from_memory( outputData, outputSize, outputWidth, outputHeight, 3, VIPS_FORMAT_UCHAR ).vipssave((char *)outputImage.c_str());
  }
  else
  {
    VImage::new_from_memory( outputData, outputSize, outputWidth, outputHeight, 3, VIPS_FORMAT_UCHAR ).dzsave((char *)outputImage.c_str());
  
    // Generate html file to view deep zoom image
    ofstream htmlFile(string(outputImage).append(".html").c_str());
    htmlFile << "<!DOCTYPE html>\n<html>\n<head><script src=\"js/openseadragon.min.js\"></script></head>\n<body>\n<style>\nhtml,\nbody,\n#rotationalMosaic\n{\nposition: fixed;\nleft: 0;\ntop: 0;\nwidth: 100%;\nheight: 100%;\n}\n</style>\n\n<div id=\"rotationalMosaic\"></div>\n\n<script>\nvar viewer = OpenSeadragon({\nid: 'rotationalMosaic',\nprefixUrl: 'icons/',\ntileSources:   \"" + outputImage + ".dzi\",\nminZoomImageRatio: 0,\nmaxZoomImageRatio: 1\n});\n</script>\n</body>\n</html>";
    htmlFile.close();
  }
}

void generateSquareThumbnails( vector< cropType > &cropData, vector< vector< unsigned char > > &mosaicTileData, vector< vector< unsigned char > > &imageTileData, string imageDirectory, int mosaicTileWidth, int imageTileWidth, bool exclude, bool spin, int cropStyle, bool flip )
{
  // Used for reading directory
  DIR *dir;
  struct dirent *ent;
  string str;

  // Used for processing image
  int size, xOffset, yOffset, width, height, mosaicTileArea = mosaicTileWidth*mosaicTileWidth*3, imageTileArea = imageTileWidth*imageTileWidth*3;
  bool different = mosaicTileWidth != imageTileWidth;

  unsigned char *c1, *c2;

  // Minimum size allowed: must be at least as large as mosaicTileWidth and imageTileWidth, and 256 if exclude is set
  int minAllow = exclude ? max(256,max(mosaicTileWidth,imageTileWidth)) : max(mosaicTileWidth,imageTileWidth);

  int num_images = 0;

  vector< string > names;

  cout << "Reading directory " << imageDirectory << endl;

  // Count the number of valid image files in the directory
  if ((dir = opendir (imageDirectory.c_str())) != NULL) 
  {
    while ((ent = readdir (dir)) != NULL)
    {   
      if( ent->d_name[0] != '.' && vips_foreign_find_load( string( imageDirectory + ent->d_name ).c_str() ) != NULL )
      {
        names.push_back( imageDirectory + ent->d_name );
        cout << "\rFound " << ++num_images << " images " << flush;
      }
    }
  }

  cout << endl;

  ProgressBar *processing_images = new ProgressBar(num_images, "Processing images");

  // Iterate through all images in directory
  for( int i = 0; i < num_images; ++i )
  {
    processing_images->Increment();
    try
    {
      str = names[i];

      // Get the width, height, and smallest and largest sizes
      width = VImage::new_memory().vipsload( (char *)str.c_str() ).width();
      height = VImage::new_memory().vipsload( (char *)str.c_str() ).height();
      int minSize = min(width,height);
      int maxSize = max(width,height);
      // Whether the a square is offset vertically or not
      bool vertical = width < height;

      if( minSize < minAllow )
      {
        continue;
      }

      // Width of shrunk images
      int sw = floor((double)maxSize*((double)mosaicTileWidth/(double)minSize));
      int lw = floor((double)maxSize*((double)imageTileWidth/(double)minSize));

      VImage image2, image;
      
      // Shrink the image and crop based on cropping style
      switch( cropStyle )
      {
        // Crop the middle square
        case 0:
          image = VImage::thumbnail((char *)str.c_str(),mosaicTileWidth,VImage::option()->set( "crop", VIPS_INTERESTING_CENTRE )->set( "size", VIPS_SIZE_DOWN ));
          break;
        // Crop the square with the most entropy
        case 1:
          image = VImage::thumbnail((char *)str.c_str(),mosaicTileWidth,VImage::option()->set( "crop", VIPS_INTERESTING_ENTROPY )->set( "size", VIPS_SIZE_DOWN ));
          break;
        // Crop the square most likely to 
        case 2:
          image = VImage::thumbnail((char *)str.c_str(),mosaicTileWidth,VImage::option()->set( "crop", VIPS_INTERESTING_ATTENTION )->set( "size", VIPS_SIZE_DOWN ));
          break;
        // Shrink the image to be cropped later
        case 3:
          // In order to crop image multiple times, gifs need to be cached with 16x16 tiles, and averything else needs to be cached with the full image
          image = vips_foreign_is_a("gifload",(char *)str.c_str()) ? 
                  VImage::thumbnail((char *)str.c_str(),ceil((double)maxSize*((double)mosaicTileWidth/(double)minSize)),VImage::option()->set( "size", VIPS_SIZE_DOWN )).cache(VImage::option()->set("tile_width",16)->set("tile_height",16)) : 
                  VImage::thumbnail((char *)str.c_str(),ceil((double)maxSize*((double)mosaicTileWidth/(double)minSize)),VImage::option()->set( "size", VIPS_SIZE_DOWN )).cache(VImage::option()->set("tile_width",ceil((double)width*((double)mosaicTileWidth/(double)minSize)))->set("tile_height",ceil((double)height*((double)mosaicTileWidth/(double)minSize))));
          break;
      }

      // Offset of crop
      xOffset = min(-(int)image.xoffset(),sw - mosaicTileWidth);
      yOffset = min(-(int)image.yoffset(),sw - mosaicTileWidth);

      // Convert image to 3 band image
      if( image.bands() == 1 )
      {
        image = image.bandjoin(image).bandjoin(image);
      }
      else if( image.bands() == 4 )
      {
        image = image.flatten();
      }

      // If the mosaicTileWidth and imageTileWidth are different then shrink and crop again
      if( different )
      {
        switch( cropStyle )
        {
          case 0:
            image2 = VImage::thumbnail((char *)str.c_str(),imageTileWidth,VImage::option()->set( "crop", VIPS_INTERESTING_CENTRE )->set( "size", VIPS_SIZE_DOWN ));
            break;
          case 1:
            image2 = VImage::thumbnail((char *)str.c_str(),imageTileWidth,VImage::option()->set( "crop", VIPS_INTERESTING_ENTROPY )->set( "size", VIPS_SIZE_DOWN ));
            break;
          case 2:
            image2 = VImage::thumbnail((char *)str.c_str(),imageTileWidth,VImage::option()->set( "crop", VIPS_INTERESTING_ATTENTION )->set( "size", VIPS_SIZE_DOWN ));
            break;
          case 3:
            image2 = vips_foreign_is_a("gifload",(char *)str.c_str()) ? 
                  VImage::thumbnail((char *)str.c_str(),ceil((double)maxSize*((double)imageTileWidth/(double)minSize)),VImage::option()->set( "size", VIPS_SIZE_DOWN )).cache(VImage::option()->set("tile_width",16)->set("tile_height",16)) : 
                  VImage::thumbnail((char *)str.c_str(),ceil((double)maxSize*((double)imageTileWidth/(double)minSize)),VImage::option()->set( "size", VIPS_SIZE_DOWN )).cache(VImage::option()->set("tile_width",ceil((double)width*((double)imageTileWidth/(double)minSize)))->set("tile_height",ceil((double)height*((double)imageTileWidth/(double)minSize))));
            break;
        }

        if( image2.bands() == 1 )
        {
          image2 = image2.bandjoin(image2).bandjoin(image2);
        }
        else if( image2.bands() == 4 )
        {
          image2 = image2.flatten();
        }
      }

      // If cropStyle is 3 then crop every possible square
      if( cropStyle == 3 )
      {
        // For every possible square
        for( int ii = sw - mosaicTileWidth; ii >= 0; --ii )
        {
          VImage newImage, newImage2;

          // Crop it correctly if it is a vertical or horizontal image
          if( vertical )
          {
            newImage = image.extract_area(0,ii,mosaicTileWidth,mosaicTileWidth);
            if(different) newImage2 = image2.extract_area(0,ii*imageTileWidth/mosaicTileWidth,imageTileWidth,imageTileWidth);
          }
          else
          {
            newImage = image.extract_area(ii,0,mosaicTileWidth,mosaicTileWidth);
            if(different) newImage2 = image2.extract_area(ii*imageTileWidth/mosaicTileWidth,0,imageTileWidth,imageTileWidth);
          }

          // Mirror the image if flip is set
          for( int ff = flip; ff >= 0; --ff)
          {
            if(flip)
            {
              newImage = newImage.flip(VIPS_DIRECTION_HORIZONTAL);
              if( different ) newImage2 = newImage2.flip(VIPS_DIRECTION_HORIZONTAL);
            }

            // Rotate the image 4 times if spin is set
            for( int jj = (spin ? 3:0); jj >= 0; --jj )
            {
              if(spin) newImage = newImage.rot90();

              // Get the image data
              c1 = (unsigned char *)newImage.data();

              // Push the data onto the vector
              mosaicTileData.push_back( vector< unsigned char >(c1, c1 + mosaicTileArea) );

              if( different )
              {
                if(spin) newImage2 = newImage2.rot90();
                c2 = (unsigned char *)newImage2.data();
                imageTileData.push_back( vector< unsigned char >(c2, c2 + imageTileArea) );
              }

              // Save the data detailing the image name, crop offset, mirror status, and rotation
              cropData.push_back( make_tuple(str,vertical?0:ii*minSize/mosaicTileWidth,vertical?ii*minSize/mosaicTileWidth:0,jj,ff) );
            }
          }
        }
      }
      // Otherwise just save the image
      else
      {
        // Mirror the image if flip is set
        for( int ff = flip; ff >= 0; --ff)
        {
          if(flip)
          {
            image = image.flip(VIPS_DIRECTION_HORIZONTAL);
            if( different ) image2 = image2.flip(VIPS_DIRECTION_HORIZONTAL);
          }

          // Rotate the image 4 times if spin is set
          for( int jj = (spin ? 3:0); jj >= 0; --jj )
          {
            if(spin) image = image.rot90();

            // Get the image data
            c1 = (unsigned char *)image.data();

            // Push the data onto the vector
            mosaicTileData.push_back( vector< unsigned char >(c1, c1 + mosaicTileArea) );

            if( different )
            {
              if(spin) image2 = image2.rot90();
              c2 = (unsigned char *)image2.data();
              imageTileData.push_back( vector< unsigned char >(c2, c2 + imageTileArea) );
            }

            // Save the data detailing the image name, crop offset, mirror status, and rotation
            cropData.push_back( make_tuple(str,xOffset*minSize/mosaicTileWidth,yOffset*minSize/mosaicTileWidth,jj,ff) );
          }
        }
      }
    }
    catch (...)
    {
    }
  }

  cout << endl;
}

void generateThumbnails( vector< vector< unsigned char > > &mosaicTileData, vector< vector< unsigned char > > &imageTileData, string imageDirectory, int mosaicTileWidth, int mosaicTileHeight, int imageTileWidth, int imageTileHeight, int cropStyle, bool flip )
{
  // Used for reading directory
  DIR *dir;
  struct dirent *ent;
  string str;

  // Used for processing image
  int size, xOffset, yOffset, width, height, mosaicTileArea = mosaicTileWidth*mosaicTileHeight*3, imageTileArea = imageTileWidth*imageTileHeight*3;
  bool different = mosaicTileWidth != imageTileWidth;

  unsigned char *c1, *c2;

  // Minimum size allowed: must be at least as large as mosaicTileWidth and imageTileWidth, and 256 if exclude is set
  int minWidth = max(mosaicTileWidth,imageTileWidth);
  int minHeight = max(mosaicTileHeight,imageTileHeight);

  int num_images = 0;

  vector< string > names;

  cout << "Reading directory " << imageDirectory << endl;

  // Count the number of valid image files in the directory
  if ((dir = opendir (imageDirectory.c_str())) != NULL) 
  {
    while ((ent = readdir (dir)) != NULL)
    {   
      if( ent->d_name[0] != '.' && vips_foreign_find_load( string( imageDirectory + ent->d_name ).c_str() ) != NULL )
      {
        names.push_back( imageDirectory + ent->d_name );
        cout << "\rFound " << ++num_images << " images " << flush;
      }
    }
  }

  cout << endl;

  ProgressBar *processing_images = new ProgressBar(num_images, "Processing images");

  // Iterate through all images in directory
  for( int i = 0; i < num_images; ++i )
  {
    processing_images->Increment();
    try
    {
      str = names[i];

      // Get the width, height, and smallest and largest sizes
      width = VImage::new_memory().vipsload( (char *)str.c_str() ).width();
      height = VImage::new_memory().vipsload( (char *)str.c_str() ).height();
      double widthRatio = (double)width/(double)mosaicTileWidth;
      double heightRatio = (double)height/(double)mosaicTileHeight;
      double minRatio = min(widthRatio,heightRatio);
      double maxRatio = max(widthRatio,heightRatio);
      // Whether the a square is offset vertically or not
      bool vertical = (double)width/(double)mosaicTileWidth < (double)height/(double)mosaicTileHeight;

      if( width < minWidth || height < minHeight )
      {
        continue;
      }

      VImage image2, image;

      // Shrink the image and crop based on cropping style
      switch( cropStyle )
      {
        // Crop the middle square
        case 0:
          image = VImage::thumbnail((char *)str.c_str(),mosaicTileWidth,VImage::option()->set( "height", mosaicTileHeight )->set( "crop", VIPS_INTERESTING_CENTRE )->set( "size", VIPS_SIZE_DOWN ));
          break;
        // Crop the square with the most entropy
        case 1:
          image = VImage::thumbnail((char *)str.c_str(),mosaicTileWidth,VImage::option()->set( "height", mosaicTileHeight )->set( "crop", VIPS_INTERESTING_ENTROPY )->set( "size", VIPS_SIZE_DOWN ));
          break;
        // Crop the square most likely to 
        case 2:
          image = VImage::thumbnail((char *)str.c_str(),mosaicTileWidth,VImage::option()->set( "height", mosaicTileHeight )->set( "crop", VIPS_INTERESTING_ATTENTION )->set( "size", VIPS_SIZE_DOWN ));
          break;
        // Shrink the image to be cropped later
        case 3:
          // In order to crop image multiple times, gifs need to be cached with 16x16 tiles, and averything else needs to be cached with the full image
          image = vips_foreign_is_a("gifload",(char *)str.c_str()) ? 
                  VImage::thumbnail((char *)str.c_str(),ceil(maxRatio/minRatio*(double)mosaicTileWidth),VImage::option()->set( "height", ceil(maxRatio/minRatio*(double)mosaicTileHeight) )->set( "size", VIPS_SIZE_DOWN )).cache(VImage::option()->set("tile_width",16)->set("tile_height",16)) : 
                  VImage::thumbnail((char *)str.c_str(),ceil(maxRatio/minRatio*(double)mosaicTileWidth),VImage::option()->set( "height", ceil(maxRatio/minRatio*(double)mosaicTileHeight) )->set( "size", VIPS_SIZE_DOWN )).cache(VImage::option()->set("tile_width",ceil(maxRatio/minRatio*(double)mosaicTileWidth))->set("tile_height",ceil(maxRatio/minRatio*(double)mosaicTileHeight)));
          break;
      }

      // Convert image to 3 band image
      if( image.bands() == 1 )
      {
        image = image.bandjoin(image).bandjoin(image);
      }
      else if( image.bands() == 4 )
      {
        image = image.flatten();
      }

      // If the mosaicTileWidth and imageTileWidth are different then shrink and crop again
      if( different )
      {
        switch( cropStyle )
        {
          // Crop the middle square
          case 0:
            image2 = VImage::thumbnail((char *)str.c_str(),imageTileWidth,VImage::option()->set( "height", imageTileHeight )->set( "crop", VIPS_INTERESTING_CENTRE )->set( "size", VIPS_SIZE_DOWN ));
            break;
          // Crop the square with the most entropy
          case 1:
            image2 = VImage::thumbnail((char *)str.c_str(),imageTileWidth,VImage::option()->set( "height", imageTileHeight )->set( "crop", VIPS_INTERESTING_ENTROPY )->set( "size", VIPS_SIZE_DOWN ));
            break;
          // Crop the square most likely to 
          case 2:
            image2 = VImage::thumbnail((char *)str.c_str(),imageTileWidth,VImage::option()->set( "height", imageTileHeight )->set( "crop", VIPS_INTERESTING_ATTENTION )->set( "size", VIPS_SIZE_DOWN ));
            break;
          // Shrink the image to be cropped later
          case 3:
            // In order to crop image multiple times, gifs need to be cached with 16x16 tiles, and averything else needs to be cached with the full image
            image2 = vips_foreign_is_a("gifload",(char *)str.c_str()) ? 
                    VImage::thumbnail((char *)str.c_str(),ceil(maxRatio/minRatio*(double)imageTileWidth),VImage::option()->set( "height", ceil(maxRatio/minRatio*(double)imageTileHeight) )->set( "size", VIPS_SIZE_DOWN )).cache(VImage::option()->set("tile_width",16)->set("tile_height",16)) : 
                    VImage::thumbnail((char *)str.c_str(),ceil(maxRatio/minRatio*(double)imageTileWidth),VImage::option()->set( "height", ceil(maxRatio/minRatio*(double)imageTileHeight) )->set( "size", VIPS_SIZE_DOWN )).cache(VImage::option()->set("tile_width",ceil(maxRatio/minRatio*(double)imageTileWidth))->set("tile_height",ceil(maxRatio/minRatio*(double)imageTileHeight)));
            break;
        }

        if( image2.bands() == 1 )
        {
          image2 = image2.bandjoin(image2).bandjoin(image2);
        }
        else if( image2.bands() == 4 )
        {
          image2 = image2.flatten();
        }
      }

      // If cropStyle is 3 then crop every possible square
      if( cropStyle == 3 )
      {
        /*
        // For every possible square
        for( int ii = sw - mosaicTileWidth; ii >= 0; --ii )
        {
          VImage newImage, newImage2;

          // Crop it correctly if it is a vertical or horizontal image
          if( vertical )
          {
            newImage = image.extract_area(0,ii,mosaicTileWidth,mosaicTileWidth);
            if(different) newImage2 = image2.extract_area(0,ii*imageTileWidth/mosaicTileWidth,imageTileWidth,imageTileWidth);
          }
          else
          {
            newImage = image.extract_area(ii,0,mosaicTileWidth,mosaicTileWidth);
            if(different) newImage2 = image2.extract_area(ii*imageTileWidth/mosaicTileWidth,0,imageTileWidth,imageTileWidth);
          }

          // Mirror the image if flip is set
          for( int ff = flip; ff >= 0; --ff)
          {
            if(flip)
            {
              newImage = newImage.flip(VIPS_DIRECTION_HORIZONTAL);
              if( different ) newImage2 = newImage2.flip(VIPS_DIRECTION_HORIZONTAL);
            }

            // Rotate the image 4 times if spin is set
            for( int jj = (spin ? 3:0); jj >= 0; --jj )
            {
              if(spin) newImage = newImage.rot90();

              // Get the image data
              c1 = (unsigned char *)newImage.data();

              // Push the data onto the vector
              mosaicTileData.push_back( vector< unsigned char >(c1, c1 + mosaicTileArea) );

              if( different )
              {
                if(spin) newImage2 = newImage2.rot90();
                c2 = (unsigned char *)newImage2.data();
                imageTileData.push_back( vector< unsigned char >(c2, c2 + imageTileArea) );
              }

              // Save the data detailing the image name, crop offset, mirror status, and rotation
              cropData.push_back( make_tuple(str,vertical?0:ii*minSize/mosaicTileWidth,vertical?ii*minSize/mosaicTileWidth:0,jj,ff) );
            }
          }
        }*/
      }
      // Otherwise just save the image
      else
      {
        // Mirror the image if flip is set
        for( int ff = flip; ff >= 0; --ff)
        {
          if(flip)
          {
            image = image.flip(VIPS_DIRECTION_HORIZONTAL);
            if( different ) image2 = image2.flip(VIPS_DIRECTION_HORIZONTAL);
          }

          // Get the image data
          c1 = (unsigned char *)image.data();

          // Push the data onto the vector
          mosaicTileData.push_back( vector< unsigned char >(c1, c1 + mosaicTileArea) );

          if( different )
          {
            c2 = (unsigned char *)image2.data();
            imageTileData.push_back( vector< unsigned char >(c2, c2 + imageTileArea) );
          }
        }
      }
    }
    catch (...)
    {
    }
  }

  cout << endl;
}

int generateLABBlock( vector< vector< float > > &imageData, vector< vector< int > > &mosaic, vector< int > &indices, vector< bool > &used, int repeat, unsigned char * c, int blockX, int blockY, int blockWidth, int blockHeight, int tileWidth, int tileHeight, int width, int numHorizontal, int numVertical, ProgressBar* buildingMosaic )
{
  // Whether to update progressbar
  bool show = !(blockX+blockY);

  // Vector lab tile data
  vector< float > d(tileWidth*tileHeight*3);

  int num_images = imageData.size();

  // For every index
  for( int p = 0; p < indices.size(); ++p )
  {
    // Get position of tile
    int i = indices[p] / blockWidth;
    int j = indices[p] % blockWidth;
    
    // Skip if outside of the array
    if( i >= blockHeight || j >= blockWidth ) continue;
    
    // Update progressbar if necessary
    if(show) buildingMosaic->Increment();

    // Add offset position
    i += blockY;
    j += blockX;

    // Start and end of tiles to check for repeating images
    int xEnd = ( (j+repeat+1<(int)mosaic[0].size()) ? j+repeat+1 : mosaic[0].size() );
    int yEnd = ( (i+repeat+1<(int)mosaic.size()) ? i+repeat+1 : mosaic.size() );

    // Extract rgb data of tile from image and convert to lab data
    for( int y = 0, n = 0; y < tileHeight; ++y )
    {
      for( int x = 0; x < tileWidth; ++x, n+=3 )
      {
        // Index of pixel
        int l = ( ( i * tileHeight + y ) * width + j * tileWidth + x ) * 3;
        
        float r,g,b;
        // Convert rgb to rgb16
        vips_col_sRGB2scRGB_16(c[l],c[l+1],c[l+2], &r,&g,&b );
        // Convert rgb16 to xyz
        vips_col_scRGB2XYZ( r, g, b, &r, &g, &b );
        // Convert xyz to lab
        vips_col_XYZ2Lab( r, g, b, &d[n], &d[n+1], &d[n+2] );
      }
    }

    int best = 0;
    double difference = DBL_MAX;

    // Iterate through every image and find the most similar one
    for( int k = 0; k < num_images; ++k )
    {
      // Check if image already exists within repeat blocks of tile
      if( repeat > 0 )
      {
        bool valid = true;

        // Break early if match found
        for( int y = (i-repeat > 0) ? i-repeat : 0; valid && y < yEnd; ++y )
        {
          for( int x = (j-repeat > 0) ? j-repeat : 0; valid && x < xEnd; ++x )
          {
            // Skip if is current tile
            if(  y == i && x == j ) continue;
            if( mosaic[y][x] == k )
            {
              valid = false;
            }
          }
        }

        // Skip current image if it was found
        if( !valid ) continue;
      }

      // Compute sum of color differences
      double sum = 0;
      for( int l = 0; l < tileWidth*tileHeight*3; l+=3 )
      {
        sum += vips_col_dE00( imageData[k][l], imageData[k][l+1], imageData[k][l+2], d[l], d[l+1], d[l+2] );
      }

      // Update for best color difference
      if( sum < difference )
      {
        difference = sum;
        best = k;
      }
    }

    // Set mosaic tile data
    mosaic[i][j] = best;

    // Set the best image as being used
    used[ best ] = true;
  }

  return 0;
}

// Takes a vector of image thumbnails, input image and number of images per row and creates output image
int generateMosaic( vector< vector< float > > &imageData, vector< vector< int > > &mosaic, string inputImage, ProgressBar *buildingMosaic, int repeat, bool square, int resize )
{
  // Whether to show progressbar
  bool show = ( buildingMosaic != NULL );

  // Load input image
  VImage image = VImage::new_memory().vipsload( (char *)inputImage.c_str() );//.colourspace( VIPS_INTERPRETATION_LAB );

  // Convert to a three band image
  if( image.bands() == 1 )
  {
    image = image.bandjoin(image).bandjoin(image);
  }
  if( image.bands() == 4 )
  {
    image = image.flatten();
  }

  // Crop out the middle square if specified
  if(square)
  {
    image = (image.width() < image.height()) ? image.extract_area(0, (image.height()-image.width())/2, image.width(), image.width()) :
                                               image.extract_area((image.width()-image.height())/2, 0, image.height(), image.height());
  }

  // Resize the image for correct mosaic tile size
  if( resize != 0 ) image = image.resize( (double)resize / (double)(image.width()) );

  int width = image.width();
  int height = image.height();

  // Get image data
  unsigned char * c = ( unsigned char * )image.data();

  // Get number of horizontal and vertical tiles
  int numHorizontal = mosaic[0].size();
  int numVertical = mosaic.size();

  // Data about tile size
  int tileWidth = width / numHorizontal;
  int tileHeight = height / numVertical;
  int tileArea = tileWidth*tileHeight*3;

  // Number of images to check against
  int num_images = imageData.size();

  // Whether an image was used or not
  vector< bool > used( num_images, false );

  // Number of threads for multithreading: needs to be a square number
  int threads = 9;
  int sqrtThreads = sqrt(threads);

  // Totat number of tiles in block
  int total = ceil((double)numVertical/(double)sqrtThreads)*ceil((double)numHorizontal/(double)sqrtThreads);

  // Create list of numbers in thread block
  vector< int > indices( total );
  iota( indices.begin(), indices.end(), 0 );

  // Shuffle the points so that patterens do not form
  if( repeat > 0 ) shuffle( indices.begin(), indices.end(), default_random_engine(time(NULL)) );

  // Break mosaic into blocks and find best matching tiles inside each block on a different thread
  future< int > ret[threads];

  for( int i = 0, k = 0; i < sqrtThreads; ++i )
  {
    for( int j = 0; j < sqrtThreads; ++j, ++k )
    {
      ret[k] = async( launch::async, &generateLABBlock, ref(imageData), ref(mosaic), ref(indices), ref(used), repeat, c, j*numHorizontal/sqrtThreads, i*numVertical/sqrtThreads, (j+1)*numHorizontal/sqrtThreads-j*numHorizontal/sqrtThreads, (i+1)*numVertical/sqrtThreads-i*numVertical/sqrtThreads, tileWidth, tileHeight, width, numHorizontal, numVertical, buildingMosaic );
    }
  }

  // Wait for threads to finish
  for( int k = 0; k < threads; ++k )
  {
    ret[k].get();
  }

  // Get total number of different images used
  return accumulate(used.begin(), used.end(), 0);
}

int generateRGBBlock( my_kd_tree_t &mat_index, vector< vector< int > > &mosaic, vector< int > &indices, vector< bool > &used, int repeat, unsigned char * c, int blockX, int blockY, int blockWidth, int blockHeight, int tileWidth, int tileHeight, int width, int numHorizontal, int numVertical, ProgressBar* buildingMosaic )
{
  // Whether to update progressbar
  bool show = !(blockX+blockY);

  // Color difference of images
  int out_dist_sqr;

  // Vector rgb tile data
  vector< int > d(tileWidth*tileHeight*3);

  // For every index
  for( int p = 0; p < indices.size(); ++p )
  {
    // Get position of tile
    int i = indices[p] / blockWidth;
    int j = indices[p] % blockWidth;
    
    // Skip if outside of the array
    if( i >= blockHeight || j >= blockWidth ) continue;
    
    // Update progressbar if necessary
    if(show) buildingMosaic->Increment();

    // Add offset position
    i += blockY;
    j += blockX;

    // Get rgb data about tile and save in vector
    for( int y = 0, n = 0; y < tileHeight; ++y )
    {
      for( int x = 0; x < tileWidth; ++x, n+=3 )
      {
        int l = ( ( i * tileHeight + y ) * width + j * tileWidth + x ) * 3;
        d[n] = c[l];
        d[n+1] = c[l+1];
        d[n+2] = c[l+2];
      }
    }

    size_t ret_index;

    if( repeat > 0 )
    {
      vector< vector< int > > newData = mat_index.m_data;
      set< int > eraseData;

      // Outside of square so that closer images are not the same
      for( int r = 1; r <= repeat; ++r )
      {
        for( int p1 = -r; eraseData.size() < newData.size()-1 && p1 <= r; ++p1 )
        {
          if( j+p1 >= 0 && j+p1 < numHorizontal && i-r >= 0 && mosaic[i-r][j+p1] >= 0 ) eraseData.insert(mosaic[i-r][j+p1]);
        }

        for( int p1 = -r; eraseData.size() < newData.size()-1 && p1 <= r; ++p1 )
        {
          if( j+p1 >= 0 && j+p1 < numHorizontal && i+r < numVertical && mosaic[i+r][j+p1] >= 0 ) eraseData.insert(mosaic[i+r][j+p1]);
        }

        for( int p1 = -r+1; eraseData.size() < newData.size()-1 && p1 < r; ++p1 )
        {
          if( i+p1 >= 0 && i+p1 < numVertical && j-r >= 0 && mosaic[i+p1][j-r] >= 0 ) eraseData.insert(mosaic[i+p1][j-r]);
        }

        for( int p1 = -r+1; eraseData.size() < newData.size()-1 && p1 < r; ++p1 )
        {
          if( i+p1 >= 0 && i+p1 < numVertical && j+r < numHorizontal && mosaic[i+p1][j+r] >= 0 ) eraseData.insert(mosaic[i+p1][j+r]);
        }
      }

      for (set<int>::reverse_iterator it = eraseData.rbegin(); it != eraseData.rend(); ++it)
      {
        newData.erase(newData.begin() + *it);
      }

      my_kd_tree_t newTree(newData[0].size(), newData, 10 );

      newTree.query( &d[0], 1, &ret_index, &out_dist_sqr );

      for (set<int>::iterator it = eraseData.begin(); it != eraseData.end(); ++it)
      {
        if( ret_index >= *it )
        {
          ++ret_index;
        }
        else
        {
          break;
        }
      }
    }
    else
    {
      mat_index.query( &d[0], 1, &ret_index, &out_dist_sqr );
    }

    // Set mosaic tile data
    mosaic[i][j] = ret_index;

    // Set the best image as being used
    used[ ret_index ] = true;
  }

  return 0;
}

int generateMosaic( my_kd_tree_t &mat_index, vector< vector< int > > &mosaic, string inputImage, ProgressBar *buildingMosaic, int repeat, bool square, int resize )
{
  // Whether to show progressbar
  bool show = ( buildingMosaic != NULL );

  // Number of images to check against
  int num_images = mat_index.kdtree_get_point_count();

  // Load input image
  VImage image = VImage::vipsload( (char *)inputImage.c_str() );

  // Convert to a three band image
  if( image.bands() == 1 )
  {
    image = image.bandjoin(image).bandjoin(image);
  }
  if( image.bands() == 4 )
  {
    image = image.flatten();
  }

  // Crop out the middle square if specified
  if(square)
  {
    image = (image.width() < image.height()) ? image.extract_area(0, (image.height()-image.width())/2, image.width(), image.width()) :
                                               image.extract_area((image.width()-image.height())/2, 0, image.height(), image.height());
  }

  // Resize the image for correct mosaic tile size
  if( resize != 0 ) image = image.resize( (double)resize / (double)(image.width()) );

  int width = image.width();
  int height = image.height();

  // Get image data
  unsigned char * c = ( unsigned char * )image.data();

  // Get number of horizontal and vertical tiles
  int numHorizontal = mosaic[0].size();
  int numVertical = mosaic.size();

  // Data about tile size
  int tileWidth = width / numHorizontal;
  int tileHeight = height / numVertical;
  int tileArea = tileWidth*tileHeight*3;

  // Whether an image was used or not
  vector< bool > used( num_images, false );

  // Number of threads for multithreading: needs to be a square number
  int threads = 9;
  int sqrtThreads = sqrt(threads);

  // Totat number of tiles in block
  int total = ceil((double)numVertical/(double)sqrtThreads)*ceil((double)numHorizontal/(double)sqrtThreads);

  // Create list of numbers in thread block
  vector< int > indices( total );
  iota( indices.begin(), indices.end(), 0 );

  // Shuffle the points so that patterens do not form
  if( repeat > 0 ) shuffle( indices.begin(), indices.end(), default_random_engine(time(NULL)) );

  // Break mosaic into blocks and find best matching tiles inside each block on a different thread
  future< int > ret[threads];

  for( int i = 0, k = 0; i < sqrtThreads; ++i )
  {
    for( int j = 0; j < sqrtThreads; ++j, ++k )
    {
      ret[k] = async( launch::async, &generateRGBBlock, ref(mat_index), ref(mosaic), ref(indices), ref(used), repeat, c, j*numHorizontal/sqrtThreads, i*numVertical/sqrtThreads, (j+1)*numHorizontal/sqrtThreads-j*numHorizontal/sqrtThreads, (i+1)*numVertical/sqrtThreads-i*numVertical/sqrtThreads, tileWidth, tileHeight, width, numHorizontal, numVertical, buildingMosaic );
    }
  }

  // Wait for threads to finish
  for( int k = 0; k < threads; ++k )
  {
    ret[k].get();
  }

  // Get total number of different images used
  return accumulate(used.begin(), used.end(), 0);
}

void buildImage( vector< vector< unsigned char > > &imageData, vector< vector< int > > &mosaic, string outputImage, int tileWidth, int tileHeight )
{
  cout << "Generating image " << outputImage << endl;

  // Dimensions of mosaic
  int width = mosaic[0].size();
  int height = mosaic.size();

  // Save data to output image
  if( vips_foreign_find_save( outputImage.c_str() ) != NULL )
  {
    // Create output image data 
    unsigned char *data = new unsigned char[width*height*tileWidth*tileHeight*3];

    // Itereate through everty tile
    for( int i = 0; i < height; ++i )
    {
      for( int j = 0; j < width; ++j )
      {
        // Iterate through every pixel in tile
        for( int y = 0; y < tileHeight; ++y )
        {
          for( int x = 0; x < tileWidth; ++x )
          {
            // Save the image data to the output tile
            data[ 3 * ( width * tileWidth * ( i * tileHeight + y ) + j * tileWidth + x ) ] = imageData[mosaic[i][j]][3*(y*tileWidth+x)];
            data[ 3 * ( width * tileWidth * ( i * tileHeight + y ) + j * tileWidth + x ) + 1 ] = imageData[mosaic[i][j]][3*(y*tileWidth+x)+1];
            data[ 3 * ( width * tileWidth * ( i * tileHeight + y ) + j * tileWidth + x ) + 2 ] = imageData[mosaic[i][j]][3*(y*tileWidth+x)+2];
          }
        }
      }
    }

    VImage::new_from_memory( data, width*height*tileWidth*tileHeight*3, width*tileWidth, height*tileHeight, 3, VIPS_FORMAT_UCHAR ).vipssave((char *)outputImage.c_str());

    // Free the data
    delete [] data;
  }
  else
  {
    int level = (int)ceil(log2( max(width*tileWidth,height*tileHeight) ) );

    if( outputImage.back() == '/' ) outputImage = outputImage.substr(0, outputImage.size()-1);

    g_mkdir(string(outputImage).append("_files/").c_str(), 0777);
    g_mkdir(string(outputImage).append("_files/").append(to_string(level)).c_str(), 0777);

    for( int i = 0; i < height*tileHeight; i+=256 )
    {
      for( int j = 0; j < width*tileWidth; j+=256 )
      {
        int thisTileWidth = min( width*tileWidth - j, 256);
        int thisTileHeight = min( height*tileHeight - i, 256);

        unsigned char *data = new unsigned char[thisTileWidth*thisTileHeight*3];

        for( int k = 0, p = 0; k < thisTileHeight; ++k )
        {
          for( int l = 0; l < thisTileWidth; ++l, p+=3 )
          {
            int x = (j+l)%tileWidth;
            int y = (i+k)%tileHeight;

            int tileX = (j+l)/tileWidth;
            int tileY = (i+k)/tileHeight;

            data[p+0] = imageData[mosaic[tileY][tileX]][3*(y*tileWidth+x)+0];
            data[p+1] = imageData[mosaic[tileY][tileX]][3*(y*tileWidth+x)+1];
            data[p+2] = imageData[mosaic[tileY][tileX]][3*(y*tileWidth+x)+2];
          }
        }

        VImage::new_from_memory( data, thisTileWidth*thisTileHeight*3, thisTileWidth, thisTileHeight, 3, VIPS_FORMAT_UCHAR ).jpegsave((char *)string(outputImage).append("_files/"+to_string(level)+"/"+to_string(j/256)+"_"+to_string(i/256)+".jpeg").c_str(),VImage::option()->set( "optimize_coding", true )->set( "strip", true ));

        delete [] data;
      }
    }

    int sizeWidth = ( floor( sqrt( width*tileWidth ) ) < sqrt( width*tileWidth ) ) ? ( (int)sqrt(width*tileWidth) )<<1 : (int)sqrt(width*tileWidth);
    int sizeHeight = ( floor( sqrt( height*tileHeight ) ) < sqrt( height*tileHeight ) ) ? ( (int)sqrt(height*tileHeight) )<<1 : (int)sqrt(height*tileHeight);

    ofstream dzi_file;
    dzi_file.open(string(outputImage).append(".dzi").c_str());
    dzi_file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
    dzi_file << "<Image xmlns=\"http://schemas.microsoft.com/deepzoom/2008\" Format=\"jpeg\" Overlap=\"0\" TileSize=\"256\">" << endl;
    dzi_file << "    <Size Height=\"" << height*tileHeight << "\" Width=\"" << width*tileWidth << "\"/>" << endl;
    dzi_file << "</Image>" << endl;
    dzi_file.close();

    width = (int)ceil((double)(width*tileWidth)/ 128.0 );
    height = (int)ceil((double)(height*tileHeight)/ 128.0 );

    for( ; level > 0; --level )
    {
      width = (int)ceil((double)width/ 2.0 );
      height = (int)ceil((double)height/ 2.0 );

      string current = string(outputImage).append("_files/"+to_string(level-1)+"/");
      string upper = string(outputImage).append("_files/"+to_string(level)+"/");

      g_mkdir((char *)current.c_str(), 0777);

      for(int i = 1; i < width; i+=2)
      {
        for(int j = 1; j < height; j+=2)
        {
          (VImage::jpegload((char *)string(upper).append(to_string(i-1)+"_"+to_string(j-1)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
          join(VImage::jpegload((char *)string(upper).append(to_string(i)+"_"+to_string(j-1)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL)).
          join(VImage::jpegload((char *)string(upper).append(to_string(i-1)+"_"+to_string(j)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
          join(VImage::jpegload((char *)string(upper).append(to_string(i)+"_"+to_string(j)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL),VIPS_DIRECTION_VERTICAL).
          jpegsave((char *)string(current).append(to_string(i>>1)+"_"+to_string(j>>1)+".jpeg").c_str() );
        }
      }
      if(width%2 == 1)
      {
        for(int j = 1; j < height; j+=2)
        {
          (VImage::jpegload((char *)string(upper).append(to_string(width-1)+"_"+to_string(j-1)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
          join(VImage::jpegload((char *)string(upper).append(to_string(width-1)+"_"+to_string(j)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_VERTICAL)).
          jpegsave((char *)string(current).append(to_string(width>>1)+"_"+to_string(j>>1)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );
        }
      }
      if(height%2 == 1)
      {
        for(int j = 1; j < width; j+=2)
        {
          (VImage::jpegload((char *)string(upper).append(to_string(j-1)+"_"+to_string(height-1)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
          join(VImage::jpegload((char *)string(upper).append(to_string(j)+"_"+to_string(height-1)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL)).
          jpegsave((char *)string(current).append(to_string(j>>1)+"_"+to_string(height>>1)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );
        }
      }
      if(width%2 == 1 && height%2 == 1)
      {
          VImage::jpegload((char *)string(upper).append(to_string(width-1)+"_"+to_string(height-1)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
          jpegsave((char *)string(current).append(to_string(width>>1)+"_"+to_string(height>>1)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );  
      }
    }
  
    // Generate html file to view deep zoom image
    ofstream htmlFile(string(outputImage).append(".html").c_str());
    htmlFile << "<!DOCTYPE html>\n<html>\n<head><script src=\"js/openseadragon.min.js\"></script></head>\n<body>\n<style>\nhtml,\nbody,\n#rotationalMosaic\n{\nposition: fixed;\nleft: 0;\ntop: 0;\nwidth: 100%;\nheight: 100%;\n}\n</style>\n\n<div id=\"rotationalMosaic\"></div>\n\n<script>\nvar viewer = OpenSeadragon({\nid: 'rotationalMosaic',\nprefixUrl: 'icons/',\ntileSources:   \"" + outputImage + ".dzi\",\nminZoomImageRatio: 0,\nmaxZoomImageRatio: 1\n});\n</script>\n</body>\n</html>";
    htmlFile.close();
  }
}

void buildDeepZoomImage( vector< vector< int > > &mosaic, vector< cropType > cropData, int numUnique, string outputImage, ofstream& htmlFile )
{
  // Rotation angles
  VipsAngle rotAngle[4] = {VIPS_ANGLE_D0,VIPS_ANGLE_D270,VIPS_ANGLE_D180,VIPS_ANGLE_D90};

  ostringstream levelData;

  // Mosaic dimension data
  int width = mosaic[0].size();
  int height = mosaic.size();

  int numHorizontal = width;
  int numVertical = height;

  // Number of levels required to generate
  int maxZoomablesLevel = INT_MIN;
  int minZoomablesLevel = INT_MAX;

  // How many contructed levels there will be
  int level = (int)ceil( log2((width > height) ? width : height ) ) + 6;

  // Used for first pass
  bool first = true;

  // Output zoomable directory name
  string outputDirectory = string( outputImage + "zoom/" );

  // Create output zoomable directory
  g_mkdir(outputDirectory.c_str(), 0777);

  // Vector for output information for html file
  vector< string > mosaicStrings;

  ProgressBar *generatingZoomables = new ProgressBar(numUnique, "Generating zoomables");

  levelData << "var levelData = [";

  // Generate deep zoom images for each used image
  {
    ostringstream currentMosaic;
    vector< int > alreadyDone;
    int numTiles = 0;

    currentMosaic << "[";
    for( int i = 0; i < height; ++i )
    {
      currentMosaic << "[";
      for( int j = 0; j < width; ++j )
      {
        int current = mosaic[i][j];

        // Skip this image if it has already been processed
        bool skip = false;
        int k = 0;
        for( ; k < numTiles; ++k )
        {
          if( alreadyDone[k] == current )
          {
            skip = true;
            break;
          }
        }

        // If the image has not been processed then convert it to a deep zoom image
        if( !skip )
        {
          // Update that this image has been processed
          alreadyDone.push_back( current );

          // Update the mosaic to have the ID for this image
          mosaic[i][j] = numTiles++;

          // Load image
          VImage image = VImage::vipsload( (char *)(get<0>(cropData[current])).c_str() );

          // Find the width of the largest square inside image
          int width = image.width();
          int height = image.height();
          int size = min( width, height );
          
          // Logarithm of square size
          int log = round( log2( size ) );

          // Number of levels to generate
          if( numTiles > 1 ) levelData << ","; 
          levelData << (log-8);

          maxZoomablesLevel = max( maxZoomablesLevel, (log-8) );
          minZoomablesLevel = min( minZoomablesLevel, (log-8) );

          int newSize = 1 << log;

          // Build the zoomable image if necessary 
          string zoomableName = outputDirectory + to_string(mosaic[i][j]);

          // Extract square, flip, and rotate based on cropdata, and then save as a deep zoom image
          if( get<4>(cropData[current]) )
          {
            image.extract_area(get<1>(cropData[current]), get<2>(cropData[current]), size, size).resize((double)newSize/(double)size).flip(VIPS_DIRECTION_HORIZONTAL).rot(rotAngle[get<3>(cropData[current])]).dzsave((char *)zoomableName.c_str(), VImage::option()->set( "depth", VIPS_FOREIGN_DZ_DEPTH_ONETILE )->set( "tile-size", 256 )->set( "overlap", false )->set( "strip", true ) );
          }
          // Do not flip if flip value is not set
          else
          {
            image.extract_area(get<1>(cropData[current]), get<2>(cropData[current]), size, size).resize((double)newSize/(double)size).rot(rotAngle[get<3>(cropData[current])]).dzsave((char *)zoomableName.c_str(), VImage::option()->set( "depth", VIPS_FOREIGN_DZ_DEPTH_ONETILE )->set( "tile-size", 256 )->set( "overlap", false )->set( "strip", true ) );
          }
          generatingZoomables->Increment();
        }
        // Otherwise set the mosaic value to the ID for the image
        else
        {
          mosaic[i][j] = k;
        }
        // Save the mosaic ID for the html file
        currentMosaic << mosaic[i][j];
        // Put a comma if there are more
        if( j < width - 1 ) currentMosaic << ","; 
      }
      currentMosaic << "]";
      if( i < height - 1 ) currentMosaic << ",";
    }
    currentMosaic << "]";
    mosaicStrings.push_back(currentMosaic.str());
  }

  cout << endl;
  ProgressBar *generatingLevels = new ProgressBar(height, "Generating levels");

  levelData << "];";

  // Grab four tiles, join them together, shrink by two in each dimension until the output is only a pixel big
  for( ; level >= 0; --level )
  {
    // Vector for whether these four images have been processed yet
    vector< vector< int > > alreadyDone;
    int numTiles = 0;

    ostringstream image, directory, input, currentMosaic;

    // Make the directory for the zoomable image
    image << outputImage << level << "/";
    input << outputImage << level + 1 << "/";
    directory << outputDirectory;
    g_mkdir(image.str().c_str(), 0777);

    // Go through every 2 by 2 images
    for( int i = 1; i < height; i+=2 )
    {
      for( int j = 1; j < width; j+=2 )
      {
        // Vector of current images
        vector< int > current = { mosaic[i-1][j-1], mosaic[i-1][j], mosaic[i][j-1], mosaic[i][j] };

        // Check whether 4 images have been processed yet
        bool skip = false;
        int k = 0;
        for( ; k < numTiles; ++k )
        {
          if( alreadyDone[k] == current )
          {
            skip = true;
            break;
          }
        }

        // Join four images, shrink my two, and save
        if( !skip )
        {
          alreadyDone.push_back( current );

          // If first pass load from zoomable image directory
          if( first )
          {
            (VImage::jpegload((char *)directory.str().append(to_string(mosaic[i-1][j-1])+"_files/0/0_0.jpeg").c_str(), VImage::option()->set( "shrink", 2)).
            join(VImage::jpegload((char *)directory.str().append(to_string(mosaic[i-1][j])+"_files/0/0_0.jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL)).
            join(VImage::jpegload((char *)directory.str().append(to_string(mosaic[i][j-1])+"_files/0/0_0.jpeg").c_str(), VImage::option()->set( "shrink", 2)).
            join(VImage::jpegload((char *)directory.str().append(to_string(mosaic[i][j])+"_files/0/0_0.jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL),VIPS_DIRECTION_VERTICAL).
            jpegsave((char *)image.str().append(to_string(numTiles)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );
          }
          // Otherwise load from tile level directory
          else
          {
            (VImage::jpegload((char *)input.str().append(to_string(mosaic[i-1][j-1])+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
            join(VImage::jpegload((char *)input.str().append(to_string(mosaic[i-1][j])+".jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL)).
            join(VImage::jpegload((char *)input.str().append(to_string(mosaic[i][j-1])+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
            join(VImage::jpegload((char *)input.str().append(to_string(mosaic[i][j])+".jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL),VIPS_DIRECTION_VERTICAL).
            jpegsave((char *)image.str().append(to_string(numTiles)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );
          }

          // The mosaic shrinks as well
          mosaic[i/2][j/2] = numTiles++;
        }
        else
        {
          // Save the ID of the image if it has been shrunk already
          mosaic[i/2][j/2] = k;
        }
      }
      generatingLevels->Increment();
    }

    // Edge cases with only two images
    if(width%2 == 1)
    {
      // Clear list of images already processed incase edge cases create incorrect duplicates
      int prevTileSize = numTiles;
      alreadyDone.clear();

      for(int j = 1; j < height; j+=2)
      {
        vector< int > current = { mosaic[j-1][width-1], mosaic[j][width-1] };

        bool skip = false;
        int k = prevTileSize;
        for( ; k < numTiles; ++k )
        {
          if( alreadyDone[k-prevTileSize] == current )
          {
            skip = true;
            break;
          }
        }

        if( !skip )
        {
          alreadyDone.push_back( current );

          if( first )
          {
            (VImage::jpegload((char *)directory.str().append(to_string(mosaic[j-1][width-1])+"_files/0/0_0.jpeg").c_str(), VImage::option()->set( "shrink", 2)).
            join(VImage::jpegload((char *)directory.str().append(to_string(mosaic[j][width-1])+"_files/0/0_0.jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_VERTICAL)).
            jpegsave((char *)image.str().append(to_string(numTiles)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );
          }
          else
          {
            (VImage::jpegload((char *)input.str().append(to_string(mosaic[j-1][width-1])+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
            join(VImage::jpegload((char *)input.str().append(to_string(mosaic[j][width-1])+".jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_VERTICAL)).
            jpegsave((char *)image.str().append(to_string(numTiles)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );
          }

          mosaic[j/2][width/2] = numTiles++;
        }
        else
        {
          mosaic[j/2][width/2] = k;
        }
      }
    }

    if(height%2 == 1)
    {
      int prevTileSize = numTiles;
      alreadyDone.clear();
      for(int j = 1; j < width; j+=2)
      {
        vector< int > current = { mosaic[height-1][j-1], mosaic[height-1][j] };

        bool skip = false;
        int k = prevTileSize;
        for( ; k < numTiles; ++k )
        {
          if( alreadyDone[k-prevTileSize] == current )
          {
            skip = true;
            break;
          }
        }

        if( !skip )
        {
          alreadyDone.push_back( current );

          if( first )
          {
            (VImage::jpegload((char *)directory.str().append(to_string(mosaic[height-1][j-1])+"_files/0/0_0.jpeg").c_str(), VImage::option()->set( "shrink", 2)).
            join(VImage::jpegload((char *)directory.str().append(to_string(mosaic[height-1][j])+"_files/0/0_0.jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL)).
            jpegsave((char *)image.str().append(to_string(numTiles)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );
          }
          else
          {
            (VImage::jpegload((char *)input.str().append(to_string(mosaic[height-1][j-1])+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
            join(VImage::jpegload((char *)input.str().append(to_string(mosaic[height-1][j])+".jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL)).
            jpegsave((char *)image.str().append(to_string(numTiles)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );
          }

          mosaic[height/2][j/2] = numTiles++;
        }
        else
        {
          mosaic[height/2][j/2] = k;
        }
      }
    }

    // Edge case for bottom corner
    if(width%2 == 1 && height%2 == 1)
    {
      mosaic[height/2][width/2] = numTiles;
      if( first )
      {
        VImage::jpegload((char *)directory.str().append(to_string(mosaic[height-1][width-1])+"_files/0/0_0.jpeg").c_str(), VImage::option()->set( "shrink", 2)).
        jpegsave((char *)image.str().append(to_string(numTiles)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );  
      }
      else
      {
        VImage::jpegload((char *)input.str().append(to_string(mosaic[height-1][width-1])+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
        jpegsave((char *)image.str().append(to_string(numTiles)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );  
      }
    }

    width = (int)ceil((double)width/2.0);
    height = (int)ceil((double)height/2.0);

    currentMosaic << "[";

    for( int i = 0; i < height; ++i )
    {
      currentMosaic << "[";
      for( int j = 0; j < width; ++j )
      {
        currentMosaic << mosaic[i][j];
        if( j < width-1 ) currentMosaic << ",";
      }
      currentMosaic << "]";
      if( i < height-1 ) currentMosaic << ",";
    }
    currentMosaic << "]";

    mosaicStrings.push_back(currentMosaic.str());

    first = false;
  }

  cout << endl;

  htmlFile << levelData.str() << endl;

  htmlFile << "var data = [ \n";

  for( int i = mosaicStrings.size()-1; i>=0; --i )
  {
    htmlFile << mosaicStrings[i] << "," << endl;
  }

  htmlFile << "];";

  htmlFile << "var mosaicWidth = " << numHorizontal << ";\nvar mosaicHeight = " << numVertical << ";\nvar mosaicLevel = " << mosaicStrings.size()-1 << ";\nvar minZoomablesLevel = " << minZoomablesLevel << ";\nvar maxZoomablesLevel = " << maxZoomablesLevel << ";\n";
}

void buildContinuousMosaic( vector< vector< vector< int > > > mosaic, vector< VImage > &images, string outputDirectory, ofstream& htmlFile )
{
  vector< string > mosaicStrings;
  vector< vector< vector< int > > > alreadyDone( 6 );

  g_mkdir( outputDirectory.c_str(), 0777);
  g_mkdir( string( outputDirectory ).append( "0/" ).c_str(), 0777);
  g_mkdir( string( outputDirectory ).append( "1/" ).c_str(), 0777);
  g_mkdir( string( outputDirectory ).append( "2/" ).c_str(), 0777);
  g_mkdir( string( outputDirectory ).append( "3/" ).c_str(), 0777);
  g_mkdir( string( outputDirectory ).append( "4/" ).c_str(), 0777);
  g_mkdir( string( outputDirectory ).append( "5/" ).c_str(), 0777);

  int numImages = mosaic.size();

  int size = 64;

  {
    ostringstream currentMosaic;
    currentMosaic << "[";

    ProgressBar *generatingLevels = new ProgressBar(numImages, "Generating level 5");

    for( int n = 0; n < numImages; ++n )
    {
      currentMosaic << "[";
      for( int i = 0; i < size; ++i )
      {
        currentMosaic << "[";
        for( int j = 0; j < size; ++j )
        {
          currentMosaic << mosaic[n][i][j];
          if( j < size-1 ) currentMosaic << ",";
        }
        currentMosaic << "]";
        if( i < size-1 ) currentMosaic << ",";
      }
      currentMosaic << "]";
      if( n < numImages-1 ) currentMosaic << ",";

      for( int i = 1; i < size; i+=2 )
      {
        for( int j = 1; j < size; j+=2 )
        {
          vector< int > current = { mosaic[n][i-1][j-1], mosaic[n][i-1][j], mosaic[n][i][j-1], mosaic[n][i][j] };

          bool skip = false;
          int k = 0;
          for( ; k < (int)alreadyDone[5].size(); ++k )
          {
            if( alreadyDone[5][k] == current )
            {
              skip = true;
              break;
            }
          }

          if( !skip )
          {
            alreadyDone[5].push_back( current );

            mosaic[n][i>>1][j>>1] = alreadyDone[5].size()-1;

            (images[current[0]].
            join(images[current[1]],VIPS_DIRECTION_HORIZONTAL)).
            join(images[current[2]].
            join(images[current[3]],VIPS_DIRECTION_HORIZONTAL),VIPS_DIRECTION_VERTICAL).
            jpegsave((char *)string( outputDirectory ).append( "5/" + to_string(mosaic[n][i>>1][j>>1]) + ".jpeg" ).c_str() /*, VImage::option()->set( "optimize_coding", true )->set( "strip", true ) */ ); 
          }
          else
          {
            mosaic[n][i>>1][j>>1] = k;
          }
        }
      }
      generatingLevels->Increment();
    }

    cout << endl;

    currentMosaic << "]\n";

    mosaicStrings.push_back( currentMosaic.str() );
  }

  for( int l = 4; l >= 0 ; --l )
  {
    ostringstream currentMosaic;
    currentMosaic << "[";

    string progressString = "Generating level "+ to_string(l);
    ProgressBar *generatingLevels = new ProgressBar(numImages, progressString.c_str());

    size = size >> 1;
    for( int n = 0; n < numImages; ++n )
    {
      currentMosaic << "[";
      for( int i = 0; i < size; ++i )
      {
        currentMosaic << "[";
        for( int j = 0; j < size; ++j )
        {
          currentMosaic << mosaic[n][i][j];
          if( j < size-1 ) currentMosaic << ",";
        }
        currentMosaic << "]";
        if( i < size-1 ) currentMosaic << ",";
      }
      currentMosaic << "]";
      if( n < numImages-1 ) currentMosaic << ",";

      for( int i = 1; i < size; i+=2 )
      {
        for( int j = 1; j < size; j+=2 )
        {
          vector< int > current = { mosaic[n][i-1][j-1], mosaic[n][i-1][j], mosaic[n][i][j-1], mosaic[n][i][j] };

          bool skip = false;
          int k = 0;
          for( ; k < (int)alreadyDone[l].size(); ++k )
          {
            if( alreadyDone[l][k] == current )
            {
              skip = true;
              break;
            }
          }

          if( !skip )
          {
            alreadyDone[l].push_back( current );
            mosaic[n][i>>1][j>>1] = alreadyDone[l].size()-1;

            (VImage::jpegload((char *)string( outputDirectory ).append(to_string(l+1) + "/" + to_string(current[0])+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
            join(VImage::jpegload((char *)string( outputDirectory ).append(to_string(l+1) + "/" + to_string(current[1])+".jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL)).
            join(VImage::jpegload((char *)string( outputDirectory ).append(to_string(l+1) + "/" + to_string(current[2])+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
            join(VImage::jpegload((char *)string( outputDirectory ).append(to_string(l+1) + "/" + to_string(current[3])+".jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL),VIPS_DIRECTION_VERTICAL).
            jpegsave((char *)string( outputDirectory ).append(to_string(l) + "/" + to_string(alreadyDone[l].size()-1)+".jpeg").c_str() /*, VImage::option()->set( "optimize_coding", true )->set( "strip", true )*/ );
          }
          else
          {
            mosaic[n][i>>1][j>>1] = k;
          }
        }
      }
      generatingLevels->Increment();
    }

    cout << endl;

    currentMosaic << "]\n";
    mosaicStrings.push_back( currentMosaic.str() );
  }

  {
    ostringstream currentMosaic;
    currentMosaic << "[";

    size = size >> 1;
    for( int n = 0; n < numImages; ++n )
    {
      currentMosaic << "[";
      for( int i = 0; i < size; ++i )
      {
        currentMosaic << "[";
        for( int j = 0; j < size; ++j )
        {
          currentMosaic << mosaic[n][i][j];
          if( j < size-1 ) currentMosaic << ",";
        }
        currentMosaic << "]";
        if( i < size-1 ) currentMosaic << ",";
      }
      currentMosaic << "]";
      if( n < numImages-1 ) currentMosaic << ",";
    }

    currentMosaic << "]\n";
    mosaicStrings.push_back( currentMosaic.str() );
  }

  htmlFile << "var data = [ \n";

  for( int i = mosaicStrings.size()-1; i>=0; --i )
  {
    htmlFile << mosaicStrings[i] << "," << endl;
  }

  htmlFile << "];";
}