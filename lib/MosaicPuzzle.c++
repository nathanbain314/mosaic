#include "MosaicPuzzle.h"
#include "progress_bar.hpp"

#define JC_VORONOI_IMPLEMENTATION
#include "jc_voronoi.h"

Vertex compute2DPolygonCentroid( vector< Vertex > &vertices )
{
  Vertex centroid( 0, 0 );
  double signedArea = 0.0;

  for( int i = 0; i < vertices.size(); ++i )
  {
    double x0 = vertices[i].x;
    double y0 = vertices[i].y;
    double x1 = vertices[(i+1) % vertices.size()].x;
    double y1 = vertices[(i+1) % vertices.size()].y;
    double a = x0*y1 - x1*y0;
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

void generateVoronoiPolygons( vector< Polygon > &polygons, int count, int numrelaxations, int width, int height )
{
  jcv_point* points = 0;

  points = (jcv_point*)malloc( sizeof(jcv_point) * (size_t)count);

  srand(0);

  for( int i = 0; i < count; ++i )
  {
    points[i].x = (float)(rand()%width);
    points[i].y = (float)(rand()%height);
  }

  jcv_rect bounding_box = { { 0.0f, 0.0f }, { (float)width, (float)height } };
  jcv_diagram diagram;
  const jcv_site* sites;
  jcv_graphedge* graph_edge;

  for( int i = 0; i < numrelaxations; ++i )
  {
    jcv_diagram diagram;
    memset(&diagram, 0, sizeof(jcv_diagram));
    jcv_diagram_generate(count, (const jcv_point*)points, &bounding_box, &diagram);

    relax_points(&diagram, points);

    jcv_diagram_free( &diagram );
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

void generateImagePolygons( string imageDirectory, vector< string > &inputDirectory, vector< Polygon > &imagePolygons, vector< Polygon > &concavePolygons, vector< vector< unsigned char > > &images, vector< vector< unsigned char > > &masks, vector< pair< int, int > > &dimensions, double buildScale, double renderScale, bool recursiveSearch )
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
  unsigned char *imageData, *maskData;

  // Iterate through all images in directory
  for( int i = 0; i < num_images; ++i )
  {
    try
    {
      str = names[i];

      Polygon P, P2;

      /*
      concavePolygonFromAlphaImage( P2, names[i], 1 );

      decimate(P2,.01);

      removeExtrasP( P2, .01 );

      decimate(P2,.02);

      removeExtrasP( P2, .01 );

      P2.center();



      convexPolygonFromVertices( P2.vertices, P );
      */

      polygonFromAlphaImage( P, names[i], 1 );

      decimate(P,.01);

      P.makeClockwise();
      P.center();

      if( P.vertices.size() == 0 )
      {
        continue;
      }

      imagePolygons.push_back( P );
      
      concavePolygons.push_back( P2 );

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

      processing_images->Increment();
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

      pair< double, double > t = intersection( center, v1, v2, v3 );

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

tuple< int, double, double, Vertex > generateSecondPassBestValues( int i2, int cellDistance, vector< Polygon > &imagePolygons, vector< Polygon > &concavePolygons, vector< Polygon > &polygons, vector< tuple< int, double, double, Vertex > > &bestValues, vector< vector< unsigned char > > &images, vector< vector< unsigned char > > &masks, vector< pair< int, int > > &dimensions, unsigned char *inputData, int inputWidth, int inputHeight, double buildScale, double angleOffset, bool skipNearby, bool trueColor, double sizePower, bool doDraw, ProgressBar *drawingImages )
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

  if( skipNearby )
  {
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
  }

  for( int i11 = 0; i11 < outsidePolygons.size(); ++i11 )
  {
    int i1 = outsidePolygons[i11];

    int bestImage   = get<0>( bestValues[i1] );
    double scale    = get<1>( bestValues[i1] );
    double rotation = get<2>( bestValues[i1] );
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
  double bestScale = 0;
  double bestRotation = 0;
  Vertex bestOffset = Vertex( 0, 0 );
  double bestDifference = 1000000000;

  for( int k1 = 0, k = 0; k1 < imagePolygons.size(); ++k1, ++k )
  {    
    if( skipNearby && skipValues.size() < imagePolygons.size() && find( skipValues.begin(), skipValues.end(), k ) != skipValues.end() ) continue;

    Polygon P = imagePolygons[k1];

    double scale, rotation;

    Vertex offset;

    findBestFit( P, R, scale, rotation, offset, angleOffset, 0 );

    double angle = rotation * 3.14159265/180.0;

    int offsetX = floor(offset.x*buildScale+0.0001);
    int offsetY = floor(offset.y*buildScale+0.0001);

    int width = dimensions[k].first;
    int height = dimensions[k].second;

    double skipDifference = bestDifference * (width+1) * (height+1);

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

    double scaleOffset = 1.0 / (scale * buildScale);

    // Traverse data for rotated image to find difference from input image
    for( double iy = offsetY, i = 0, endy = newHeight; i < endy; i += scaleOffset, ++iy )
    {
      int ix = offsetX;
      double j = 0;
      int endx = newWidth;

      // Index of point in input image
      unsigned long long index = (unsigned long long)iy*(unsigned long long)inputWidth+(unsigned long long)ix;

      double icos = i * cosAngle;
      double isin = i * sinAngle;

      for( ; j < endx; j += scaleOffset, ++index, ++ix )
      {
        if( (ix < 0) || (ix > inputWidth - 1) || (iy < 0) || (iy > inputHeight - 1) ) continue;

        int r = 0, g = 0, b = 0, m = 0;

        int numUsed = 0;

        for( double ni = i; ni < i + scaleOffset && ni < endy; ++ni )
        {
          for( double nj = j; nj < j + scaleOffset && nj < endx ; ++nj )
          {
            int nx = rotateX(nj,ni,cosAngle,sinAngle,halfWidth,halfHeight) - xOffset;// + halfWidth;
            int ny = rotateY(nj,ni,cosAngle,sinAngle,halfWidth,halfHeight) - yOffset;// + halfHeight;

            if( (nx < 0) || (nx > width-1) || (ny < 0) || (ny > height-1) ) continue;

            int p = width*ny + nx;

            if( masks[k][p] == 0 ) continue;

            ++numUsed;

            r += images[k][3*p+0];
            g += images[k][3*p+1];
            b += images[k][3*p+2];
            m += masks[k][p];
          }
        }

        if( numUsed > 0 )
        {
          r /= numUsed;
          g /= numUsed;
          b /= numUsed;
          m /= numUsed;

          if( m < 100 ) continue;

          ++usedPixels;

          if( trueColor )
          {
            float l1,a1,b1,l2,a2,b2;
            // Convert rgb to rgb16
            vips_col_sRGB2scRGB_8(inputData[3ULL*index+0ULL],inputData[3ULL*index+1ULL],inputData[3ULL*index+2ULL], &l1,&a1,&b1 );
            vips_col_sRGB2scRGB_8(r,g,b, &l2,&a2,&b2 );
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
            int ir = inputData[3ULL*index+0ULL] - r;
            int ig = inputData[3ULL*index+1ULL] - g;
            int ib = inputData[3ULL*index+2ULL] - b;

            difference += sqrt( ir*ir + ig*ig + ib*ib );
          }
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

void generateBestValues( int start, int end, vector< int > &indices, vector< Polygon > &polygons, vector< Polygon > &imagePolygons, vector< tuple< int, double, double, Vertex > > &bestValues, vector< vector< unsigned char > > &images, vector< vector< unsigned char > > &masks, vector< pair< int, int > > &dimensions, unsigned char *inputData, int inputWidth, int inputHeight, double buildScale, double angleOffset, bool trueColor, double sizePower, double cellDistance, bool skipNearby, ProgressBar *findingBestImages )
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
    double bestScale = 0;
    double bestRotation = 0;
    Vertex bestOffset = Vertex( 0, 0 );
    double bestDifference = 1000000000;

    for( int k1 = 0, k = 0; k1 < imagePolygons.size(); ++k1, ++k )
    {
      if( skipNearby && skipValues.size() < imagePolygons.size() && find( skipValues.begin(), skipValues.end(), k ) != skipValues.end() ) continue;

      Polygon P = imagePolygons[k];

      double scale, rotation;

      Vertex offset;

      findBestFit( P, R, scale, rotation, offset, angleOffset, 0 );

      double angle = rotation * 3.14159265/180.0;

      int offsetX = floor(offset.x*buildScale+0.0001);
      int offsetY = floor(offset.y*buildScale+0.0001);

      int width = dimensions[k].first;
      int height = dimensions[k].second;

      double skipDifference = bestDifference * (width+1) * (height+1);

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

      double scaleOffset = 1.0 / (scale * buildScale);

      // Traverse data for rotated image to find difference from input image
      for( double iy = offsetY, i = 0, endy = newHeight; i < endy; i += scaleOffset, ++iy )
      {
        int ix = offsetX;
        double j = 0;
        int endx = newWidth;

        // Index of point in input image
        unsigned long long index = (unsigned long long)iy*(unsigned long long)inputWidth+(unsigned long long)ix;

        double icos = i * cosAngle;
        double isin = i * sinAngle;

        for( ; j < endx; j += scaleOffset, ++index, ++ix )
        {
          if( (ix < 0) || (ix > inputWidth - 1) || (iy < 0) || (iy > inputHeight - 1) ) continue;

          int r = 0, g = 0, b = 0, m = 0;

          int numUsed = 0;

          for( double ni = i; ni < i + scaleOffset && ni < endy; ++ni )
          {
            for( double nj = j; nj < j + scaleOffset && nj < endx ; ++nj )
            {
              int nx = rotateX(nj,ni,cosAngle,sinAngle,halfWidth,halfHeight) - xOffset;// + halfWidth;
              int ny = rotateY(nj,ni,cosAngle,sinAngle,halfWidth,halfHeight) - yOffset;// + halfHeight;

              if( (nx < 0) || (nx > width-1) || (ny < 0) || (ny > height-1) ) continue;

              int p = width*ny + nx;

              if( masks[k][p] < 100 ) continue;

              ++numUsed;

              r += images[k][3*p+0];
              g += images[k][3*p+1];
              b += images[k][3*p+2];
              m += masks[k][p];
//                break;
            }
//              break;
          }

          if( numUsed > 0 )
          {
            r /= numUsed;
            g /= numUsed;
            b /= numUsed;
            m /= numUsed;

            if( m < 100 ) continue;

            ++usedPixels;

            int ir = inputData[3ULL*index+0ULL] - r;//images[k][3*p+0];
            int ig = inputData[3ULL*index+1ULL] - g;//images[k][3*p+1];
            int ib = inputData[3ULL*index+2ULL] - b;//images[k][3*p+2];

            if( trueColor )
            {
              float l1,a1,b1,l2,a2,b2;
              // Convert rgb to rgb16
              vips_col_sRGB2scRGB_8(inputData[3ULL*index+0ULL],inputData[3ULL*index+1ULL],inputData[3ULL*index+2ULL], &l1,&a1,&b1 );
              vips_col_sRGB2scRGB_8(r,g,b, &l2,&a2,&b2 );
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
              int ir = inputData[3ULL*index+0ULL] - r;
              int ig = inputData[3ULL*index+1ULL] - g;
              int ib = inputData[3ULL*index+2ULL] - b;

              difference += sqrt( ir*ir + ig*ig + ib*ib );
            }
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

void RunMosaicPuzzle( string inputName, string outputImage, vector< string > inputDirectory, int count, bool secondPass, double renderScale, double buildScale, double angleOffset, bool trueColor, bool skipNearby, double sizePower )
{
  vector< Polygon > polygons, imagePolygons, concavePolygons;
  vector< vector< unsigned char > > images, masks;
  vector< pair< int, int > > dimensions;

  for( int i = 0; i < inputDirectory.size(); ++i )
  {
    string imageDirectory = inputDirectory[i];
    if( imageDirectory.back() != '/' ) imageDirectory += '/';
    generateImagePolygons( imageDirectory, inputDirectory, imagePolygons, concavePolygons, images, masks, dimensions, buildScale, renderScale, false );
  }

  VImage inputImage = VImage::vipsload((char *)inputName.c_str()).autorot().resize(buildScale);
  unsigned char *inputData  = (unsigned char *)inputImage.data();
  unsigned char *grayscaleData = (unsigned char *)inputImage.colourspace(VIPS_INTERPRETATION_B_W).data();

  int inputWidth = inputImage.width(), inputHeight = inputImage.height();

  int outputWidth = inputWidth;
  int outputHeight = inputHeight;

  generateVoronoiPolygons( polygons, count, 100, outputWidth, outputHeight );


  double cellrange[4] = {1000000000,1000000000,-1000000000,-1000000000};

  int mi = polygons.size()/2;

  for( int i = 0; i < polygons[mi].vertices.size(); ++i )
  {
    cellrange[0] = min(cellrange[0],polygons[mi].vertices[i].x);
    cellrange[1] = min(cellrange[1],polygons[mi].vertices[i].y);
    cellrange[2] = max(cellrange[2],polygons[mi].vertices[i].x);
    cellrange[3] = max(cellrange[3],polygons[mi].vertices[i].y);
  }

  double cellDistance = 4*(cellrange[3]-cellrange[1])*(cellrange[3]-cellrange[1]) + (cellrange[2]-cellrange[0])*(cellrange[2]-cellrange[0]);


  vector< tuple< int, double, double, Vertex > > bestValues(polygons.size(), make_tuple(-1,0,0,Vertex(0,0)) );

  int threads = sysconf(_SC_NPROCESSORS_ONLN);

  ProgressBar *findingBestImages = new ProgressBar(polygons.size()*imagePolygons.size()/threads, "Finding best images");

  vector< int > indices( polygons.size() );
  iota( indices.begin(), indices.end(), 0 );

  // Shuffle the points so that patterens do not form
  shuffle( indices.begin(), indices.end(), default_random_engine(0));//time(NULL)) );

  future< void > ret[threads];

  for( int k = 0; k < threads; ++k )
  {
    ret[k] = async( launch::async, &generateBestValues, k*indices.size()/threads, (k+1)*indices.size()/threads, ref(indices), ref(polygons), ref(imagePolygons), ref(bestValues), ref(images), ref(masks), ref(dimensions), inputData, inputWidth, inputHeight, buildScale, angleOffset, trueColor, sizePower, cellDistance, skipNearby, findingBestImages );
  }

  // Wait for threads to finish
  for( int k = 0; k < threads; ++k )
  {
    ret[k].get();
  }

  findingBestImages->Finish();



  unsigned char *outputData = ( unsigned char * )calloc(3*(outputWidth*renderScale)*(outputHeight*renderScale),sizeof(unsigned char));

  vector< int > indices2( ceil((double)polygons.size()/threads) );

  iota( indices2.begin(), indices2.end(), 0 );

  // Shuffle the points so that patterns do not form
  shuffle( indices2.begin(), indices2.end(), default_random_engine(10));//time(NULL)) );

  if( secondPass )
  {
    ProgressBar *fillingGaps = new ProgressBar(secondPass*imagePolygons.size()*indices2.size(), "Filling gaps");

    for( int i3 = 0; i3 < indices2.size(); ++i3 )
    {
      future< tuple< int, double, double, Vertex > > ret[threads];

      for( int k = 0; k < threads; ++k )
      {
        int i2 = indices2[i3]+k*indices2.size();
        if( i2 >= polygons.size() ) continue;

        ret[k] = async( launch::async, &generateSecondPassBestValues, i2, cellDistance, ref(imagePolygons), ref(concavePolygons), ref(polygons), ref(bestValues), ref(images), ref(masks), ref(dimensions), inputData, inputWidth, inputHeight, buildScale, angleOffset, skipNearby, trueColor, sizePower, k==0, fillingGaps );
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

  double maxScale = -1;

  for( int i1 = 0; i1 < polygons.size(); ++i1 )
  {
    int bestImage   = get<0>( bestValues[i1] );
    double scale    = get<1>( bestValues[i1] );
    double rotation = get<2>( bestValues[i1] );
    Vertex offset   = get<3>( bestValues[i1] );

    scale *= renderScale;

    maxScale = max(maxScale,scale);

    int offsetX = floor(renderScale*offset.x + 0.0001);
    int offsetY = floor(renderScale*offset.y + 0.0001);

    Polygon R = polygons[i1];

    vector< double > ink = {0,0,0};

    // Set the values for the output image
    int width = dimensions[bestImage].first;
    int height = dimensions[bestImage].second;

    double cosAngle = cos(rotation);
    double sinAngle = sin(rotation);
    double halfWidth = (double)width/2.0;
    double halfHeight = (double)height/2.0;

    int xOffset = max( rotateX(0,0,cosAngle,sinAngle,halfWidth,halfHeight), max( rotateX(width,0,cosAngle,sinAngle,halfWidth,halfHeight), max( rotateX(0,height,cosAngle,sinAngle,halfWidth,halfHeight), rotateX(width,height,cosAngle,sinAngle,halfWidth,halfHeight) ) ) ) - width;
    int yOffset = max( rotateY(0,0,cosAngle,sinAngle,halfWidth,halfHeight), max( rotateY(width,0,cosAngle,sinAngle,halfWidth,halfHeight), max( rotateY(0,height,cosAngle,sinAngle,halfWidth,halfHeight), rotateY(width,height,cosAngle,sinAngle,halfWidth,halfHeight) ) ) ) - height;

    int newWidth = width + 2*xOffset;
    int newHeight = height + 2*yOffset;

    cosAngle = cos(-rotation);
    sinAngle = sin(-rotation);
    halfWidth = (double)newWidth/2.0;
    halfHeight = (double)newHeight/2.0;

    double scaleOffset = 1.0 / scale;

    int ix, iy;

    iy = offsetY - scale*newHeight/2;

    // Render the image onto the output image
    for( double i = 0; i < newHeight; i += scaleOffset, ++iy )
    {
      ix = offsetX - scale*newWidth/2;

      for( double j = 0; j < newWidth; j += scaleOffset, ++ix )
      {
        int newX = rotateX(j,i,cosAngle,sinAngle,halfWidth,halfHeight) - xOffset;
        int newY = rotateY(j,i,cosAngle,sinAngle,halfWidth,halfHeight) - yOffset;

        // Make sure that pixel is inside image
        if( (ix < 0) || (ix > outputWidth - 1) || (iy < 0) || (iy > outputHeight - 1) ) continue;

        // Set index
        unsigned long long index = iy*outputWidth+ix;

        int r = 0, g = 0, b = 0, m = 0;

        int numUsed = 0;

        for( double ni = i; ni < i + scaleOffset && ni < newHeight; ++ni )
        {
          for( double nj = j; nj < j + scaleOffset && nj < newWidth ; ++nj )
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