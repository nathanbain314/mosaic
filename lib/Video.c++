#include "Video.h"

void videoThread( int start, int end, int numHorizontal, int p, int frames, int mosaicTileWidth, int mosaicTileHeight, int tileArea, int width, my_kd_tree_t* mat_index, vector< vector< float > > &mosaicTileDataFloat, vector< vector< vector< float > > > &e, vector< vector< int > > &starts, vector< vector< vector< int > > > &mosaic, vector< size_t > &sequenceStarts, float *c, float *edgeData, bool bruteForce )
{
  size_t ret_index;
  float out_dist_sqr;
  nanoflann::KNNResultSet<int> resultSet(1);

  int frameLength = tileArea * frames;

  for( int i = start; i < end; ++i )
  {
    for( int j = 0; j < numHorizontal; ++j )
    {
      int p2 = p - starts[i][j];

      if( p2 >= 0 )
      {
        for( int y = i*mosaicTileHeight,ii=tileArea*(p2%frames); y < (i+1)*mosaicTileHeight; ++y )
        {
          for( int x = j*mosaicTileWidth, jj = 3*(width*y+x); x < (j+1)*mosaicTileWidth; ++x, ii+=3, jj+=3 )
          {
            e[i][j][ii+0] = c[jj+0];
            e[i][j][ii+1] = c[jj+1];
            e[i][j][ii+2] = c[jj+2];
            e[i][j][tileArea*frames+ii+0] = edgeData[jj/3];
            e[i][j][tileArea*frames+ii+1] = 1.0f;
            e[i][j][tileArea*frames+ii+2] = 1.0f;
          }
        }

        if(p2%frames == frames-1 && p2/frames < mosaic.size() )
        {
          if( bruteForce )
          {
            float bestDifference = -1.0f;

            for( int index = 0; index < sequenceStarts.size(); ++index )
            {
              int startIndex = sequenceStarts[index];
              float difference = 0.0f;
              
              for( int frame = 0, index3 = 0; frame < frames; ++frame )
              {
                for( int index2 = 0; index2 < tileArea; ++index2, ++index3 )
                {
                  float t = e[i][j][index3+frameLength] * ( mosaicTileDataFloat[startIndex+frame][index2] - e[i][j][index3] );

                  difference += t*t;
                }
              }

              if( difference < 0 || difference < bestDifference )
              {
                ret_index = index;

                bestDifference = difference;
              }
            }
          }
          else
          {
            mat_index->query( &e[i][j][0], 1, &ret_index, &out_dist_sqr );
          }

          mosaic[p2/frames][i][j] = sequenceStarts[ret_index];
        }
      }
    }
  }
}

void batchLoadHelper( int start, int end, vector< cropType > &cropData, vector< vector< unsigned char > > &mosaicTileData, vector< vector< unsigned char > > &imageTileData, vector< size_t > &inputTileStarts, vector< string > &inputDirectory, int mosaicTileWidth, int mosaicTileHeight, int imageTileWidth, int imageTileHeight, bool recursiveSearch, ProgressBar* loadingFrames )
{
  size_t numImages = 0;

  for( size_t i = start; i < end; ++i )
  {
    if( start == 0 ) loadingFrames->Increment();

    string imageDirectory = inputDirectory[i];
    if( imageDirectory.back() != '/' ) imageDirectory += '/';

    vector< vector< unsigned char > > mosaicTileData2, imageTileData2;

    generateThumbnails( cropData, mosaicTileData2, imageTileData2, inputDirectory, imageDirectory, mosaicTileWidth, mosaicTileHeight, imageTileWidth, imageTileHeight, false, false, 0, false, true, recursiveSearch, false );

    vector< size_t > idx;
    idx.resize(numImages);
    iota(idx.begin(), idx.end(), 0);
    sort(idx.begin(), idx.end(), [&cropData](size_t i1, size_t i2) {return get<0>(cropData[i1]) < get<0>(cropData[i2]);});

    for( int i = 0; i < idx.size(); ++i )
    {
      mosaicTileData.push_back(mosaicTileData2[idx[i]]);
    }

    if( mosaicTileWidth != imageTileWidth ) 
    {
      for( int i = 0; i < idx.size(); ++i )
      {
        imageTileData.push_back(imageTileData2[idx[i]]);
      }
    }

    inputTileStarts.push_back(numImages);

    numImages = mosaicTileData.size();

    inputTileStarts.push_back(numImages);
  }
}

