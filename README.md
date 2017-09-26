#mosaic
This is a tool for building photomosaic images, high resolution zoomable photomosaics, and infinite continuous photomosaics.

[Libvips](http://www.vips.ecs.soton.ac.uk/index.php?title=Libvips) has to be installed in order to use this program.
There are two executables that will be created: RunMosaic and RunContinuous.

#RunMosaic
This creates a photomosaic and saves it as either a single image or a zoomable image.
##options
- -p,  --picture : Image to use as reference for the photomosaic to be based on.
- -i,  --image : Directory of images that will be used in the creation of the photomosaic.
- -o,  --output : Name of photomosaic to be created. If the name ends in an image format ( e.g., .png, .jpg ) then the mosaic will be saved as a single image. Otherwise the image tiles will be placed in a directory with the output name and an html file with the output name will be generated.
- -n, --numHorizontal : Number of tiles horizontally
- -r, --repeat : Closest distance to repeat image. This is the closest distance that the same image can be used in the mosaic
- -c, --color : Use [de00](https://en.wikipedia.org/wiki/Color_difference#CIEDE2000) for color difference
- -s, --square : Generate square mosaic
- -t, --thumbnailSize : Maximum thumbnail size

##Tips
###Single images
A mosaic saved as single image will be built out of the individual tiles. If thumbnailSize is not specified then the the mosaic will be approximately the same size as the reference image, otherwise it will be (thumbnailSize*numHorizontal)
###Zoomable images
A mosaic saved as a zoomable image will generate [deep zoom images](https://msdn.microsoft.com/fr-fr/library/cc645077%28v=vs.95%29.aspx) of each photo in the mosaic and combine and shrink these to build the rest of the tiles for each level. It will then create an html file that uses [openseadragon](http://openseadragon.github.io) to display the mosaic as a custom tile source. The program will save each unique tile only once and reuse it in order to save space with no duplicated tiles. 
###Mosaic creation
The photomosaic will look better with a large and diverse set of input images. If there are more tiles in the mosaic then it will look closer to the original, but if there are fewer it might look more impressive. You may get the error "Too many open files" which can be solved by using **ulimit -n** *int*. Setting the ulimit to slightly more than the number of images in the input directory should fix the problem.
####Sum of squares
The default color difference algorithm is the sum of squares of the differences of all of the pixels red, blue, and green values. An image is represented as a point in a high dimensional space where every red, green, and blue value is a different dimension. [Nanoflann](https://github.com/jlblancoc/nanoflann) is then used to compute the nearest neighbor which is the most similar image.
####de00
The sum of squares difference is much faster and is also usually better at getting closer shapes in an image. However, the colors can be off and it can create poor results in some circumstances. The program can use the [de00](https://en.wikipedia.org/wiki/Color_difference#CIEDE2000) algorithm to compute the sum of the differences for each image and choose the most similar image.
####Thumbnail size
The default thumbnail size is the width of the image divided by the number of tiles horizontally. If you set the max thumbnail size to be less than this then the image and tiles will shrunk, which will increase the speed and the tiles will be more of the average color, which can sometimes create better images. 
####Examples
![William](http://nathanbain.com/cgit/mosaic.git/plain/examples/william.png)
Sum of squares: Each image has twice as many horizontal images
Even at low levels it still resembles the original image

![All](http://nathanbain.com/cgit/mosaic.git/plain/examples/all.png)
Sum of squares: thumbnailsSize = 1 (SS1) and default (SSD); de00: thumbnailsSize = 1 (DE1) and default (DED)
Rita Vrataski is most visible with the SSD but looks like it has a color filter, while the colors at DED are closer to the original but only has some shape characteristics of the original. Since the lizard has few tiles SSD gets the colors very wrong but SS1 looks much better. The Penrose triangle doesn't even show up in SSD because it is all brown, and DED doesn't differentiate between brown and dark red. 

#RunContinuous
This creates 64 by 64 image photomosaics made up more photomosaics and utilizes [openseadragon](http://openseadragon.github.io) to create an infinite zoom through endless mosaics.
##options
- -i,  --image : Directory of images that will be used in the creation of the photomosaic.
- -o,  --output : Name of continuous mosaic to be created.
- -r, --repeat : Closest distance to repeat image. This is the closest distance that the same image can be used in the mosaic
- -c, --color : Use [de00](https://en.wikipedia.org/wiki/Color_difference#CIEDE2000) for color difference
- -t, --thumbnailSize : Maximum thumbnail size

##Continuous HTML File
The HTML file uses [openseadragon](http://openseadragon.github.io) to create a custom tile source to display the mosaic. The same image is only saved once in order to increase speed and decrease storage space. There is a slider on the webpage that controls the zoom speed. 

#Examples
[This webpage](http://nathanbain.com/mosaic/) contains several photomosaics and one continuous mosaic.