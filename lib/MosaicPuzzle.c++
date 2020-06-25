#include "MosaicPuzzle.h"
#include "mosaicTools.h"
#include "progress_bar.hpp"

#define JC_VORONOI_IMPLEMENTATION
#include "jc_voronoi.h"

Vertex compute2DPolygonCentroid( vector< Vertex > &vertices )
{
  Vertex centroid( 0, 0 );
  float signedArea = 0.0;

  for( int i = 0; i < vertices.size(); ++i )
  {
    float x0 = vertices[i].x;
    float y0 = vertices[i].y;
    float x1 = vertices[(i+1) % vertices.size()].x;
    float y1 = vertices[(i+1) % vertices.size()].y;
    float a = x0*y1 - x1*y0;
    signedArea += a;
    centroid.x += (x0 + x1)*a;
    centroid.y += (y0 + y1)*a;
  }

  signedArea *= 0.5;
  centroid.x /= ( 6.0 * signedArea );
  centroid.y /= ( 6.0 * signedArea );

  return centroid;
}

static void relax_points(const jcv_diagram* diagram, jcv_point* points)
{
  const jcv_site* sites = jcv_diagram_get_sites(diagram);
  for( int i = 0; i < diagram->numsites; ++i )
  {
    const jcv_site* site = &sites[i];
    jcv_point sum = site->p;

    const jcv_graphedge* edge = site->edges;

    vector< Vertex > polygon;

    while( edge )
    {
      polygon.push_back(Vertex(edge->pos[0].x,edge->pos[0].y));
      sum.x += edge->pos[0].x;
      sum.y += edge->pos[0].y;
      edge = edge->next;
    }

    Vertex p = compute2DPolygonCentroid( polygon );

    points[site->index].x = p.x;
    points[site->index].y = p.y;
  }
}

void generateVoronoiPolygons( vector< Polygon > &polygons, int count, int numrelaxations, int width, int height, string fileName )
{
  jcv_point* points = 0;

  points = (jcv_point*)malloc( sizeof(jcv_point) * (size_t)count);

  if( fileName != " " )
  {
    ifstream data( fileName, ios::binary );
    data.read( (char *)&count, sizeof(int) );
    cout << count << endl;
    data.read( (char *)points, count*sizeof(jcv_point));
  }
  else
  {
    srand(0);

    for( int i = 0; i < count; ++i )
    {
      points[i].x = (float)(rand()%width);
      points[i].y = (float)(rand()%height);
    }
  }

  jcv_rect bounding_box = { { 0.0f, 0.0f }, { (float)width, (float)height } };
  jcv_diagram diagram;
  const jcv_site* sites;
  jcv_graphedge* graph_edge;

  if( fileName == " " )
  {
    for( int i = 0; i < numrelaxations; ++i )
    {
      jcv_diagram diagram;
      memset(&diagram, 0, sizeof(jcv_diagram));
      jcv_diagram_generate(count, (const jcv_point*)points, &bounding_box, &diagram);

      relax_points(&diagram, points);

      jcv_diagram_free( &diagram );
    }
  }

  memset(&diagram, 0, sizeof(jcv_diagram));

  jcv_diagram_generate( count, (const jcv_point *)points, &bounding_box, &diagram);

  sites = jcv_diagram_get_sites(&diagram);
  for ( int i = 0; i < diagram.numsites; ++i)
  {
    Polygon P;

    graph_edge = sites[i].edges;

    while (graph_edge)
    {
      P.addVertex( graph_edge->pos[0].x, graph_edge->pos[0].y );
      P.addVertex( graph_edge->pos[1].x, graph_edge->pos[1].y );

      P.addEdge( P.vertices.size()-2, P.vertices.size()-1 );

      graph_edge = graph_edge->next;
    }

    P.makeClockwise();

    polygons.push_back( P );
  }

  jcv_diagram_free(&diagram);
}

