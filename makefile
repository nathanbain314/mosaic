RunMosaic:
	g++ -O3 -o RunMosaic mosaic.c++ -L/usr/X11/lib -I/opt/X11/include -lpthread -lX11

clean:
	rm -f RunMosaic
