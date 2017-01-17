#mosaic
This is a tool for building photomosaic images, high resolution zoomable photomosaics, and infinite continuous photomosaics.

[Libvips](http://www.vips.ecs.soton.ac.uk/index.php?title=Libvips) is required in order for this to build. Building is achieved by creating the build the build directory and then running **make**.

There are two executables that will be created: RunMosaic and RunContinuous.

#RunMosaic
This creates a photomosaic and saves it as either a single image or a zoomable image. Zoomable images have an accompanying csv file describing which image was used at each point. An html file is provided which uses [openseadragon](http://openseadragon.github.io) to render full resolution images on top of the mosaic when the user zooms far enough to need it. This allows for large mosaics that don't waste storage space on saving the same image multiple times or scaling small images up to a larger resolution to fit with the other tiles.
##options
- -p,  --picture : Image to use as reference for the photomosaic to be based on.
- -i,  --image : Directory of images that will be used in the creation of the photomosaic.
- -o,  --output : Name of photomosaic to be created. If the name references an image then the mosaic will be saved as a single image. Otherwise the mosaic will be saved as  a deep zoom image.
- -s, --shrink :  Amount that the input image will be shrunk by before building the mosaic.
- -b, --build : Sets whether or not to build a directory of zoomable images for each of the images in the input directory.
- -d, --directory : Directory to save zoomable images to if the **-b** flag is set

##Full Resolution HTML File
The zoomable image will only be saved with 64 by 64 pixel thumbnails of each image, so in order to view the full resolution mosaic you will need to build the zoomable image directory with **-b -d directory_name**. In *full_resolution.html*, change *main_image.dzi* (line 15) and *main_image.csv* (line 21) to the zoomable image and csv files that you created, and change *mosaic_files* (lines 48 and 105) to the directory of zoomable images.
##Tips
The more photos in your input directory the better your photomosaic will look. Creating a very large single image will require a large amount of memory and may end up failing, so it would be better to shrink it more or to save it as a zoomable image. Each image building up the mosaic is 64 by 64 pixels,and there is an image for every pixel of the shrunken input image, so the size of the output image will be multiplied by a ratio of 64/(shrink value). If your mosaic uses too many images you may get the error "Too many open files" which can be solved by using **ulimit -n** *some_integer* to set the number of open files allowed on your machine.
Your mosaic might not use every image in the directory sent into it, but if you create a directory of zoomable images it will create one for every image, so the following are some commands to delete the unused ones.

    cat out.csv | sed "s/,/\'$'\n''/g' | sed '/^$/d' | sort -u > used_files
    
    ls -l mosaic_files/ | grep ^- | awk '{print $9}' | sed 's/.\{4\}$//‘ > created_files
    
    rm $(comm -23 created_files used_files | sed -e 's/^/mosaic_files\//'| sed -e 's/$/.dzi/‘)
    
    rm -r $(comm -23 created_files used_files | sed -e 's/^/mosaic_files\//'| sed -e 's/$/_files/')

#RunContinuous
This creates 64 by 64 image photomosaics made up more photomosaics and utilizes [openseadragon](http://openseadragon.github.io) to create an infinite zoom through endless mosaics.
##options
- -i,  --image : Directory of images that will be used in the creation of the photomosaics.
- -d,  --directory : Directory that will be filled with created photomosaics
##Continuous HTML File
The only way to view this continuous photo mosaic is by using the accompanying *continuous.html* file. You will need to change all of the occurrences of *mosaics/* (lines 17, 23, and 168) to your output directory name, and change *32.csv* (line17) to the photomosaic you wish to start off with and *30.csv* (line 23) to the accompanying csv file.

#Examples
Input image
![Input image](https://raw.githubusercontent.com/nathanbain314/mosaic/master/input.jpg)
Static image photo mosaic
![Static mosaic](https://raw.githubusercontent.com/nathanbain314/mosaic/master/output.png)
[Full resolution zoomable mosaic](http://nathanbain.com/mosaic/full_resolution.html)

[Infinite continuous zoomable mosaic](http://nathanbain.com/mosaic/continuous.html)