void generateImagePolygonsThread( int start, int end, vector< string > &names, vector< Polygon > &imagePolygons, vector< vector< unsigned char > > &images, vector< vector< unsigned char > > &masks, vector< pair< int, int > > &dimensions, ProgressBar *processing_images )
{
  unsigned char *imageData, *maskData;

  // Iterate through all images in directory
  for( int i = start; i < end; ++i )
  {
    try
    {
      string str = names[i];

//      cout << str << endl;

      Polygon P, P2;

      polygonFromAlphaImage( P, names[i], 1 );

      decimate(P,.01);

      P.makeClockwise();
      P.center();

      if( P.vertices.size() == 0 )
      {
        continue;
      }

      imagePolygons.push_back( P );
      
      // Load image and create white one band image of the same size
      VImage image = VImage::vipsload( (char *)str.c_str() ).autorot().colourspace(VIPS_INTERPRETATION_sRGB);

      int left, top, centerImageWidth, centerImageHeight;

      left = image.find_trim( &top, &centerImageWidth, &centerImageHeight );

      image = image.extract_area( left, top, centerImageWidth, centerImageHeight );

      int width = image.width();
      int height = image.height();

      VImage mask = VImage::black(width,height).invert();

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

      // Extract mask of alpha image
      if( image.bands() == 4 )
      {
        mask = image.extract_band(3);
        
        image = image.flatten();
      }

      // Save data to vector
      imageData = ( unsigned char * )image.data();
      maskData = ( unsigned char * )mask.data();

      images.push_back( vector< unsigned char >(imageData, imageData + (3*width*height)));
      masks.push_back( vector< unsigned char >(maskData, maskData + (width*height)));
      dimensions.push_back( pair< int, int >(width,height) );

      if( start == 0 ) processing_images->Increment();
    }
    catch (...)
    {
    }
  }
}

void generateImagePolygons( string imageDirectory, vector< string > &inputDirectory, vector< Polygon > &imagePolygons, vector< vector< unsigned char > > &images, vector< vector< unsigned char > > &masks, vector< pair< int, int > > &dimensions, bool recursiveSearch )
{
  DIR *dir;
  struct dirent *ent;
  string str;

  int num_images = 0;

  vector< string > names;

  struct stat s;

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
      else if( recursiveSearch && ent->d_name[0] != '.' )
      {
        stat(ent->d_name, &s);
        if (s.st_mode & S_IFDIR)
        {
          inputDirectory.push_back( imageDirectory + ent->d_name );
        }
      }
    }
  }

  cout << endl;

  int threads = numberOfCPUS();

  ProgressBar *processing_images = new ProgressBar(ceil((float)num_images/threads), "Processing images");

  vector< vector< unsigned char > > imagesThread[threads];
  vector< vector< unsigned char > > masksThread[threads];
  vector< pair< int, int > > dimensionsThread[threads];
  vector< Polygon > imagePolygonsThread[threads];

  future< void > ret[threads];

  for( int k = 0; k < threads; ++k )
  {
    ret[k] = async( launch::async, &generateImagePolygonsThread, k*num_images/threads, (k+1)*num_images/threads, ref(names), ref(imagePolygonsThread[k]), ref(imagesThread[k]), ref(masksThread[k]), ref(dimensionsThread[k]), processing_images );
  }

  // Wait for threads to finish
  for( int k = 0; k < threads; ++k )
  {
    ret[k].get();

    images.insert(images.end(), imagesThread[k].begin(), imagesThread[k].end());
    masks.insert(masks.end(), masksThread[k].begin(), masksThread[k].end());
    dimensions.insert(dimensions.end(), dimensionsThread[k].begin(), dimensionsThread[k].end());
    imagePolygons.insert(imagePolygons.end(), imagePolygonsThread[k].begin(), imagePolygonsThread[k].end());
  }

  processing_images->Finish();
}

int rotateX( float x, float y, float cosAngle, float sinAngle, float halfWidth, float halfHeight )
{
  return cosAngle*(x-halfWidth) - sinAngle*(y-halfHeight) + halfWidth;
}

int rotateY( float x, float y, float cosAngle, float sinAngle, float halfWidth, float halfHeight )
{
  return sinAngle*(x-halfWidth) + cosAngle*(y-halfHeight) + halfHeight;
}

void computeLargerPolygon( vector< Vertex > &vertices, vector< Edge > &edges, Vertex &center, Polygon &P )
{
  vector< Vertex > verticesCopy = vertices;
  sort( vertices.begin(), vertices.end(), bind(grahamScanSort, placeholders::_1, placeholders::_2, center));

  for( int i = 0; i < vertices.size(); i += 2 )
  {
    Vertex v1 = vertices[i];

    bool skipVertex = false;

    for( int j = 0; j < edges.size(); ++j )
    {
      Vertex v2 = verticesCopy[edges[j].v1];
      Vertex v3 = verticesCopy[edges[j].v2];

      pair< float, float > t = intersection( center, v1, v2, v3 );

      if( t.first > 0.0000001 && t.first < 1 - 0.0000001 && t.second > 0 && t.second < 1 )
      {
        skipVertex = true;
        break;
      }
    }

    if( !skipVertex )
    {
      P.addVertex( v1 );
      if( P.vertices.size() > 1 )
      {
        P.addEdge(P.vertices.size()-2,P.vertices.size()-1);
        P.addVertex( v1 );
      }
    }
  }

  P.addVertex( P.vertices[0] );
  P.addEdge(P.vertices.size()-2,P.vertices.size()-1);
}

