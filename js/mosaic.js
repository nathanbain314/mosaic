viewer = OpenSeadragon({
  id:            "mosaic",
  prefixUrl:     "icons/",
  maxZoomPixelRatio: 1,
  alwaysBlend: true,
  tileSources:   {
      height: mosaicHeight<<(maxZoomablesLevel+8),
      width:  mosaicWidth<<(maxZoomablesLevel+8),
      tileSize: 256,
      minLevel: 0, 
      maxLevel: mosaicLevel+maxZoomablesLevel+0,
      tileOverlap: 0,
      getTileUrl: function( level, x, y ){
        if( level < mosaicLevel )
        {
          return outputName + level + "/" + data[level][y][x] + ".jpeg";
        }
        else
        {
          var superLevel = (level-mosaicLevel);
          var shrink = 1<<superLevel;
          var m = data[mosaicLevel][y>>superLevel][x>>superLevel];
          return outputDirectory + m + "_files/" + superLevel + "/" + (x%shrink) + "_" + (y%shrink) + ".jpeg";
        }
      },
      tileExists: function( level, x, y ){
        if( level < mosaicLevel )
        {
          return true;
        }
        else
        {
          var superLevel = (level-mosaicLevel);
          var m = data[mosaicLevel][y>>superLevel][x>>superLevel];
          if( levelData[m] >= superLevel )
          {
            return true;
          }
        }
        return false;
      }
  }
});

viewer.viewport.getMaxZoom = function() { 
  var minzoom = (mosaicWidth<<(minZoomablesLevel+8))/document.getElementById("mosaic").offsetWidth;

  if( viewer.viewport.getZoom() >= minzoom )
  {
    var maxLevel = minZoomablesLevel;
    var bounds = viewer.viewport.getBounds(true);
    var startX = bounds.x < 0 ? 0 : Math.floor(bounds.x * mosaicWidth);
    var startY = bounds.y < 0 ? 0 : Math.floor(bounds.y * mosaicWidth);
    var endX = bounds.x+bounds.width > 1 ? mosaicWidth : Math.ceil((bounds.x+bounds.width) * mosaicWidth);
    var endY = bounds.y+bounds.height > 1 ? mosaicHeight : Math.ceil((bounds.y+bounds.height) * mosaicWidth);

    for( ; startY < endY; ++startY )
    {
      for( ; startX < endX; ++startX )
      {
        maxLevel = Math.max( maxLevel, levelData[data[mosaicLevel][startY][startX]] );
      }
    }

    return minzoom<<( maxLevel - minZoomablesLevel );
  }
  else
  {
    return minzoom;
  }
}