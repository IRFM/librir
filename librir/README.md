# Librir

**librir** as a C/C++/Python library to manipulate infrared video from the WEST tokamak, 
accessing WEST diagnostic signals and building Cognitive Vision/Machine Learning applications.

It is divided in 5  libraries:

 - **tools**: provides misceallenous functions for file/folder management, temporay folder management, datetime/string manipulation, etc.
 - **geometry**: provides various functions to manipulate 2D polygons.
 - **signal_processing**: provides various signal/image processing algorithms. 
 - **video_io**: record and read back compressed IR video files.
 - **west**: access to the WEST signal database (ARCADE).

**Librir** can be used from C, C++ or Python, and is known to work on Windows (compiled with MingW or Visual Studio) and Linux (compiled with gcc).




# Compiling Librir

Librir depends on the following C libraries:
 - [zstd](https://facebook.github.io/zstd/): generic compression library
 - [blosc](https://www.blosc.org/): compression of numerical signals
 - [libjpeg](http://libjpeg.sourceforge.net/): lossy image compression
 - [charls](https://github.com/team-charls/charls): lossy/lossless jpegls image compression
 - tslib: IRFM internal library to access the ARCADE database
 - [ffmpeg](https://www.ffmpeg.org/): video compression/decompression

Librir directly embbeds the source code of these libraries which are compiled in the librir compilation process. Only the ffmpeg library needs to be compiled separatly.

## Ffmpeg compilation

Librir needs a version of ffmpeg compiled with h264 and/or h265 video codec support. h264 video codec implementation provided by libx264 is preferred as it produces smaller video files at a faster rate than any h265/HEVC implementation. The choice of video codec library will decide the librir license:
 - ffmpeg compiled with [libx264](https://www.videolan.org/developers/x264.html) or [libx265](https://www.videolan.org/developers/x265.html) will be under the GPL license, as well as librir.
 - ffmpeg compiled with [kvazaar](https://github.com/ultravideo/kvazaar) library will be under the LGPL license, as well as librir.

A prebuild version of ffmpeg for msvc 64 bits(GPL license, compiled with libx264 and kvazaar) is provided within the librir in the 3rd_64 directory.
To build ffmpeg yourself (with either gcc or msvc), follow the instructions in 3rd_64/build_ffmpeg.txt


## Librir compilation

Librir compilation system relies on [qmake](https://doc.qt.io/qt-5/qmake-manual.html).
To build Visual Studio project files, type:

    qmake -r -tp vc librir.pro "CONFIG+=release"

To generate a Makefile and compile librir, type:

    qmake -r librir.pro "CONFIG+=release"
    make
Headers, compiled libraries and necessary files will be copied to the *librir* subfolder. The *librir* folder is, in itself, a valid Python module that can be copied to the *site-packages* directory of your Python installation.

# Authors
Librir is developed and maintained by:
 -  Victor MONCADA (victor.moncada@cea.fr)
 -  Leo DUBUS (leo.dubus@cea.fr)
 -  Erwan GRELIER (erwan.grelier@cea.fr)
