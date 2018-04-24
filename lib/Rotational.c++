#include "Rotational.h"

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

  processing_images->Finish();
}

int rotateX( double x, double y, double cosAngle, double sinAngle, double halfWidth, double halfHeight )
{
  return cosAngle*(x-halfWidth) - sinAngle*(y-halfHeight) + halfWidth;
}

int rotateY( double x, double y, double cosAngle, double sinAngle, double halfWidth, double halfHeight )
{
  return sinAngle*(x-halfWidth) + cosAngle*(y-halfHeight) + halfHeight;
}

rotateResult findSmallest( int x, int y, int start, int end, int imageWidth, int imageHeight, vector< vector< unsigned char > > &images, vector< vector< unsigned char > > &masks, vector< pair< int, int > > &dimensions, unsigned char * imageData, unsigned char * computeMaskData, int angleOffset, bool trueColor )
{
  int bestImage = -1;
  double bestDifference = DBL_MAX, bestAngle = 0;

  // Run through every image in thread area
  // Round down to nearest even number
  for( int k = start-start%2; k < end-end%2; k+=2 )
  {
    // Rotate image for every angleOffset angles
    for( double a = 0; a < 360; a += angleOffset )
    {
      // Convert to radians
      double angle = a * 3.14159265/180.0;
      int width = dimensions[k].first;
      int height = dimensions[k].second;

      // Compute data for point rotation
      double cosAngle = cos(angle);
      double sinAngle = sin(angle);
      double halfWidth = (double)width/2.0;
      double halfHeight = (double)height/2.0;

      // Conpute side offset of rotated image from regular image
      int xOffset = max( rotateX(0,0,cosAngle,sinAngle,halfWidth,halfHeight), max( rotateX(width,0,cosAngle,sinAngle,halfWidth,halfHeight), max( rotateX(0,height,cosAngle,sinAngle,halfWidth,halfHeight), rotateX(width,height,cosAngle,sinAngle,halfWidth,halfHeight) ) ) ) - width;
      int yOffset = max( rotateY(0,0,cosAngle,sinAngle,halfWidth,halfHeight), max( rotateY(width,0,cosAngle,sinAngle,halfWidth,halfHeight), max( rotateY(0,height,cosAngle,sinAngle,halfWidth,halfHeight), rotateY(width,height,cosAngle,sinAngle,halfWidth,halfHeight) ) ) ) - height;

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
          int newX = rotateX(j,i,cosAngle,sinAngle,halfWidth,halfHeight) - xOffset;
          int newY = rotateY(j,i,cosAngle,sinAngle,halfWidth,halfHeight) - yOffset;

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

          if( trueColor )
          {
            float l1,a1,b1,l2,a2,b2;
            // Convert rgb to rgb16
            vips_col_sRGB2scRGB_16(ir,ig,ib, &l1,&a1,&b1 );
            vips_col_sRGB2scRGB_16(tr,tg,tb, &l2,&a2,&b2 );
            // Convert rgb16 to xyz
            vips_col_scRGB2XYZ( l1, a1, b1, &l1, &a1, &b1 );
            vips_col_scRGB2XYZ( l2, a2, b2, &l2, &a2, &b2 );
            // Convert xyz to lab
            vips_col_XYZ2Lab( l1, a1, b1, &l1, &a1, &b1 );
            vips_col_XYZ2Lab( l2, a2, b2, &l2, &a2, &b2 );

            difference += vips_col_dE00( l1, a1, b1, l2, a2, b2 );
          }
          else
          {
            // Compute the sum-of-squares for color difference
            difference += (ir-tr)*(ir-tr) + (ig-tg)*(ig-tg) + (ib-tb)*(ib-tb);
          
          }
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

  return make_tuple(bestImage, bestAngle, bestDifference );
}

void buildRotationalImage( string inputImage, string outputImage, vector< vector< unsigned char > > &images, vector< vector< unsigned char > > &masks, vector< pair< int, int > > &dimensions, double resize, int numIter, int angleOffset, int numSkip, trueColor )
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

    // Only choose point if it is not already taken
    if( numSkip > -1 && computeMaskData[y*imageWidth+x]==255)
    {
      if( --numSkip < 0 ) break;
      --inc;
      continue;
    }
  
    processing_images->Increment();

    int bestImage = -1;
    double bestDifference = DBL_MAX, bestAngle = 0;

    int threads = 4;

    future< rotateResult > ret[threads];

    for( int k = 0; k < threads; ++k )
    {
      ret[k] = async( launch::async, &findSmallest, x, y, k*images.size()/threads, (k+1)*images.size()/threads, imageWidth, imageHeight, ref(images), ref(masks), ref(dimensions), imageData, computeMaskData, angleOffset, trueColor );
    }

    // Wait for threads to finish
    for( int k = 0; k < threads; ++k )
    {
      rotateResult r = ret[k].get();

      if( get<2>(r) < bestDifference )
      {
        bestImage = get<0>(r);
        bestAngle = get<1>(r);
        bestDifference = get<2>(r);
      }
    }

    // If no image was chosen then skip
    if( bestImage == -1 ) continue;

    // Set the angle and offsets like before
    int width = dimensions[bestImage].first;
    int height = dimensions[bestImage].second;

    double cosAngle = cos(bestAngle);
    double sinAngle = sin(bestAngle);
    double halfWidth = (double)width/2.0;
    double halfHeight = (double)height/2.0;

    int xOffset = max( rotateX(0,0,cosAngle,sinAngle,halfWidth,halfHeight), max( rotateX(width,0,cosAngle,sinAngle,halfWidth,halfHeight), max( rotateX(0,height,cosAngle,sinAngle,halfWidth,halfHeight), rotateX(width,height,cosAngle,sinAngle,halfWidth,halfHeight) ) ) ) - width;
    int yOffset = max( rotateY(0,0,cosAngle,sinAngle,halfWidth,halfHeight), max( rotateY(width,0,cosAngle,sinAngle,halfWidth,halfHeight), max( rotateY(0,height,cosAngle,sinAngle,halfWidth,halfHeight), rotateY(width,height,cosAngle,sinAngle,halfWidth,halfHeight) ) ) ) - height;

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
        int newX = rotateX(j,i,cosAngle,sinAngle,halfWidth,halfHeight) - xOffset;
        int newY = rotateY(j,i,cosAngle,sinAngle,halfWidth,halfHeight) - yOffset;

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

    xOffset = max( rotateX(0,0,cosAngle,sinAngle,halfWidth,halfHeight), max( rotateX(width,0,cosAngle,sinAngle,halfWidth,halfHeight), max( rotateX(0,height,cosAngle,sinAngle,halfWidth,halfHeight), rotateX(width,height,cosAngle,sinAngle,halfWidth,halfHeight) ) ) ) - width;
    yOffset = max( rotateY(0,0,cosAngle,sinAngle,halfWidth,halfHeight), max( rotateY(width,0,cosAngle,sinAngle,halfWidth,halfHeight), max( rotateY(0,height,cosAngle,sinAngle,halfWidth,halfHeight), rotateY(width,height,cosAngle,sinAngle,halfWidth,halfHeight) ) ) ) - height;

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
          int newX = rotateX((double)j+k/2.0,i,cosAngle,sinAngle,halfWidth,halfHeight) - xOffset;
          int newY = rotateY((double)j+k/2.0,i,cosAngle,sinAngle,halfWidth,halfHeight) - yOffset;

          if( (newX < 0) || (newX > width-1) || (newY < 0) || (newY > height-1) ) continue;

          int p = width*newY + newX;

          r+=images[bestImage+1][3*p+0];
          g+=images[bestImage+1][3*p+1];
          b+=images[bestImage+1][3*p+2];
          m+=masks[bestImage+1][p];
          ++used;

          newX = rotateX(j,(double)i+k/2.0,cosAngle,sinAngle,halfWidth,halfHeight) - xOffset;
          newY = rotateY(j,(double)i+k/2.0,cosAngle,sinAngle,halfWidth,halfHeight) - yOffset;

          if( (newX < 0) || (newX > width-1) || (newY < 0) || (newY > height-1) ) continue;

          p = width*newY + newX;

          r+=images[bestImage+1][3*p+0];
          g+=images[bestImage+1][3*p+1];
          b+=images[bestImage+1][3*p+2];
          m+=masks[bestImage+1][p];
          ++used;
        }

        int newX = rotateX(j,i,cosAngle,sinAngle,halfWidth,halfHeight) - xOffset;
        int newY = rotateY(j,i,cosAngle,sinAngle,halfWidth,halfHeight) - yOffset;

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

  processing_images->Finish();

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

void RunRotational( string inputName, string outputName, vector< string > inputDirectory, int numIter, int angleOffset, double imageScale, double renderScale, int numSkip, bool trueColor, string fileName )
{
  vector< vector< unsigned char > > images, masks;
  vector< pair< int, int > > dimensions;

  bool loadData = (fileName != " ");

  if( loadData )
  {
    ifstream data( fileName, ios::binary );

    if(data.is_open())
    {
      int numImages;
      data.read( (char *)&numImages, sizeof(int) );
      data.read( (char *)&imageScale, sizeof(double) );
      data.read( (char *)&renderScale, sizeof(double) );
    
      images.resize( numImages );
      masks.resize( numImages );
      
      for( int i = 0; i < numImages; ++i )
      {
        int width, height;
        data.read( (char *)&width, sizeof(int) );
        data.read( (char *)&height, sizeof(int) );

        images[i].resize( width*height*3 );
        data.read((char *)images[i].data(), images[i].size()*sizeof(unsigned char));

        masks[i].resize( width*height );
        data.read((char *)masks[i].data(), masks[i].size()*sizeof(unsigned char));

        dimensions.push_back(pair< int, int >(width,height));
      }
    }
    else
    {
      loadData = false;
    }
  }
  if( !loadData )
  {
    for( int i = 0; i < inputDirectory.size(); ++i )
    {
      string imageDirectory = inputDirectory[i];
      if( imageDirectory.back() != '/' ) imageDirectory += '/';
      generateRotationalThumbnails( imageDirectory, images, masks, dimensions, imageScale, renderScale );
    }

    if( fileName != " " )
    {
      ofstream data( fileName, ios::binary );

      int numImages = images.size();

      data.write( (char *)&numImages, sizeof(int) );
      data.write( (char *)&imageScale, sizeof(double) );
      data.write( (char *)&renderScale, sizeof(double) );

      for( int i = 0; i < numImages; ++i )
      {
        int width = dimensions[i].first;
        int height = dimensions[i].second;

        data.write( (char *)&width, sizeof(int) );
        data.write( (char *)&height, sizeof(int) );

        data.write((char *)&images[i].front(), images[i].size() * sizeof(unsigned char)); 
        data.write((char *)&masks[i].front(), masks[i].size() * sizeof(unsigned char)); 
      }
    }
  }

  buildRotationalImage( inputName, outputName, images, masks, dimensions, renderScale/imageScale, numIter, angleOffset, numSkip, trueColor );
}