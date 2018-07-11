#include "Collage.h"

void generateCollageThumbnails( string imageDirectory, vector< string > &inputDirectory, vector< vector< unsigned char > > &images, vector< vector< unsigned char > > &masks, vector< pair< int, int > > &dimensions, double scale, double renderScale, int minSize, int maxSize, bool recursiveSearch )
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
      else if( recursiveSearch && ent->d_name[0] != '.' && ent->d_type == DT_DIR )
      {
        inputDirectory.push_back( imageDirectory + ent->d_name );
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

      int width = image.width();
      int height = image.height();

      int maxDimension = max( width, height );

      if( minSize > -1 && ( image.width() < minSize || image.height() < minSize ) ) continue;
      if( maxSize > -1 && maxDimension > maxSize ) image = image.resize( (double)maxSize/(double)maxDimension );

      width = image.width();
      height = image.height();

      VImage white = VImage::black(width,height).invert();

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

rotateResult findSmallest( int x, int y ,int start, int end, int imageWidth, int imageHeight, int skip, int iteration, vector< int > &lastUsed, vector< vector< unsigned char > > &images, vector< vector< unsigned char > > &masks, vector< pair< int, int > > &dimensions, unsigned char * imageData, unsigned char * computeMaskData, int angleOffset, bool trueColor )
{
  int bestImage = -1;
  double bestDifference = 3<<16, bestAngle = 0;

  // Run through every image in thread area
  // Round down to nearest even number
  for( int k = start-start%2; k < end-end%2; k+=2 )
  {
    if( skip >=0 && iteration - lastUsed[k>>1] <= skip ) continue;

    // Rotate image for every angleOffset angles
    for( double a = 0; a < 360; a += angleOffset )
    {
      // Convert to radians
      double angle = a * 3.14159265/180.0;
      int width = dimensions[k].first;
      int height = dimensions[k].second;

      double maxDifference = bestDifference * (width+1) * (height+1);
      double skipDifference = maxDifference;

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

      double xOffset2 = (cosAngle-1.0)*halfWidth - sinAngle*halfHeight;
      double yOffset2 = sinAngle*halfWidth + (cosAngle-1.0)*halfHeight;

      // Traverse data for rotated image to find difference from input image
      for( int iy = max( y - newHeight/2, 0 ), i = iy - y + newHeight/2, endy = min( imageHeight - y + newHeight/2, newHeight); i < endy && difference < skipDifference; ++i, ++iy )
      {
        int ix = max( x - newWidth/2, 0 );
        int j = ix - x + newWidth/2;
        int endx = min( imageWidth - x + newWidth/2, newWidth);

        // Index of point in input image
        unsigned long long index = (unsigned long long)iy*(unsigned long long)imageWidth+(unsigned long long)ix;

        double icos = (double)i * cosAngle;
        double isin = (double)i * sinAngle;

        for( ; j < endx && difference < skipDifference; ++j, ++index )
        {
          // New x and y of rotated image point
          int newX = (double)j*cosAngle - isin - xOffset2 - xOffset;
          int newY = (double)j*sinAngle + icos - yOffset2 - yOffset;

          // Make sure that the point is within the image
          if( (newX < 0) || (newX > width-1) || (newY < 0) || (newY > height-1) ) continue;

          // Update the maxValue the difference can be to be a candidate for best image
          skipDifference -= bestDifference;

          // Make sure the the point is not fully colored
          if( computeMaskData[index] == 255 ) continue;

          // Point in image
          int p = width*newY + newX;

          // Make sure that the point is not transparent
          if( masks[k][p] == 0 ) continue;

          // Update the number of pixels that are used
          ++usedPixels;

          // Update skipDifference
          skipDifference += bestDifference;

          if( trueColor )
          {
            float l1,a1,b1,l2,a2,b2;
            // Convert rgb to rgb16
            vips_col_sRGB2scRGB_8(imageData[3ULL*index+0ULL],imageData[3ULL*index+1ULL],imageData[3ULL*index+2ULL], &l1,&a1,&b1 );
            vips_col_sRGB2scRGB_8(images[k][3*p+0],images[k][3*p+1],images[k][3*p+2], &l2,&a2,&b2 );
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
            int ir = imageData[3ULL*index+0ULL]-images[k][3*p+0];
            int ig = imageData[3ULL*index+1ULL]-images[k][3*p+1];
            int ib = imageData[3ULL*index+2ULL]-images[k][3*p+2];

            difference += ir*ir + ig*ig + ib*ib;
          }
        }
      }

      // If the image is more similar then choose this one as the best image and angle
      if( usedPixels > 0 && difference/usedPixels < bestDifference )
      {
        bestDifference = difference/usedPixels;
        bestImage = k;
        bestAngle = angle;
      }
    }
  }

  return make_tuple(bestImage, bestAngle, bestDifference );
}

void buildTopLevel( string outputImage, int start, int end, int outputWidth, int outputHeight, vector< pair< int, int > > &points, vector< pair< int, double > > &bestImages, vector< vector< unsigned char > > &images, vector< vector< unsigned char > > &masks, vector< pair< int, int > > &dimensions, double resize, bool trueColor, ProgressBar *topLevel )
{
  for( int tileOffsetY = start; tileOffsetY < end; tileOffsetY += 256 )
  {
    int tileHeight = min( outputHeight - tileOffsetY, 256 );// + ( tileOffsetY > 0 && tileOffsetY + 256 < outputHeight );

    for( int tileOffsetX = 0; tileOffsetX < outputWidth; tileOffsetX += 256 )
    {
      if( start == 0 ) topLevel->Increment();

      int tileWidth = min( outputWidth - tileOffsetX, 256 );// + ( tileOffsetX > 0 && tileOffsetX + 256 < outputWidth );

      unsigned char * tileMaskData = ( unsigned char * )calloc (tileHeight*tileWidth,sizeof(unsigned char));
      unsigned char * tileData = ( unsigned char * )calloc (tileHeight*tileWidth*3,sizeof(unsigned char));
      
      for( int point = 0; point < points.size(); ++point )
      {
        int x = points[point].first;
        int y = points[point].second;

        int bestImage = bestImages[point].first;
        double bestAngle = bestImages[point].second;

        // Set the values for the output image
        int width = dimensions[bestImage+1].first;
        int height = dimensions[bestImage+1].second;

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

        x*=resize;
        y*=resize;

        if( x - newWidth/2 > tileOffsetX + tileWidth || x + newWidth/2 < tileOffsetX || y - newHeight/2 > tileOffsetY + tileHeight || y + newHeight/2 < tileOffsetY ) continue;

        // Render the image onto the output image
        for( int i = 0; i < newHeight; ++i )
        {
          for( int j = 0; j < newWidth; ++j )
          {
            int newX = rotateX(j,i,cosAngle,sinAngle,halfWidth,halfHeight) - xOffset;
            int newY = rotateY(j,i,cosAngle,sinAngle,halfWidth,halfHeight) - yOffset;

            if( (newX < 0) || (newX > width-1) || (newY < 0) || (newY > height-1) ) continue;

            int p = width*newY + newX;

            // Make sure that image pixel is not transparent
            if( masks[bestImage+1][p] == 0 ) continue;

            int ix = x + j - newWidth/2;
            int iy = y + i - newHeight/2;

            ix -= tileOffsetX;// > 0 ? tileOffsetX-1 : 0;
            iy -= tileOffsetY;// > 0 ? tileOffsetY-1: 0;

            // Make sure that pixel is inside image
            if( (ix < 0) || (ix > tileWidth - 1) || (iy < 0) || (iy > tileHeight - 1) ) continue;

            // Set index
            unsigned long long index = iy*tileWidth+ix;

            // Make sure that output pixel is transparent
            if( tileMaskData[index] == 255 ) continue;

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

            // Set output colors
            tileData[3*index+0] = min(tileData[3*index+0]+int((double)(255-tileMaskData[index])*(double)r/255.0),255);
            tileData[3*index+1] = min(tileData[3*index+1]+int((double)(255-tileMaskData[index])*(double)g/255.0),255);
            tileData[3*index+2] = min(tileData[3*index+2]+int((double)(255-tileMaskData[index])*(double)b/255.0),255);
            
            // Set output transparency mask
            tileMaskData[index] = min(255,m+(int)tileMaskData[index]);
          }
        }
      }

      VImage::new_from_memory( tileData, tileHeight*tileWidth*3, tileWidth, tileHeight, 3, VIPS_FORMAT_UCHAR ).jpegsave((char *)string(outputImage).append(to_string(tileOffsetX/256)+"_"+to_string(tileOffsetY/256)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );
      
      free( tileData );
      free( tileMaskData );
    }
  }
}

void buildCollage( string inputImage, string outputImage, vector< vector< unsigned char > > &images, vector< vector< unsigned char > > &masks, vector< pair< int, int > > &dimensions, double resize, int angleOffset, int fillPercentage, bool trueColor, int skip )
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
  unsigned char * computeMaskData = ( unsigned char * )calloc (imageWidth*imageHeight,sizeof(unsigned char));

  vector< pair< int, int > > points;
  vector< pair< int, double > > bestImages;

  vector< int > lastUsed( images.size()/2, -skip-1 );

  unsigned long long alphaSum = 0;

  unsigned long long stopAlphaSum = (unsigned long long)imageWidth * (unsigned long long)imageHeight * 255ULL * (unsigned long long)fillPercentage / 100ULL;

  ProgressBar *processing_images = new ProgressBar(stopAlphaSum, "Generating collage");

  int x, y, iteration = 0;

  while( alphaSum < stopAlphaSum )
  {
    do
    {
      // Place image at random point
      y = rand()%(imageHeight);
      x = rand()%(imageWidth);
    }
    // Only choose point if it is not already taken
    while( computeMaskData[y*imageWidth+x] == 255);

    processing_images->Progressed(alphaSum);

    int bestImage = -1;
    double bestDifference = DBL_MAX, bestAngle = 0;

    int threads = sysconf(_SC_NPROCESSORS_ONLN);

    future< rotateResult > ret[threads];

    for( int k = 0; k < threads; ++k )
    {
      ret[k] = async( launch::async, &findSmallest, x, y, k*images.size()/threads, (k+1)*images.size()/threads, imageWidth, imageHeight, skip, iteration, ref(lastUsed), ref(images), ref(masks), ref(dimensions), imageData, computeMaskData, angleOffset, trueColor );
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

        int t = computeMaskData[index];

        computeMaskData[index] = min((int)computeMaskData[index] + (int)masks[bestImage][p],255);

        alphaSum += (unsigned long long)(computeMaskData[index] - t);
      }
    }

    points.push_back( pair< int, int >(x,y) );
    bestImages.push_back( pair< int, double >(bestImage,bestAngle) );

    lastUsed[bestImage>>1] = iteration;

    ++iteration;
  }

  processing_images->Finish();
  
  // Save image as static image or zoomable image
  if( vips_foreign_find_save( outputImage.c_str() ) != NULL )
  {
    // Create black images of output and masks
    unsigned char * outputData = ( unsigned char * )calloc (outputSize,sizeof(unsigned char));
    unsigned char * outputMaskData = ( unsigned char * )calloc ((unsigned long long)outputWidth*(unsigned long long)outputHeight,sizeof(unsigned char));

    ProgressBar *buildingImage = new ProgressBar(points.size(), "Building image");

    for( int point = 0; point < points.size(); ++point )
    {
      buildingImage->Increment();

      int x = points[point].first;
      int y = points[point].second;

      int bestImage = bestImages[point].first;
      double bestAngle = bestImages[point].second;
      
      // Set the values for the output image
      int width = dimensions[bestImage+1].first;
      int height = dimensions[bestImage+1].second;

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

    buildingImage->Finish();

    VImage::new_from_memory( outputData, outputSize, outputWidth, outputHeight, 3, VIPS_FORMAT_UCHAR ).vipssave((char *)outputImage.c_str());
  }
  else
  {  
    int level = (int)ceil(log2( max(outputWidth,outputHeight) ) );

    if( outputImage.back() == '/' ) outputImage = outputImage.substr(0, outputImage.size()-1);

    g_mkdir(string(outputImage).append("_files/").c_str(), 0777);
    g_mkdir(string(outputImage).append("_files/").append(to_string(level)).c_str(), 0777);

    int threads = sysconf(_SC_NPROCESSORS_ONLN);

    ProgressBar *topLevel = new ProgressBar(ceil((double)outputWidth/256.0)*ceil((double)outputHeight/((double)threads*256.0)), "Building top level");

    future< void > ret[threads];

    for( int k = 0; k < threads; ++k )
    {
      int start = k*outputHeight/threads;
      int end = (k+1)*outputHeight/threads;

      start = (start / 256 ) * 256;
      if( k+1 < threads ) end = (end / 256 ) * 256;

      ret[k] = async( launch::async, &buildTopLevel, string(outputImage).append("_files/"+to_string(level)+"/"), start, end, outputWidth, outputHeight, ref(points), ref(bestImages), ref(images), ref(masks), ref(dimensions), resize, trueColor, topLevel );
    }

    // Wait for threads to finish
    for( int k = 0; k < threads; ++k )
    {
      ret[k].get();
    }
    
    topLevel->Finish();

    ofstream dzi_file;
    dzi_file.open(string(outputImage).append(".dzi").c_str());
    dzi_file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
    dzi_file << "<Image xmlns=\"http://schemas.microsoft.com/deepzoom/2008\" Format=\"jpeg\" Overlap=\"0\" TileSize=\"256\">" << endl;
    dzi_file << "    <Size Height=\"" << outputHeight << "\" Width=\"" << outputWidth << "\"/>" << endl;
    dzi_file << "</Image>" << endl;
    dzi_file.close();

    int numLevels = 0;
    for( int o = ceil((double)outputWidth/256.0); o > 1; o = ceil((double)o/2.0) ) numLevels += o;

    ProgressBar *lowerLevels = new ProgressBar(numLevels, "Building lower levels");

    outputWidth = (int)ceil((double)outputWidth/ 128.0 );
    outputHeight = (int)ceil((double)outputHeight/ 128.0 );

    for( ; level > 0; --level )
    {
      outputWidth = (int)ceil((double)outputWidth/ 2.0 );
      outputHeight = (int)ceil((double)outputHeight/ 2.0 );

      string current = string(outputImage).append("_files/"+to_string(level-1)+"/");
      string upper = string(outputImage).append("_files/"+to_string(level)+"/");

      g_mkdir((char *)current.c_str(), 0777);

      for(int i = 1; i < outputWidth; i+=2)
      {
        lowerLevels->Increment();

        for(int j = 1; j < outputHeight; j+=2)
        {
          (VImage::jpegload((char *)string(upper).append(to_string(i-1)+"_"+to_string(j-1)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
          join(VImage::jpegload((char *)string(upper).append(to_string(i)+"_"+to_string(j-1)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL)).
          join(VImage::jpegload((char *)string(upper).append(to_string(i-1)+"_"+to_string(j)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
          join(VImage::jpegload((char *)string(upper).append(to_string(i)+"_"+to_string(j)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL),VIPS_DIRECTION_VERTICAL).
          jpegsave((char *)string(current).append(to_string(i>>1)+"_"+to_string(j>>1)+".jpeg").c_str() );
        }
      }
      if(outputWidth%2 == 1)
      {
        for(int j = 1; j < outputHeight; j+=2)
        {
          (VImage::jpegload((char *)string(upper).append(to_string(outputWidth-1)+"_"+to_string(j-1)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
          join(VImage::jpegload((char *)string(upper).append(to_string(outputWidth-1)+"_"+to_string(j)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_VERTICAL)).
          jpegsave((char *)string(current).append(to_string(outputWidth>>1)+"_"+to_string(j>>1)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );
        }
      }
      if(outputHeight%2 == 1)
      {
        for(int j = 1; j < outputWidth; j+=2)
        {
          (VImage::jpegload((char *)string(upper).append(to_string(j-1)+"_"+to_string(outputHeight-1)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
          join(VImage::jpegload((char *)string(upper).append(to_string(j)+"_"+to_string(outputHeight-1)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)),VIPS_DIRECTION_HORIZONTAL)).
          jpegsave((char *)string(current).append(to_string(j>>1)+"_"+to_string(outputHeight>>1)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );
        }
      }
      if(outputWidth%2 == 1 && outputHeight%2 == 1)
      {
        VImage::jpegload((char *)string(upper).append(to_string(outputWidth-1)+"_"+to_string(outputHeight-1)+".jpeg").c_str(), VImage::option()->set( "shrink", 2)).
        jpegsave((char *)string(current).append(to_string(outputWidth>>1)+"_"+to_string(outputHeight>>1)+".jpeg").c_str(), VImage::option()->set( "optimize_coding", true )->set( "strip", true ) );  
      }
    }

    lowerLevels->Finish();

    // Generate html file to view deep zoom image
    ofstream htmlFile(string(outputImage).append(".html").c_str());
    htmlFile << "<!DOCTYPE html>\n<html>\n<head><script src=\"js/openseadragon.min.js\"></script></head>\n<body>\n<style>\nhtml,\nbody,\n#collage\n{\nposition: fixed;\nleft: 0;\ntop: 0;\nwidth: 100%;\nheight: 100%;\n}\n</style>\n\n<div id=\"collage\"></div>\n\n<script>\nvar viewer = OpenSeadragon({\nid: 'collage',\nprefixUrl: 'icons/',\ntileSources:   \"" + outputImage + ".dzi\",\nminZoomImageRatio: 0,\nmaxZoomImageRatio: 1\n});\n</script>\n</body>\n</html>";
    htmlFile.close();
  }
}

void RunCollage( string inputName, string outputName, vector< string > inputDirectory, int angleOffset, double imageScale, double renderScale, int fillPercentage, bool trueColor, string fileName, int minSize, int maxSize, int skip, bool recursiveSearch )
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
      generateCollageThumbnails( imageDirectory, inputDirectory, images, masks, dimensions, imageScale, renderScale, minSize, maxSize, recursiveSearch );
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

  skip = min( skip, (((int)images.size())>>1)-1 );

  buildCollage( inputName, outputName, images, masks, dimensions, renderScale/imageScale, angleOffset, fillPercentage, trueColor, skip );
}