# mosaic
This is a tool for building photomosaic images, high resolution zoomable photomosaics, and infinite continuous photomosaics.

# Building
## Unix
Install [libvips](https://github.com/jcupitt/libvips) and [cmake](https://cmake.org).
Then you can compile the program.  
```
  cmake .  
  make. 
```
## Windows
Install [mingw-6w4](http://mingw-w64.org/doku.php/download/mingw-builds). At the screen with the various options for installing, choose **x86_64** as the architecture.
Install [cmake](https://cmake.org/download/) , and at the screen where gives choices adding it to the path, make sure to add it to the system path.
Download the libvips w64 binaries from the vips [download page](https://github.com/jcupitt/libvips/releases) and extract it.
Open up a MinGW-W64 terminal. 
Set an environment variable named **VIPS_INCLUDE** to be the location of the libvips folder and add the binary folder to the system path. 
```
  set VIPS_INCLUDE=C:\Users\nathanbain314\Downloads\vips-dev-w64-all-8.6.3\vips-dev-8.6. 
  set PATH=%PATH%;C:\Users\nathanbain314\Downloads\vips-dev-w64-all-8.6.3\vips-dev-8.6\bin. 
```
Then compile the progam. 
```
  cmake . -G "MinGW Makefiles".  
  mingw32-make
```
# RunMosaic
This creates a photomosaic and saves it as either a single image or a zoomable image.
## options
- -p,  --picture : Image to use as reference for the photomosaic to be based on. A directory can also be used, in which case every image in that directory will be turned into a mosaic and saved in the output directory.
- -d,  --directory : Directory of images that will be used in the creation of the photomosaic. Use this option multiple times for multiple directories.
- -o,  --output : Name of photomosaic to be created. If the name ends in an image format ( e.g., .png, .jpg ) then the mosaic will be saved as a single image. Otherwise the image tiles will be placed in a directory with the output name and an html file with the output name will be generated.
- -n, --numHorizontal : Number of tiles horizontally
- -r, --repeat : Closest distance to repeat image. This is the closest distance that the same image can be used in the mosaic
- -c, --cropStyle : What cropStyle to use. 0 = center, 1 = entropy, 2 = attention, 3 = all possible crops
- -t, --trueColor : Use [de00](https://en.wikipedia.org/wiki/Color_difference# CIEDE2000) for color difference
- -s, --spin : Rotate the images to create four times as many images
- -m, --mosaicTileSize : Maximum tile size for generating mosaic
- -i, --imageTileSize : Tile size for generating image
- -f, --file : File of image data to save or load
## Tips
### Single images
A mosaic saved as single image will be built out of the individual tiles. The imageTileSize will default to mosaicTileSize, and changing this is useful for when the output image is too large or too small.
### Zoomable images
A mosaic saved as a zoomable image will generate [deep zoom images](https://msdn.microsoft.com/fr-fr/library/cc645077%28v=vs.95%29.aspx) of each photo in the mosaic and combine and shrink these to build the rest of the tiles for each level. It will then create an html file that uses [openseadragon](http://openseadragon.github.io) to display the mosaic as a custom tile source. The program will save each unique tile only once and reuse it in order to save space with no duplicated tiles. 
#### Crop Styles
The tiles in a mosaic are all square, so if the input image is not square then it will have to be cropped down to a square. There are 4 styles for cropping images:     
&nbsp;&nbsp;&nbsp;&nbsp;0. (default) Center: crops the center square of each input image     
&nbsp;&nbsp;&nbsp;&nbsp;1. Entropy: crops a square based on how busy it is     
&nbsp;&nbsp;&nbsp;&nbsp;2. Attention: uses vipsinteresting attention functionality to crop a square likely to draw human attention     
&nbsp;&nbsp;&nbsp;&nbsp;3. All: Crops every possible square from an image     
### Mosaic creation
The photomosaic will look better with a large and diverse set of input images. If there are more tiles in the mosaic then it will look closer to the original, but if there are fewer it might look more impressive. You may get the error "Too many open files" which can be solved by using **ulimit -n** *int*. Setting the ulimit to slightly more than the number of images in the input directory should fix the problem.
#### Sum of squares
The default color difference algorithm is the sum of squares of the differences of all of the pixels red, blue, and green values. An image is represented as a point in a high dimensional space where every red, green, and blue value is a different dimension. [Nanoflann](https://github.com/jlblancoc/nanoflann) is then used to compute the nearest neighbor which is the most similar image.
#### de00
The sum of squares difference is much faster and is also usually better at getting closer shapes in an image. However, the colors can be off and it can create poor results in some circumstances. The program can use the [de00](https://en.wikipedia.org/wiki/Color_difference# CIEDE2000) algorithm to compute the sum of the differences for each image and choose the most similar image.
#### MosaicTileSize
MosaicTileSize will default to width/numHorizontal. Using a smaller mosaicTileSize will take less time and will sometimes look better for mosaics with fewer tiles or for when you want the average color of each image.
#### Examples
##### Difference in number of tiles
![William](https://raw.githubusercontent.com/nathanbain314/mosaic/master/examples/william.png)
Sum of squares: Each image has twice as many horizontal images
Even at low levels it still resembles the original image

##### Difference in mosaic generations
| Color comparison = | Sum of squares | Sum of squares | de00 | de00 |
| --- | --- | --- | --- | --- |
| mosaicTileSize = | 1 | default | 1 | default | 
| ![Rita](https://raw.githubusercontent.com/nathanbain314/mosaic/master/examples/rita/0.png) | ![Rita](https://raw.githubusercontent.com/nathanbain314/mosaic/master/examples/rita/1.png) | ![Rita](https://raw.githubusercontent.com/nathanbain314/mosaic/master/examples/rita/2.png) | ![Rita](https://raw.githubusercontent.com/nathanbain314/mosaic/master/examples/rita/3.png) | ![Rita](https://raw.githubusercontent.com/nathanbain314/mosaic/master/examples/rita/4.png) |
| ![Lizard](https://raw.githubusercontent.com/nathanbain314/mosaic/master/examples/lizard/0.png) | ![Lizard](https://raw.githubusercontent.com/nathanbain314/mosaic/master/examples/lizard/1.png) | ![Lizard](https://raw.githubusercontent.com/nathanbain314/mosaic/master/examples/lizard/2.png) | ![Lizard](https://raw.githubusercontent.com/nathanbain314/mosaic/master/examples/lizard/3.png) | ![Lizard](https://raw.githubusercontent.com/nathanbain314/mosaic/master/examples/lizard/4.png) |
| ![Penrose](https://raw.githubusercontent.com/nathanbain314/mosaic/master/examples/penrose/0.png) | ![Penrose](https://raw.githubusercontent.com/nathanbain314/mosaic/master/examples/penrose/1.png) | ![Penrose](https://raw.githubusercontent.com/nathanbain314/mosaic/master/examples/penrose/2.png) | ![Penrose](https://raw.githubusercontent.com/nathanbain314/mosaic/master/examples/penrose/3.png) | ![Penrose](https://raw.githubusercontent.com/nathanbain314/mosaic/master/examples/penrose/4.png) |

Rita is most visible with the sum of squares and default mosaicTileSize but looks like it has a color filter while the colors with de00 are closer to the original but lacks some of the shape characteristics. The lizard gets some brown spots that aren't in the original image, but by setting the mosaicTileSize to one removes them. The Penrose triangle doesn't even appear with the sum of squares algorithm and default mosaicTileSize, so the other options must be used. 

##### Pixelated mosaics
| Lego | Lines | Circles |
| --- | --- | --- |
| ![Mona](https://raw.githubusercontent.com/nathanbain314/mosaic/master/examples/monaLego.png) | ![Mona](https://raw.githubusercontent.com/nathanbain314/mosaic/master/examples/monaLines.png) | ![Mona](https://raw.githubusercontent.com/nathanbain314/mosaic/master/examples/monaCircles.png) |

Setting the mosaicTileSize to 1 can help with odd image inputs that benefit for a more pixelated mosaic.

##### Directory of reference images
![Buck](https://raw.githubusercontent.com/nathanbain314/mosaic/master/examples/buck.gif)
The program can process an entire directory of images all the same size if that directory is passed as the reference image. The input images will only be loaded and processed once instead for each image in order to save time. The results will be saved in the output directory with the same name as the original image.
# RunContinuous
This creates 64 by 64 image photomosaics made up more photomosaics and utilizes [openseadragon](http://openseadragon.github.io) to create an infinite zoom through endless mosaics.
## options
- -i,  --image : Directory of images that will be used in the creation of the photomosaic.
- -o,  --output : Name of continuous mosaic to be created.
- -r, --repeat : Closest distance to repeat image. This is the closest distance that the same image can be used in the mosaic
- -t, --trueColor : Use [de00](https://en.wikipedia.org/wiki/Color_difference# CIEDE2000) for color difference
- -m, --mosaicTileSize : Maximum tile size for generating mosaic

## Continuous HTML File
The HTML file uses [openseadragon](http://openseadragon.github.io) to create a custom tile source to display the mosaic. The same image is only saved once in order to increase speed and decrease storage space. There is a slider on the webpage that controls the zoom speed. 

# Examples
[This webpage](http://nathanbain.com/mosaic/) contains several photomosaics and one continuous mosaic.