tuple< int, float, float, Vertex > generateSecondPassBestValues( int i2, int cellDistance, vector< Polygon > &imagePolygons, vector< Polygon > &polygons, vector< tuple< int, float, float, Vertex > > &bestValues, vector< vector< float > > &images, vector< vector< unsigned char > > &masks, vector< pair< int, int > > &dimensions, float *inputData, float * edgeData, int inputWidth, int inputHeight, float angleOffset, bool skipNearby, float sizePower, bool doDraw, ProgressBar *drawingImages )
{
  set< int > skipValues;

  vector< Vertex > vertices;
  vector< Edge > edges;

  vertices.push_back(Vertex(0,0));
  vertices.push_back(Vertex(inputWidth,0));
  vertices.push_back(Vertex(inputWidth,0));
  vertices.push_back(Vertex(inputWidth,inputHeight));
  vertices.push_back(Vertex(inputWidth,inputHeight));
  vertices.push_back(Vertex(0,inputHeight));
  vertices.push_back(Vertex(0,inputHeight));
  vertices.push_back(Vertex(0,0));

  edges.push_back(Edge(0,1));
  edges.push_back(Edge(2,3));
  edges.push_back(Edge(4,5));
  edges.push_back(Edge(6,7));

  vector< int > outsidePolygons;

  for( int i1 = 0; i1 < polygons.size(); ++i1 )
  {
    if( i1 == i2 ) continue;

    Vertex center(0,0);

    for( int j2 = 0; j2 < polygons[i1].vertices.size(); ++j2 )
    {
      center.x += polygons[i1].vertices[j2].x;
      center.y += polygons[i1].vertices[j2].y;
    }

    center.x /= polygons[i1].vertices.size();
    center.y /= polygons[i1].vertices.size();

    if( distance( get<3>( bestValues[i2] ), center ) < 4*cellDistance )
    {
      outsidePolygons.push_back( i1 );
      skipValues.insert( get<0>( bestValues[i1] ) );
    }
  }

  for( int i11 = 0; i11 < outsidePolygons.size(); ++i11 )
  {
    int i1 = outsidePolygons[i11];

    int bestImage   = get<0>( bestValues[i1] );
    float scale    = get<1>( bestValues[i1] );
    float rotation = get<2>( bestValues[i1] );
    Vertex offset   = get<3>( bestValues[i1] );

    int offsetX = floor(offset.x + 0.0001);
    int offsetY = floor(offset.y + 0.0001);

    Polygon P = imagePolygons[bestImage];

    Vertex po = P.computeOffset(rotation*180.0/3.14159265358979);

    P.rotateBy( rotation*180.0/3.14159265358979, false );

    P.scaleBy( scale );
    P.offsetBy( offset );

    for( int i = 0; i < P.edges.size(); ++i )
    {
      Vertex v1 = P.vertices[P.edges[i].v1];
      Vertex v2 = P.vertices[P.edges[i].v2];

      vertices.push_back( v1 );
      vertices.push_back( v2 );

      edges.push_back(Edge(vertices.size()-2,vertices.size()-1));
    }
  }

  Polygon R;

  computeLargerPolygon( vertices, edges, get<3>( bestValues[i2] ), R );

  decimate(R,0.01);

  R.makeClockwise();

  int i1 = -1;

  int bestImage = 0;
  float bestScale = 0;
  float bestRotation = 0;
  Vertex bestOffset = Vertex( 0, 0 );
  float bestDifference = 1000000000;

  for( int k1 = 0, k = 0; k1 < imagePolygons.size(); ++k1, ++k )
  {    
    if( skipNearby && skipValues.size() < imagePolygons.size() && find( skipValues.begin(), skipValues.end(), k ) != skipValues.end() ) continue;

    Polygon P = imagePolygons[k1];

    float scale, rotation;

    Vertex offset;

    findBestFit( P, R, scale, rotation, offset, angleOffset, 0 );

    float angle = rotation * 3.14159265/180.0;

    int offsetX = floor(offset.x+0.0001);
    int offsetY = floor(offset.y+0.0001);

    int width = dimensions[k].first;
    int height = dimensions[k].second;

    float skipDifference = bestDifference * (width+1) * (height+1);

    // Compute data for point rotation
    float cosAngle = cos(angle);
    float sinAngle = sin(angle);
    float halfWidth = (float)width/2.0;
    float halfHeight = (float)height/2.0;

    // Conpute side offset of rotated image from regular image
    int xOffset = max( rotateX(0,0,cosAngle,sinAngle,halfWidth,halfHeight), max( rotateX(width,0,cosAngle,sinAngle,halfWidth,halfHeight), max( rotateX(0,height,cosAngle,sinAngle,halfWidth,halfHeight), rotateX(width,height,cosAngle,sinAngle,halfWidth,halfHeight) ) ) ) - width;
    int yOffset = max( rotateY(0,0,cosAngle,sinAngle,halfWidth,halfHeight), max( rotateY(width,0,cosAngle,sinAngle,halfWidth,halfHeight), max( rotateY(0,height,cosAngle,sinAngle,halfWidth,halfHeight), rotateY(width,height,cosAngle,sinAngle,halfWidth,halfHeight) ) ) ) - height;

    // New width and height of rotated image
    int newWidth = width + 2*xOffset;
    int newHeight = height + 2*yOffset;

    float difference = 0;
    float usedPixels = 0;

    // New data for point rotation in reverse direction
    cosAngle = cos(-angle);
    sinAngle = sin(-angle);
    halfWidth = (float)newWidth/2.0;
    halfHeight = (float)newHeight/2.0;

    float xOffset2 = (cosAngle-1.0)*halfWidth - sinAngle*halfHeight;
    float yOffset2 = sinAngle*halfWidth + (cosAngle-1.0)*halfHeight;

    float scaleOffset = 1.0 / scale;

    // Traverse data for rotated image to find difference from input image
    for( float iy = offsetY, i = 0, endy = newHeight; i < endy; i += scaleOffset, ++iy )
    {
      int ix = offsetX;
      float j = 0;
      int endx = newWidth;

      // Index of point in input image
      unsigned long long index = (unsigned long long)iy*(unsigned long long)inputWidth+(unsigned long long)ix;

      float icos = i * cosAngle;
      float isin = i * sinAngle;

      for( ; j < endx; j += scaleOffset, ++index, ++ix )
      {
        if( (ix < 0) || (ix > inputWidth - 1) || (iy < 0) || (iy > inputHeight - 1) ) continue;

        float l = 0, a = 0, b = 0, m = 0;

        int numUsed = 0;

        for( float ni = i; ni < i + scaleOffset && ni < endy; ++ni )
        {
          for( float nj = j; nj < j + scaleOffset && nj < endx ; ++nj )
          {
            int nx = rotateX(nj,ni,cosAngle,sinAngle,halfWidth,halfHeight) - xOffset;// + halfWidth;
            int ny = rotateY(nj,ni,cosAngle,sinAngle,halfWidth,halfHeight) - yOffset;// + halfHeight;

            if( (nx < 0) || (nx > width-1) || (ny < 0) || (ny > height-1) ) continue;

            int p = width*ny + nx;

            if( masks[k][p] == 0 ) continue;

            ++numUsed;

            l += images[k][3*p+0];
            a += images[k][3*p+1];
            b += images[k][3*p+2];
            m += masks[k][p];
          }
        }

        if( numUsed > 0 )
        {
          l /= numUsed;
          a /= numUsed;
          b /= numUsed;
          m /= numUsed;

          if( m < 100 ) continue;

          ++usedPixels;

          // Compute the sum-of-squares for color difference
          float il = edgeData[index] * (inputData[3ULL*index+0ULL]-l);
          float ia = inputData[3ULL*index+1ULL]-a;
          float ib = inputData[3ULL*index+2ULL]-b;

          difference += sqrt( il*il + ia*ia + ib*ib );
        }
      }
    }

    // If the image is more similar then choose this one as the best image and angle
    if( k == 0 || ( usedPixels > 0 && difference/pow(usedPixels,sizePower) < bestDifference ) )
    {
      bestDifference = difference/pow(usedPixels,sizePower);

      bestImage = k;
      bestScale = scale;
      bestRotation = angle;
      bestOffset = offset;
    }

    if( doDraw ) drawingImages->Increment();
  }

  return make_tuple( bestImage, bestScale, bestRotation, bestOffset );
}

