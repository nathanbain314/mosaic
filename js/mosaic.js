viewer = OpenSeadragon({
  id:            "mosaic",
  prefixUrl:     "icons/",
  maxZoomPixelRatio: 1,
  alwaysBlend: true,
  tileSources:   {
      height: mosaicHeight*mosaicTileHeight<<maxZoomablesLevel,
      width:  mosaicWidth*mosaicTileWidth<<maxZoomablesLevel,
      tileWidth: mosaicTileWidth,
      tileHeight: mosaicTileHeight,
      minLevel: 0, 
      maxLevel: mosaicLevel+maxZoomablesLevel,
      tileOverlap: 0,
      getTileUrl: function( level, x, y ){
        if( level < mosaicLevel )
        {
          return outputName + level + "/" + data[level][y][x] + ".jpeg";
        }
        else
        {
          var superLevel = (level-mosaicLevel);
          var shrink = (1<<superLevel) - 1;
          return outputDirectory + data[mosaicLevel][y>>superLevel][x>>superLevel] + "_files/" + superLevel + "/" + (x&shrink) + "_" + (y&shrink) + ".jpeg";
        }
      },
      tileExists: function( level, x, y ){
        var superLevel = (level-mosaicLevel);

        return superLevel < 0 || levelData[data[mosaicLevel][y>>superLevel][x>>superLevel]] >= superLevel;
      }
  }
});

var minWidth = mosaicWidth*mosaicTileWidth<<minZoomablesLevel;
var mosaicWidthHeight = mosaicWidth * mosaicTileWidth/mosaicTileHeight;

viewer.viewport.getMaxZoom = function() { 
  var minzoom = minWidth/document.getElementById("mosaic").offsetWidth;

  if( viewer.viewport.getZoom() >= minzoom )
  {
    var maxLevel = minZoomablesLevel;
    var bounds = viewer.viewport.getBounds(true);

    var endX = bounds.x+bounds.width > 1 ? mosaicWidth : Math.ceil((bounds.x+bounds.width) * mosaicWidth);
    var endY = bounds.y+bounds.height > 1 ? mosaicHeight : Math.ceil((bounds.y+bounds.height) * mosaicWidthHeight);

    var startXValue = bounds.x < 0 ? 0 : Math.floor(bounds.x * mosaicWidth);

    for( var startY = bounds.y < 0 ? 0 : Math.floor(bounds.y * mosaicWidth * mosaicTileWidth/mosaicTileHeight); startY < endY; ++startY )
    {
      for( var startX = startXValue; startX < endX; ++startX )
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