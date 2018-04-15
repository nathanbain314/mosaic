var modulo = [0,1,3,7,15,31,63,127,255,511,1023,2047,4095,8191];
var zoomed = false;
var done = true;
var zoomSpeed = 1.0;

viewer = OpenSeadragon({
  id:            "continuous",
  prefixUrl:     "icons/",
      maxZoomPixelRatio: 1,
      alwaysBlend: true,
      mouseNavEnabled: false,
      //zoomPerClick: 1.2,
      //zoomPerScroll: 1.1,
      showZoomControl: false,
      showHomeControl: false,
  tileSources:   {
      height: 1048576,
      width:  1048576,
      tileSize: 256,
      minLevel: 0, 
      maxLevel: 11,
      tileOverlap: 0,
      getTileUrl: function( level, x, y ){
        var p = modulo[level];
        var h = x >> level;
        var v = y >> level;
        y = y&p;
        x = x&p;

        if( level < 6 )
        {
          return outputDirectory + level + "/" + data[level][topLevel[v][h]][y][x] + ".jpeg";
        }
        else
        {
          var superLevel = level - 6;
          p = modulo[superLevel];
          var m = data[6][topLevel[v][h]][y>>superLevel][x>>superLevel];
          return outputDirectory + superLevel + "/" + data[superLevel][m][y&p][x&p] + ".jpeg";
        }
      }
  }
});

viewer.addHandler('open', function() 
{
  viewportPoint = viewer.viewport.getCenter();
  var bounds = viewer.viewport.getBounds(true); 
  bounds.x = 0;
  bounds.y = 0;
  bounds.width = 0.5;
  bounds.height = 0.5;
  viewer.viewport.fitBounds(bounds,true);

  var tracker = new OpenSeadragon.MouseTracker({
      element: viewer.container,
      moveHandler: function(event) {
          var webPoint = event.position;
          viewportPoint = viewer.viewport.pointFromPixel(webPoint);  
      }
  });  

  tracker.setTracking(true);
});

var zoomIn = function () {
  if( !zoomed && done && viewer.viewport.getZoom(false) > viewer.viewport.getMaxZoom()*3/4 )
  {
    zoomed = true;
    done = false;

    var oldImage = viewer.world.getItemAt(0);
    var oldBounds = oldImage.getBounds();
    var oldSource = oldImage.source;

    viewer.addTiledImage({
        tileSource: oldSource,
        x: oldBounds.x,
        y: oldBounds.y,
        width: oldBounds.width
    });
  }
  if( zoomed && viewer.viewport.getZoom(false) > viewer.viewport.getMaxZoom()*7/8 )
  {
    zoomed = false;
    var size = 128;
    var bounds = viewer.viewport.getBounds(true);
    var bounds2 = viewer.viewport.getBounds(false);
    var startX = Math.max( Math.floor(bounds.x * size), 0 );
    var startY = Math.max( Math.floor(bounds.y * size), 0 );

    topLevel = [[data[6][topLevel[startY>63?1:0][startX>63?1:0]][startY&63][startX&63],
                data[6][topLevel[startY>63?1:0][startX>62?1:0]][startY&63][(startX+1)&63]],
                [data[6][topLevel[startY>62?1:0][startX>63?1:0]][(startY+1)&63][startX&63],
                data[6][topLevel[startY>62?1:0][startX>62?1:0]][(startY+1)&63][(startX+1)&63]]];

    var oldImage = viewer.world.getItemAt(0);

    viewportPoint.x = (viewportPoint.x * size - startX)/2;
    viewportPoint.y = (viewportPoint.y * size - startY)/2;

    bounds.x = (bounds.x*size - startX)/2;
    bounds.y = (bounds.y*size - startY)/2;
    bounds.width = bounds.width*size/2;
    bounds.height = bounds.height*size/2;

    bounds2.x = (bounds2.x*size - startX)/2;
    bounds2.y = (bounds2.y*size - startY)/2;
    bounds2.width = bounds2.width*size/2;
    bounds2.height = bounds2.height*size/2;

    viewer.world.removeItem(oldImage);

    viewer.viewport.fitBounds(bounds,true).fitBounds(bounds2,false);
  }
  if( !done && viewer.viewport.getZoom( false ) < 50 )
  {
    done = true;
  }

  if( zoomSpeed > 1.0 ) viewer.viewport.zoomBy(zoomSpeed, viewportPoint, false);
}

var zoomer = setInterval(zoomIn, 60);

$("#slider").slider({
                min: 1.0,
                max: 1.05,
                step: 0.0001,
                value: 1.0,
                slide: function(event, ui) {
                    zoomSpeed = ui.value;
                }
            });