void generateBestValues( int start, int end, vector< int > &indices, vector< Polygon > &polygons, vector< Polygon > &imagePolygons, vector< tuple< int, float, float, Vertex > > &bestValues, vector< vector< float > > &images, vector< vector< unsigned char > > &masks, vector< pair< int, int > > &dimensions, float *inputData, float * edgeData, int inputWidth, int inputHeight, float angleOffset, float sizePower, float cellDistance, bool skipNearby, ProgressBar *findingBestImages )
{
  for( int st = start; st < end; ++st )
  {
    int i1 = indices[st];
    Polygon R = polygons[i1];


    set< int > skipValues;

    if( skipNearby )
    {
      Vertex center1(0,0);

      for( int j2 = 0; j2 < polygons[i1].vertices.size(); ++j2 )
      {
        center1.x += polygons[i1].vertices[j2].x;
        center1.y += polygons[i1].vertices[j2].y;
      }

      center1.x /= polygons[i1].vertices.size();
      center1.y /= polygons[i1].vertices.size();

      for( int i2 = 0; i2 < polygons.size(); ++i2 )
      {
        if( i1 == i2 || get<0>( bestValues[i2] ) < 0 ) continue;

        Vertex center(0,0);

        for( int j2 = 0; j2 < polygons[i2].vertices.size(); ++j2 )
        {
          center.x += polygons[i2].vertices[j2].x;
          center.y += polygons[i2].vertices[j2].y;
        }

        center.x /= polygons[i2].vertices.size();
        center.y /= polygons[i2].vertices.size();

        if( distance( center1, center ) < 4*cellDistance )
        {
          skipValues.insert( get<0>( bestValues[i2] ) );
        }
      }
    }


    int bestImage = 0;
    float bestScale = 0;
    float bestRotation = 0;
    Vertex bestOffset = Vertex( 0, 0 );
    float bestDifference = 1000000000;

    for( int k1 = 0, k = 0; k1 < imagePolygons.size(); ++k1, ++k )
    {
      if( skipNearby && skipValues.size() < imagePolygons.size() && find( skipValues.begin(), skipValues.end(), k ) != skipValues.end() ) continue;

      Polygon P = imagePolygons[k];

      float scale, rotation;

      Vertex offset;

      findBestFit( P, R, scale, rotation, offset, angleOffset, 0 );

      float angle = rotation * 3.14159265/180.0;

      int offsetX = floor(offset.x+0.0001);
      int offsetY = floor(offset.y+0.0001);

      int width = dimensions[k].first;
      int height = dimensions[k].second;

      float skipDifference = bestDifference * (width+1) * (height+1);

      // Compute data for point rotation
      float cosAngle = cos(angle);
      float sinAngle = sin(angle);
      float halfWidth = (float)width/2.0;
      float halfHeight = (float)height/2.0;

      // Conpute side offset of rotated image from regular image
      int xOffset = max( rotateX(0,0,cosAngle,sinAngle,halfWidth,halfHeight), max( rotateX(width,0,cosAngle,sinAngle,halfWidth,halfHeight), max( rotateX(0,height,cosAngle,sinAngle,halfWidth,halfHeight), rotateX(width,height,cosAngle,sinAngle,halfWidth,halfHeight) ) ) ) - width;
      int yOffset = max( rotateY(0,0,cosAngle,sinAngle,halfWidth,halfHeight), max( rotateY(width,0,cosAngle,sinAngle,halfWidth,halfHeight), max( rotateY(0,height,cosAngle,sinAngle,halfWidth,halfHeight), rotateY(width,height,cosAngle,sinAngle,halfWidth,halfHeight) ) ) ) - height;

      // New width and height of rotated image
      int newWidth = width + 2*xOffset;
      int newHeight = height + 2*yOffset;

      float difference = 0;
      float usedPixels = 0;

      // New data for point rotation in reverse direction
      cosAngle = cos(-angle);
      sinAngle = sin(-angle);
      halfWidth = (float)newWidth/2.0;
      halfHeight = (float)newHeight/2.0;

      float xOffset2 = (cosAngle-1.0)*halfWidth - sinAngle*halfHeight;
      float yOffset2 = sinAngle*halfWidth + (cosAngle-1.0)*halfHeight;

      float scaleOffset = 1.0 / scale;

      // Traverse data for rotated image to find difference from input image
      for( float iy = offsetY, i = 0, endy = newHeight; i < endy; i += scaleOffset, ++iy )
      {
        int ix = offsetX;
        float j = 0;
        int endx = newWidth;

        // Index of point in input image
        unsigned long long index = (unsigned long long)iy*(unsigned long long)inputWidth+(unsigned long long)ix;

        float icos = i * cosAngle;
        float isin = i * sinAngle;

        for( ; j < endx; j += scaleOffset, ++index, ++ix )
        {
          if( (ix < 0) || (ix > inputWidth - 1) || (iy < 0) || (iy > inputHeight - 1) ) continue;

          float l = 0, a = 0, b = 0, m = 0;

          int numUsed = 0;

          for( float ni = i; ni < i + scaleOffset && ni < endy; ++ni )
          {
            for( float nj = j; nj < j + scaleOffset && nj < endx ; ++nj )
            {
              int nx = rotateX(nj,ni,cosAngle,sinAngle,halfWidth,halfHeight) - xOffset;// + halfWidth;
              int ny = rotateY(nj,ni,cosAngle,sinAngle,halfWidth,halfHeight) - yOffset;// + halfHeight;

              if( (nx < 0) || (nx > width-1) || (ny < 0) || (ny > height-1) ) continue;

              int p = width*ny + nx;

              if( masks[k][p] < 100 ) continue;

              ++numUsed;

              l += images[k][3*p+0];
              a += images[k][3*p+1];
              b += images[k][3*p+2];
              m += masks[k][p];
//                break;
            }
//              break;
          }

          if( numUsed > 0 )
          {
            l /= numUsed;
            a /= numUsed;
            b /= numUsed;
            m /= numUsed;

            if( m < 100 ) continue;

            ++usedPixels;

            // Compute the sum-of-squares for color difference
            float il = edgeData[index] * (inputData[3ULL*index+0ULL]-l);
            float ia = inputData[3ULL*index+1ULL]-a;
            float ib = inputData[3ULL*index+2ULL]-b;

            difference += sqrt( il*il + ia*ia + ib*ib );
          }
        }
      }

      // If the image is more similar then choose this one as the best image and angle
      if( k == 0 || ( usedPixels > 0 && difference/pow(usedPixels,sizePower) < bestDifference ) )
      {
        bestDifference = difference/pow(usedPixels,sizePower);

        bestImage = k;
        bestScale = scale;
        bestRotation = angle;
        bestOffset = offset;
      }

      if( !start ) findingBestImages->Increment();
    }

    bestValues[i1] = make_tuple( bestImage, bestScale, bestRotation, bestOffset );
  }
}