void RunVideo( string inputName, string outputName, vector< string > inputDirectory, int numHorizontal, int mosaicTileWidth, int mosaicTileHeight, int imageTileWidth, int repeat, string fileName, int frames, int skip, bool recursiveSearch, bool batchLoad, bool bruteForce, float edgeWeight, bool smoothImage )
{
  vector< string > inputImages;
  vector< string > outputImages;

  if( inputName.back() != '/' ) inputName += '/';
  if( outputName.back() != '/' ) outputName += '/';

  DIR *dir;
  struct dirent *ent;

  if ((dir = opendir (inputName.c_str())) != NULL) 
  {
    while ((ent = readdir (dir)) != NULL)
    {   
      if( ent->d_name[0] != '.' && vips_foreign_find_load( string( inputName + ent->d_name ).c_str() ) != NULL )
      {
        inputImages.push_back( inputName + ent->d_name );
        outputImages.push_back( outputName + ent->d_name );
      }
    }
  }

  sort(inputImages.begin(), inputImages.end());
  sort(outputImages.begin(), outputImages.end());

  int width = VImage::new_memory().vipsload( (char *)inputImages[0].c_str() ).autorot().width();
  int height = VImage::new_memory().vipsload( (char *)inputImages[0].c_str() ).autorot().height();

  if( mosaicTileWidth == 0 ) mosaicTileWidth = width/numHorizontal;
  if( imageTileWidth == 0 ) imageTileWidth = mosaicTileWidth;
  if( mosaicTileHeight == 0 ) mosaicTileHeight = mosaicTileWidth;

  int imageTileHeight = imageTileWidth*mosaicTileHeight/mosaicTileWidth;

  size_t numImages = 0;
  vector< cropType > cropData;
  vector< vector< unsigned char > > mosaicTileData;
  vector< vector< unsigned char > > imageTileData;
  float *d;
  vector< size_t > sequenceStarts;
  vector< size_t > inputTileStarts;

  bool loadData = (fileName != " ");

  if( loadData )
  {
    ifstream data( fileName, ios::binary );

    if(data.is_open())
    {
      data.read( (char *)&numImages, sizeof(size_t) );
      data.read( (char *)&mosaicTileWidth, sizeof(int) );
      data.read( (char *)&mosaicTileHeight, sizeof(int) );
      data.read( (char *)&imageTileWidth, sizeof(int) );
      data.read( (char *)&imageTileHeight, sizeof(int) );
    
      mosaicTileData.resize( numImages );
      
      for( size_t i = 0; i < numImages; ++i )
      {
        mosaicTileData[i].resize( mosaicTileWidth*mosaicTileHeight*3 );
        data.read((char *)mosaicTileData[i].data(), mosaicTileData[i].size()*sizeof(unsigned char));
      }

      if( imageTileHeight != mosaicTileHeight )
      {
        imageTileData.resize( numImages );
        
        for( size_t i = 0; i < numImages; ++i )
        {
          imageTileData[i].resize( imageTileWidth*imageTileHeight*3 );
          data.read((char *)imageTileData[i].data(), imageTileData[i].size()*sizeof(unsigned char));
        }
      }

      size_t inputTileStartsLength;
      data.read((char *)&inputTileStartsLength, sizeof(size_t));

      inputTileStarts.resize( inputTileStartsLength );
      data.read((char *)inputTileStarts.data(), inputTileStartsLength*sizeof(size_t));
    }
    else
    {
      loadData = false;
    }
  }

  size_t tileArea = mosaicTileWidth*mosaicTileHeight*3;
  size_t imageTileArea = imageTileWidth*imageTileHeight*3;
  int numVertical = int( (double)height / (double)width * (double)numHorizontal * (double)mosaicTileWidth/(double)mosaicTileHeight );

  int preW = width;
  width = mosaicTileWidth*numHorizontal;
  height = ceil((float)width * (float)height/preW);

  if( !loadData )
  {
    if( batchLoad )
    {
      string imageDirectory = inputDirectory[0];
      if( imageDirectory.back() != '/' ) imageDirectory += '/';

      int threads = numberOfCPUS();

      future< void > ret[threads];

      vector< cropType > cropDataThread[threads];
      vector< vector< unsigned char > > mosaicTileDataThread[threads];
      vector< vector< unsigned char > > imageTileDataThread[threads];
      vector< size_t > inputTileStartsThread[threads];

      generateThumbnails( cropDataThread[0], mosaicTileDataThread[0], imageTileDataThread[0], inputDirectory, imageDirectory, mosaicTileWidth, mosaicTileHeight, imageTileWidth, imageTileHeight, false, false, 0, false, true, recursiveSearch );

      inputDirectory.erase(inputDirectory.begin());

      ProgressBar *loadingFrames = new ProgressBar(inputDirectory.size()/threads+1, "Loading frames");

      for( int k = 0; k < threads; ++k )
      {
        ret[k] = async( launch::async, &batchLoadHelper, k*inputDirectory.size()/threads, (k+1)*inputDirectory.size()/threads, ref( cropDataThread[k] ), ref( mosaicTileDataThread[k] ), ref( imageTileDataThread[k] ), ref( inputTileStartsThread[k] ), ref( inputDirectory ), mosaicTileWidth, mosaicTileHeight, imageTileWidth, imageTileHeight, recursiveSearch, loadingFrames );
      }

      // Wait for threads to finish
      for( int k = 0; k < threads; ++k )
      { 
        cropData.insert(cropData.end(), cropDataThread[k].begin(), cropDataThread[k].end());
        mosaicTileData.insert(mosaicTileData.end(), mosaicTileDataThread[k].begin(), mosaicTileDataThread[k].end());
        if( mosaicTileWidth != imageTileWidth ) imageTileData.insert(imageTileData.end(), imageTileDataThread[k].begin(), imageTileDataThread[k].end());

        for( int k1 = 0; k1 < inputTileStartsThread[k].size(); ++k1 )
        {
          inputTileStartsThread[k][k1] += numImages;
        }

        inputTileStarts.insert(inputTileStarts.end(), inputTileStartsThread[k].begin(), inputTileStartsThread[k].end());

        numImages = mosaicTileData.size();

        ret[k].get();
      }

      loadingFrames->Finish();
    }
    else
    {
      for( int i = 0; i < inputDirectory.size(); ++i )
      {
        string imageDirectory = inputDirectory[i];
        if( imageDirectory.back() != '/' ) imageDirectory += '/';

        vector< vector< unsigned char > > mosaicTileData2;
        vector< vector< unsigned char > > imageTileData2;

        generateThumbnails( cropData, mosaicTileData2, imageTileData2, inputDirectory, imageDirectory, mosaicTileWidth, mosaicTileHeight, imageTileWidth, imageTileHeight, false, false, 0, false, false, recursiveSearch );

        vector< size_t > idx;
        idx.resize(numImages);
        iota(idx.begin(), idx.end(), 0);
        sort(idx.begin(), idx.end(), [&cropData](size_t i1, size_t i2) {return get<0>(cropData[i1]) < get<0>(cropData[i2]);});

        for( int i = 0; i < idx.size(); ++i )
        {
          mosaicTileData.push_back(mosaicTileData2[idx[i]]);
        }

        if( mosaicTileWidth != imageTileWidth ) 
        {
          for( int i = 0; i < idx.size(); ++i )
          {
            imageTileData.push_back(imageTileData2[idx[i]]);
          }
        }

        inputTileStarts.push_back(numImages);

        numImages = mosaicTileData.size();

        inputTileStarts.push_back(numImages);
      }
    }

    if( numImages == 0 ) 
    {
      cout << "No valid images found. The directory might not contain any images, or they might be too small, or they might not be a valid format." << endl;
      return;
    }

    if( fileName != " " )
    {
      ofstream data( fileName, ios::binary );
      data.write( (char *)&numImages, sizeof(size_t) );

      data.write( (char *)&mosaicTileWidth, sizeof(int) );

      data.write( (char *)&mosaicTileHeight, sizeof(int) );

      data.write( (char *)&imageTileWidth, sizeof(int) );

      data.write( (char *)&imageTileHeight, sizeof(int) );

      for( size_t i = 0; i < numImages; ++i )
      {
        data.write((char *)&mosaicTileData[i].front(), mosaicTileData[i].size() * sizeof(unsigned char)); 
      }

      if( imageTileWidth != mosaicTileWidth )
      {
        for( size_t i = 0; i < numImages; ++i )
        {
          data.write((char *)&imageTileData[i].front(), imageTileData[i].size() * sizeof(unsigned char)); 
        }
      }

      size_t inputTileStartsLength = inputTileStarts.size();

      data.write((char *)&inputTileStartsLength, sizeof(size_t));
      data.write((char *)&inputTileStarts.front(), inputTileStartsLength*sizeof(size_t));
    }
  }

  vector< vector< float > > mosaicTileDataFloat(numImages,vector< float >(tileArea));

  for( size_t i = 0; i < numImages; ++i )
  {
    for( size_t p = 0; p < tileArea; p += 3 )
    {
      rgbToLab(mosaicTileData[i][p+0],mosaicTileData[i][p+1],mosaicTileData[i][p+2],mosaicTileDataFloat[i][p+0],mosaicTileDataFloat[i][p+1],mosaicTileDataFloat[i][p+2]);
    }
  }

  my_kd_tree_t *mat_index;

  if( !bruteForce )
  {
    size_t matSize = 0;

    for( size_t i = 0; i < inputTileStarts.size(); i+=2 )
    {
      for( size_t j = inputTileStarts[i]; j < inputTileStarts[i+1]-frames; j+=skip )
      {
        ++matSize;
      }
    }

    d = (float *) malloc(matSize*tileArea*frames*sizeof(float));

    for( size_t i = 0, l = 0; i < inputTileStarts.size(); i+=2 )
    {
      for( size_t j = inputTileStarts[i]; j < inputTileStarts[i+1]-frames; j+=skip, ++l )
      {
        sequenceStarts.push_back(j);
        for( size_t k = 0; k < frames; ++k )
        {
          for( size_t p = 0; p < tileArea; ++p )
          {
            d[ l*tileArea*frames + k*tileArea + p ] = mosaicTileDataFloat[j+k][p];
          }
        }
      }
    }

    mat_index = new my_kd_tree_t(tileArea*frames, matSize, d, 10 );
  }
  else
  {
    for( size_t i = 0, l = 0; i < inputTileStarts.size(); i+=2 )
    {
      for( size_t j = inputTileStarts[i]; j < inputTileStarts[i+1]-frames; j+=skip, ++l )
      {
        sequenceStarts.push_back(j);
      }
    }
  }

  vector< vector< vector< float > > > e( numVertical, vector< vector< float > >( numHorizontal, vector< float >(frames*tileArea*2) ) );

  vector< vector< int > > starts(numVertical, vector< int >( numHorizontal ) );

  int randStart = min(frames,(int)inputImages.size()-frames);

  for( int i = 0; i < numVertical; ++i )
    for( int j = 0; j < numHorizontal; ++j )
      starts[i][j] = rand()%randStart;

  g_mkdir(outputName.c_str(), 0777);
  
  vector< vector< vector< int > > > mosaic( inputImages.size()/frames+1, vector< vector< int > >( numVertical, vector< int >( numHorizontal, -1 ) ) );

  ProgressBar *processing_video = new ProgressBar(inputImages.size(), "Processing video");

  int threads = numberOfCPUS();

  float * edgeData = new float[width*height];

  for( int p = 0; p < inputImages.size(); ++p )
  {
    VImage image = VImage::vipsload( (char *)inputImages[p].c_str() ).autorot();
    image = image.resize( (double)(numHorizontal*mosaicTileWidth) / (double)(image.width()) );
    unsigned char * c = ( unsigned char * )image.data();
    float * c2 = new float[3*width*height];

    for( int p2 = 0; p2 < width*height; ++p2 )
    {
      rgbToLab( c[3*p2+0], c[3*p2+1], c[3*p2+2], c2[3*p2+0], c2[3*p2+1], c2[3*p2+2] );
    }

    generateEdgeWeights( image, edgeData, mosaicTileHeight, edgeWeight, smoothImage );

    future< void > ret[threads];

    for( int k = 0; k < threads; ++k )
    {
      ret[k] = async( launch::async, &videoThread, k*numVertical/threads, (k+1)*numVertical/threads, numHorizontal, p, frames, mosaicTileWidth, mosaicTileHeight, tileArea, image.width(), ref(mat_index), ref( mosaicTileDataFloat ), ref(e), ref(starts), ref(mosaic), ref(sequenceStarts), c2, edgeData, bruteForce );
    }

    // Wait for threads to finish
    for( int k = 0; k < threads; ++k )
    {
      ret[k].get();
    }

    processing_video->Increment();
  }

  processing_video->Finish();

  unsigned char *data = new unsigned char[numHorizontal*numVertical*imageTileWidth*imageTileHeight*3];

  ProgressBar *rendering_video = new ProgressBar(inputImages.size(), "Rendering video");

  for( int p = 0; p < inputImages.size(); ++p )
  {
    for( int i = 0; i < numVertical; ++i )
    {
      for( int j = 0; j < numHorizontal; ++j )
      {
        for( int y = 0, y2=0; y < imageTileHeight; ++y )
        {
          for( int x = 0; x < imageTileWidth; ++x )
          {
            int pp = p-starts[i][j];
            if( pp < 0 || mosaic[pp/frames][i][j] < 0 )
            {
              data[ 3 * ( numHorizontal * imageTileWidth * ( i * imageTileHeight + y ) + j * imageTileWidth + x ) ] = 0;
              data[ 3 * ( numHorizontal * imageTileWidth * ( i * imageTileHeight + y ) + j * imageTileWidth + x ) + 1 ] = 0;
              data[ 3 * ( numHorizontal * imageTileWidth * ( i * imageTileHeight + y ) + j * imageTileWidth + x ) + 2] = 0;
            }
            else
            {
              if( mosaicTileWidth == imageTileWidth )
              {
                data[ 3 * ( numHorizontal * imageTileWidth * ( i * imageTileHeight + y ) + j * imageTileWidth + x ) ] = mosaicTileData[mosaic[pp/frames][i][j]+(pp%frames)][y2++];
                data[ 3 * ( numHorizontal * imageTileWidth * ( i * imageTileHeight + y ) + j * imageTileWidth + x ) + 1 ] = mosaicTileData[mosaic[pp/frames][i][j]+(pp%frames)][y2++];
                data[ 3 * ( numHorizontal * imageTileWidth * ( i * imageTileHeight + y ) + j * imageTileWidth + x ) + 2] = mosaicTileData[mosaic[pp/frames][i][j]+(pp%frames)][y2++];
              }
              else
              {
                data[ 3 * ( numHorizontal * imageTileWidth * ( i * imageTileHeight + y ) + j * imageTileWidth + x ) ] = imageTileData[mosaic[pp/frames][i][j]+(pp%frames)][y2++];
                data[ 3 * ( numHorizontal * imageTileWidth * ( i * imageTileHeight + y ) + j * imageTileWidth + x ) + 1 ] = imageTileData[mosaic[pp/frames][i][j]+(pp%frames)][y2++];
                data[ 3 * ( numHorizontal * imageTileWidth * ( i * imageTileHeight + y ) + j * imageTileWidth + x ) + 2] = imageTileData[mosaic[pp/frames][i][j]+(pp%frames)][y2++];
              }
            }
          }
        }
      }
    }
    VImage::new_from_memory( data, numHorizontal*numVertical*imageTileArea, numHorizontal*imageTileWidth, numVertical*imageTileHeight, 3, VIPS_FORMAT_UCHAR ).vipssave((char *)outputImages[p].c_str());
    rendering_video->Increment();
  }

  if( !bruteForce )
  {
    free(d);
  }

  rendering_video->Finish();
}
