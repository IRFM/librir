**Librir** is a C/C++/Python library dedicated to the manipulation of infrared videos. It is mainly used in the [CEA/IRFM/WEST](https://irfm.cea.fr/en/west/) tokamak to archive and read back infrared videos acquired by the infrared [diagnostic](https://www.sciencedirect.com/science/article/pii/S0920379619304120), and as a building block for [Cognitive Vision/Machine Learning applications](https://www.sciencedirect.com/science/article/pii/S092037962300220X).

**Librir** is divided into 4 small libraries:

 - [tools](docs/tools.md): provides miscellaneous functions for file/folder management, datetime/string manipulation, etc...
 - [geometry](docs/geometry.md): provides various functions to manipulate 2D polygons.
 - [signal_processing](docs/signal_processing.md): provides various signal/image processing algorithms. 
 - [video_io](docs/video_io.md): record and read back compressed IR video files (lossy or lossless).
 - [registration](docs/registration.md) (Python only): motion correction within infrared videos.

**Librir** can be used from C, C++ or Python, and is known to work on Windows (compiled with MingW or Visual Studio) and Linux (compiled with `gcc`).

While developped in C++11, Librir is mostly used through its Python interface. The Python wrapper is built using the low level C interface (using the C++ components) and the Python *ctypes* module. 

# Compiling Librir

Librir depends on the following C libraries:

- [zstd](https://facebook.github.io/zstd/): generic compression library.
- [blosc](https://www.blosc.org/): compression of numerical signals.
- [libjpeg](http://libjpeg.sourceforge.net/): lossy image compression.
- [charls](https://github.com/team-charls/charls): lossy/lossless jpegls image compression.
- [ffmpeg](https://www.ffmpeg.org/): video compression/decompression.

Librir directly embbeds the source code of these libraries which are compiled within the Librir compilation process. Only the ffmpeg library needs to be compiled separately. Note that Librir only supports versions 4.3 and 4.4 of ffmpeg, which must be compiled with at least [libx264](https://www.videolan.org/developers/x264.html) and [kvazaar](https://github.com/ultravideo/kvazaar) support.

As configuring/compiling ffmpeg is a heavy error prone task, its compilation is automated by Librir compilation steps:

-	For msvc builds, Librir ships a precompiled version of ffmpeg with all required dependencies (64 bits only).
-	For gcc builds, the build process will automatically download ffmpeg, libx264 and kvazaar and compile them.

CMake is required to compile Librir. Below example shows how to compile Librir on Unix based environments:

```bash
git clone https://github.com/IRFM/librir.git
mkdir build
cd build
cmake ..
make
make install
````
This will install Librir locally in *build/install*. The *install* folder also contains the *librir* directory which is the python wrapper of the library. You can directly add the *install* folder to your Python library paths in order to use Librir from Python, or install it using the provided *setup.py*.

To perform a global installation, you must set the cmake option LOCAL_INSTALL to OFF.
To ease the compilation process, the scripts *build.sh* (Unix environments and Msys2) and *build.bat* (msvc build) are provided. Type ```build --help``` for more information.

# Using Librir from Python or C++

Installing Librir will produce the *librir* folder within the installation directory. This module is directly importable from Python as long as you add the installation path to your *sys.path*. You can also install it within your Python environment using the provided *setup.py*
To use Librir from C/C++ code, a *.pc* file is available in the installation folder and can be consumed by ```pkg-config```. For ```cmake``` users, Librir provides configuration files for the ```find_package``` function. Example:

```cmake
cmake_minimum_required(VERSION 3.16)

project(test)

list(APPEND CMAKE_PREFIX_PATH "librir_installation_path/lib/cmake/librir")

find_package(librir REQUIRED)

include_directories("${LIBRIR_INCLUDE_DIR}")
link_directories("${LIBRIR_LIB_DIRS}")
add_executable(test test.cpp)
target_link_libraries(test ${LIBRIR_LIBRARIES} )
```

# Authors

Librir is developed and maintained by:

- [Victor MONCADA](mailto:victor.moncada@cea.fr) (victor.moncada@cea.fr)
- [Leo DUBUS](mailto:leo.dubus@cea.fr) (leo.dubus@cea.fr)
- [Erwan GRELIER](mailto:erwan.grelier@cea.fr) (erwan.grelier@cea.fr)
- [Christian STARON](mailto:christian.staron@cea.fr) (christian.staron@cea.fr)
