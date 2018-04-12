#include "Video.h"

void RunMosaic( string inputName, string outputName, vector< string > inputDirectory, int numHorizontal, bool trueColor, int mosaicTileWidth, int mosaicTileHeight, int imageTileWidth, int repeat, string fileName, int frames, int skip )
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

  int width = VImage::new_memory().vipsload( (char *)inputImages[0].c_str() ).width();
  int height = VImage::new_memory().vipsload( (char *)inputImages[0].c_str() ).height();

  if( mosaicTileWidth == 0 ) mosaicTileWidth = width/numHorizontal;
  if( imageTileWidth == 0 ) imageTileWidth = mosaicTileWidth;
  if( mosaicTileHeight == 0 ) mosaicTileHeight = mosaicTileWidth;

  int imageTileHeight = imageTileWidth*mosaicTileHeight/mosaicTileWidth;

  int numImages = 0;
  vector< cropType > cropData;
  vector< vector< unsigned char > > mosaicTileData;
  vector< vector< unsigned char > > imageTileData;
  vector< vector< int > > d;
  vector< int > sequenceStarts;
  vector< int > idx;

  bool loadData = (fileName != " ");

  if( loadData )
  {
    ifstream data( fileName, ios::binary );

    if(data.is_open())
    {
      data.read( (char *)&numImages, sizeof(int) );
      data.read( (char *)&mosaicTileWidth, sizeof(int) );
      data.read( (char *)&mosaicTileHeight, sizeof(int) );
      data.read( (char *)&imageTileWidth, sizeof(int) );
      data.read( (char *)&imageTileHeight, sizeof(int) );

      for( int i = 0; i < numImages; ++i )
      {
        int strLen, cropX, cropY, rot;
        bool flip;
        string str;
        data.read((char *)&strLen, sizeof(int));
        char* temp = new char[strLen+1];
        data.read(temp, strLen);
        temp[strLen] = '\0';
        str = temp;
        delete [] temp;
        data.read((char *)&cropX, sizeof(int));
        data.read((char *)&cropY, sizeof(int));
        data.read((char *)&rot, sizeof(int));
        data.read((char *)&flip, sizeof(bool));

        cropData.push_back(make_tuple(str,cropX,cropY,rot,flip));
      }
    
      mosaicTileData.resize( numImages );
      
      for( int i = 0; i < numImages; ++i )
      {
        mosaicTileData[i].resize( mosaicTileWidth*mosaicTileHeight*3 );
        data.read((char *)mosaicTileData[i].data(), mosaicTileData[i].size()*sizeof(unsigned char));
      }

      if( imageTileHeight != mosaicTileHeight )
      {
        imageTileData.resize( numImages );
        
        for( int i = 0; i < numImages; ++i )
        {
          imageTileData[i].resize( imageTileWidth*imageTileHeight*3 );
          data.read((char *)imageTileData[i].data(), imageTileData[i].size()*sizeof(unsigned char));
        } 
      }
    }
    else
    {
      loadData = false;
    }
  }

  int tileArea = mosaicTileWidth*mosaicTileHeight*3;
  int imageTileArea = imageTileWidth*imageTileHeight*3;
  int numVertical = int( (double)height / (double)width * (double)numHorizontal * (double)mosaicTileWidth/(double)mosaicTileHeight );

  if( !loadData )
  {
    for( int i = 0; i < inputDirectory.size(); ++i )
    {
      string imageDirectory = inputDirectory[i];
      if( imageDirectory.back() != '/' ) imageDirectory += '/';
      generateThumbnails( cropData, mosaicTileData, imageTileData, imageDirectory, mosaicTileWidth, mosaicTileHeight, imageTileWidth, imageTileHeight );

      idx.resize(mosaicTileData.size());
      iota(idx.begin()+numImages, idx.end(), numImages);

      sort(idx.begin()+numImages, idx.end(), [&cropData](int i1, int i2) {return get<0>(cropData[i1]) < get<0>(cropData[i2]);});

      for( int j = numImages, l = d.size(); j < mosaicTileData.size()-frames; j+=skip, ++l )
      {
        d.push_back(vector< int >(tileArea*frames));
        sequenceStarts.push_back(j);
        for( int k = 0; k < frames; ++k )
        {
          for( int p = 0; p < tileArea; ++p )
          {
            d[l][k*tileArea+p] = mosaicTileData[idx[j+k]][p];
          }
        }
      }

      numImages = mosaicTileData.size();
    }

    if( numImages == 0 ) 
    {
      cout << "No valid images found. The directory might not contain any images, or they might be too small, or they might not be a valid format." << endl;
      return;
    }

    if( fileName != " " )
    {
      ofstream data( fileName, ios::binary );
      data.write( (char *)&numImages, sizeof(int) );

      data.write( (char *)&mosaicTileWidth, sizeof(int) );

      data.write( (char *)&mosaicTileHeight, sizeof(int) );

      data.write( (char *)&imageTileWidth, sizeof(int) );

      data.write( (char *)&imageTileHeight, sizeof(int) );

      for( int i = 0; i < numImages; ++i )
      {
        string str = get<0>(cropData[i]);
        int strLen = str.length();
        data.write((char *)&strLen,sizeof(strLen));
        data.write((char *)str.c_str(), strLen);
        data.write((char *)&get<1>(cropData[i]),sizeof(int));
        data.write((char *)&get<2>(cropData[i]),sizeof(int));
        data.write((char *)&get<3>(cropData[i]),sizeof(int));
        data.write((char *)&get<4>(cropData[i]),sizeof(bool));
      }

      for( int i = 0; i < numImages; ++i )
      {
        data.write((char *)&mosaicTileData[i].front(), mosaicTileData[i].size() * sizeof(unsigned char)); 
      }

      if( imageTileWidth != mosaicTileWidth )
      {
        for( int i = 0; i < numImages; ++i )
        {
          data.write((char *)&imageTileData[i].front(), imageTileData[i].size() * sizeof(unsigned char)); 
        }
      }
    }
  }

  vector< vector< vector< int > > > e( numVertical, vector< vector< int > >( numHorizontal, vector< int >(frames*tileArea) ) );
  //vector< vector< int > > d( ceil(((double)numImages-(double)frames)/(double)skip), vector< int >(tileArea*frames) );

  vector< vector< int > > starts(numVertical, vector< int >( numHorizontal ) );

  int randStart = min(frames,(int)inputImages.size()-frames);

  for( int i = 0; i < numVertical; ++i )
    for( int j = 0; j < numHorizontal; ++j )
      starts[i][j] = rand()%randStart;

  g_mkdir(outputName.c_str(), 0777);
  
  vector< vector< vector< int > > > mosaic( inputImages.size()/frames+1, vector< vector< int > >( numVertical, vector< int >( numHorizontal, -1 ) ) );

  my_kd_tree_t mat_index(tileArea, d, 10 );

  size_t ret_index;
  int out_dist_sqr;
  nanoflann::KNNResultSet<int> resultSet(1);

  ProgressBar *processing_video = new ProgressBar(inputImages.size(), "Processing video");

  for( int p = 0; p < inputImages.size(); ++p )
  {
    VImage image = VImage::vipsload( (char *)inputImages[p].c_str() );
    image = image.resize( (double)(numHorizontal*mosaicTileWidth) / (double)(image.width()) );
    unsigned char * c = ( unsigned char * )image.data();

    for( int i = 0; i < numVertical; ++i )
      for( int j = 0; j < numHorizontal; ++j )
      {
        int p2 = p - starts[i][j];
        if( p2 >= 0 )
        {
          for( int y = i*mosaicTileHeight,ii=0; y < (i+1)*mosaicTileHeight; ++y )
            for( int x = j*mosaicTileWidth; x < (j+1)*mosaicTileWidth; ++x )
            {
              e[i][j][tileArea*(p2%frames)+ii++] = c[3*(image.width()*y+x)+0];
              e[i][j][tileArea*(p2%frames)+ii++] = c[3*(image.width()*y+x)+1];
              e[i][j][tileArea*(p2%frames)+ii++] = c[3*(image.width()*y+x)+2];
            }
          if(p2%frames == frames-1 && p2/frames < mosaic.size() )
          {
            mat_index.query( &e[i][j][0], 1, &ret_index, &out_dist_sqr );
            mosaic[p2/frames][i][j] = sequenceStarts[ret_index];
          }
        }
      }
    processing_video->Increment();
  }

  cout << endl;

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
                data[ 3 * ( numHorizontal * imageTileWidth * ( i * imageTileHeight + y ) + j * imageTileWidth + x ) ] = mosaicTileData[idx[mosaic[pp/frames][i][j]+(pp%frames)]][y2++];
                data[ 3 * ( numHorizontal * imageTileWidth * ( i * imageTileHeight + y ) + j * imageTileWidth + x ) + 1 ] = mosaicTileData[idx[mosaic[pp/frames][i][j]+(pp%frames)]][y2++];
                data[ 3 * ( numHorizontal * imageTileWidth * ( i * imageTileHeight + y ) + j * imageTileWidth + x ) + 2] = mosaicTileData[idx[mosaic[pp/frames][i][j]+(pp%frames)]][y2++];
              }
              else
              {
                data[ 3 * ( numHorizontal * imageTileWidth * ( i * imageTileHeight + y ) + j * imageTileWidth + x ) ] = imageTileData[idx[mosaic[pp/frames][i][j]+(pp%frames)]][y2++];
                data[ 3 * ( numHorizontal * imageTileWidth * ( i * imageTileHeight + y ) + j * imageTileWidth + x ) + 1 ] = imageTileData[idx[mosaic[pp/frames][i][j]+(pp%frames)]][y2++];
                data[ 3 * ( numHorizontal * imageTileWidth * ( i * imageTileHeight + y ) + j * imageTileWidth + x ) + 2] = imageTileData[idx[mosaic[pp/frames][i][j]+(pp%frames)]][y2++];
              }
            }
          }
        }
      }
    }
    VImage::new_from_memory( data, numHorizontal*numVertical*imageTileArea, numHorizontal*imageTileWidth, numVertical*imageTileHeight, 3, VIPS_FORMAT_UCHAR ).vipssave((char *)outputImages[p].c_str());
    rendering_video->Increment();
  }

  cout << endl;
}