void RunMosaicPuzzle( string inputName, string outputImage, vector< string > inputDirectory, int count, bool secondPass, float renderScale, float buildScale, float angleOffset, float edgeWeight, bool skipNearby, float sizePower, bool recursiveSearch, string fileName )
{
  vector< Polygon > polygons, imagePolygons;
  vector< vector< unsigned char > > images, masks;
  vector< pair< int, int > > dimensions;

  for( int i = 0; i < inputDirectory.size(); ++i )
  {
    string imageDirectory = inputDirectory[i];
    if( imageDirectory.back() != '/' ) imageDirectory += '/';
    generateImagePolygons( imageDirectory, inputDirectory, imagePolygons, images, masks, dimensions, recursiveSearch );
  }

  VImage inputImage = VImage::vipsload((char *)inputName.c_str()).autorot().resize(buildScale);
  unsigned char *inputData  = (unsigned char *)inputImage.data();
  unsigned char *grayscaleData = (unsigned char *)inputImage.colourspace(VIPS_INTERPRETATION_B_W).data();

  int inputWidth = inputImage.width(), inputHeight = inputImage.height();

  float * c2 = new float[3*inputWidth*inputHeight];

  for( int p = 0; p < inputWidth*inputHeight; ++p )
  {
    rgbToLab( inputData[3*p+0], inputData[3*p+1], inputData[3*p+2], c2[3*p+0], c2[3*p+1], c2[3*p+2] );
  }

  vector< vector< float > > images2;

  for( int i = 0; i < images.size(); ++i )
  {
    images2.push_back( vector< float >( images[i].size() ) );
    for( int p = 0; p < images[i].size(); p+=3 )
    {
      rgbToLab( images[i][p+0], images[i][p+1], images[i][p+2], images2[i][p+0], images2[i][p+1], images2[i][p+2] );
    }
  }


  // Load input image
  VImage edgeImage = VImage::vipsload( "edge.jpg" ).autorot().resize(buildScale);

  // Convert to a three band image
  if( edgeImage.bands() == 1 )
  {
    edgeImage = edgeImage.bandjoin(edgeImage).bandjoin(edgeImage);
  }
  if( edgeImage.bands() == 4 )
  {
    edgeImage = edgeImage.flatten();
  }

  float * edgeData = new float[inputWidth*inputHeight];

  generateEdgeWeights( edgeImage, edgeData, 24, edgeWeight );





  int outputWidth = inputWidth;
  int outputHeight = inputHeight;

  generateVoronoiPolygons( polygons, count, 100, outputWidth, outputHeight, fileName );


  float cellrange[4] = {1000000000,1000000000,-1000000000,-1000000000};

  int mi = polygons.size()/2;

  for( int i = 0; i < polygons[mi].vertices.size(); ++i )
  {
    cellrange[0] = min(cellrange[0],polygons[mi].vertices[i].x);
    cellrange[1] = min(cellrange[1],polygons[mi].vertices[i].y);
    cellrange[2] = max(cellrange[2],polygons[mi].vertices[i].x);
    cellrange[3] = max(cellrange[3],polygons[mi].vertices[i].y);
  }

  float cellDistance = 4*(cellrange[3]-cellrange[1])*(cellrange[3]-cellrange[1]) + (cellrange[2]-cellrange[0])*(cellrange[2]-cellrange[0]);


  vector< tuple< int, float, float, Vertex > > bestValues(polygons.size(), make_tuple(-1,0,0,Vertex(0,0)) );

  int threads = numberOfCPUS();

  ProgressBar *findingBestImages = new ProgressBar(polygons.size()*imagePolygons.size()/threads, "Finding best images");

  vector< int > indices( polygons.size() );
  iota( indices.begin(), indices.end(), 0 );

  // Shuffle the points so that patterens do not form
  shuffle( indices.begin(), indices.end(), default_random_engine(0));//time(NULL)) );

  future< void > ret[threads];

  for( int k = 0; k < threads; ++k )
  {
    ret[k] = async( launch::async, &generateBestValues, k*indices.size()/threads, (k+1)*indices.size()/threads, ref(indices), ref(polygons), ref(imagePolygons), ref(bestValues), ref(images2), ref(masks), ref(dimensions), c2, edgeData, inputWidth, inputHeight, angleOffset, sizePower, cellDistance, skipNearby, findingBestImages );
  }

  // Wait for threads to finish
  for( int k = 0; k < threads; ++k )
  {
    ret[k].get();
  }

  findingBestImages->Finish();



  unsigned char *outputData = ( unsigned char * )calloc(3*(outputWidth*renderScale)*(outputHeight*renderScale),sizeof(unsigned char));

  vector< int > indices2( ceil((float)polygons.size()/threads) );

  iota( indices2.begin(), indices2.end(), 0 );

  // Shuffle the points so that patterns do not form
  shuffle( indices2.begin(), indices2.end(), default_random_engine(10));//time(NULL)) );

  if( secondPass )
  {
    ProgressBar *fillingGaps = new ProgressBar(secondPass*imagePolygons.size()*indices2.size(), "Filling gaps");

    for( int i3 = 0; i3 < indices2.size(); ++i3 )
    {
      future< tuple< int, float, float, Vertex > > ret[threads];

      for( int k = 0; k < threads; ++k )
      {
        int i2 = indices2[i3]+k*indices2.size();
        if( i2 >= polygons.size() ) continue;

        ret[k] = async( launch::async, &generateSecondPassBestValues, i2, cellDistance, ref(imagePolygons), ref(polygons), ref(bestValues), ref(images2), ref(masks), ref(dimensions), c2, edgeData, inputWidth, inputHeight, angleOffset, skipNearby, sizePower, k==0, fillingGaps );
      }

      // Wait for threads to finish
      for( int k = 0; k < threads; ++k )
      {
        int i2 = indices2[i3]+k*indices2.size();
        if( i2 >= polygons.size() ) continue;

        bestValues[i2] = ret[k].get();
      }
    }

    fillingGaps->Finish();
  }

  outputWidth *= renderScale;
  outputHeight *= renderScale;


  ProgressBar *drawingImages = new ProgressBar(polygons.size(), "Rendering output");

  float maxScale = -1;

  for( int i1 = 0; i1 < polygons.size(); ++i1 )
  {
    int bestImage   = get<0>( bestValues[i1] );
    float scale    = get<1>( bestValues[i1] );
    float rotation = get<2>( bestValues[i1] );
    Vertex offset   = get<3>( bestValues[i1] );

    scale *= renderScale;

    maxScale = max(maxScale,scale);

    int offsetX = floor(renderScale*offset.x + 0.0001);
    int offsetY = floor(renderScale*offset.y + 0.0001);

    Polygon R = polygons[i1];

    vector< float > ink = {0,0,0};

    // Set the values for the output image
    int width = dimensions[bestImage].first;
    int height = dimensions[bestImage].second;

    float cosAngle = cos(rotation);
    float sinAngle = sin(rotation);
    float halfWidth = (float)width/2.0;
    float halfHeight = (float)height/2.0;

    int xOffset = max( rotateX(0,0,cosAngle,sinAngle,halfWidth,halfHeight), max( rotateX(width,0,cosAngle,sinAngle,halfWidth,halfHeight), max( rotateX(0,height,cosAngle,sinAngle,halfWidth,halfHeight), rotateX(width,height,cosAngle,sinAngle,halfWidth,halfHeight) ) ) ) - width;
    int yOffset = max( rotateY(0,0,cosAngle,sinAngle,halfWidth,halfHeight), max( rotateY(width,0,cosAngle,sinAngle,halfWidth,halfHeight), max( rotateY(0,height,cosAngle,sinAngle,halfWidth,halfHeight), rotateY(width,height,cosAngle,sinAngle,halfWidth,halfHeight) ) ) ) - height;

    int newWidth = width + 2*xOffset;
    int newHeight = height + 2*yOffset;

    cosAngle = cos(-rotation);
    sinAngle = sin(-rotation);
    halfWidth = (float)newWidth/2.0;
    halfHeight = (float)newHeight/2.0;

    float scaleOffset = 1.0 / scale;

    int ix, iy;

    iy = offsetY - scale*newHeight/2;

    // Render the image onto the output image
    for( float i = 0; i < newHeight; i += scaleOffset, ++iy )
    {
      ix = offsetX - scale*newWidth/2;

      for( float j = 0; j < newWidth; j += scaleOffset, ++ix )
      {
        int newX = rotateX(j,i,cosAngle,sinAngle,halfWidth,halfHeight) - xOffset;
        int newY = rotateY(j,i,cosAngle,sinAngle,halfWidth,halfHeight) - yOffset;

        // Make sure that pixel is inside image
        if( (ix < 0) || (ix > outputWidth - 1) || (iy < 0) || (iy > outputHeight - 1) ) continue;

        // Set index
        unsigned long long index = iy*outputWidth+ix;

        int r = 0, g = 0, b = 0, m = 0;

        int numUsed = 0;

        for( float ni = i; ni < i + scaleOffset && ni < newHeight; ++ni )
        {
          for( float nj = j; nj < j + scaleOffset && nj < newWidth ; ++nj )
          {
            int nx = rotateX(nj,ni,cosAngle,sinAngle,halfWidth,halfHeight) - xOffset;
            int ny = rotateY(nj,ni,cosAngle,sinAngle,halfWidth,halfHeight) - yOffset;

            if( (nx < 0) || (nx > width-1) || (ny < 0) || (ny > height-1) ) continue;

            ++numUsed;

            int p = width*ny + nx;

            r += images[bestImage][3*p+0];
            g += images[bestImage][3*p+1];
            b += images[bestImage][3*p+2];
            m += masks[bestImage][p];
          }
        }

        if( numUsed > 0 )
        {
          r /= numUsed;
          g /= numUsed;
          b /= numUsed;
          m /= numUsed;

          if( m < 100 ) continue;

          // Set output colors
          outputData[3*index+0] = r;
          outputData[3*index+1] = g;
          outputData[3*index+2] = b;
        }
      }
    }

    drawingImages->Increment();
  }

  drawingImages->Finish();

  cout << "Generating image " << outputImage << endl;

  VImage::new_from_memory( outputData, 3*outputWidth*outputHeight, outputWidth, outputHeight, 3, VIPS_FORMAT_UCHAR ).vipssave((char *)outputImage.c_str());
}