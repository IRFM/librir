
# Tools library

The *tools* library provides low-level functions for file/folder management, datetime/string manipulation and data compression. All C++ functions/classes are defined within the *rir::* namespace. The library provides a C interface through *tools.h*. 

The Python module only expose (through the C interface) the following functions:

-	```zstd_compress```: compress a bytes object using zstd library
-	```zstd_decompress```: decompress a bytes object using zstd library
-	```zstd_compress_bound```: return the maximum compressed size for a given input buffer size
-	```blosc_compress_zstd```: compress a bytes object using blosc+zstd libraries
-	```blosc_decompress_zstd```: decompress a bytes object using blosc+zstd libraries

In addition, the tools module provides the ```FileAttribute``` class used to add custom attributes to any file stored in a custom trailer.  This is mostly used to add attributes to existing MP4 videos, as the additional trailer does not prevent ffmpeg from reading the video file. The ```FileAttribute``` class is mostly used by the *video_io* module to add the following attributes to compressed video files:

-	Timestamps: define custom timestamps for each frame of the video
-	Frame attributes: add any number of attributes to a frame on the form *key -> value* where key is a string object and value a bytes object
-	Global attributes: add global attributes to the video on the form *key -> value* where key is a string object and value a bytes object

Usage:

```python 
from librir.tools.FileAttributes import FileAttributes

# Create a FileAttributes object over 'my_file'.
# Create 'my_file' is it doesn't exist, otherwise read existing attributes from 'my_file' or create an attributes trailer if non is found.
f = FileAttributes("my_file")

#define some global attributes
f.attributes["attr1"] = 1
f.attributes["attr2"] = "a value"

# set timestamps BEFORE setting frame attributes
f.timestamps = [0, 1, 2, 3]

# set frame attributes for frame 0
f.set_frame_attributes(0, {"attr1" : 1, "attr2" : "a value"} )

# write attributes
f.close()

#
# read back and potentially update the attributes
#


f = FileAttributes("my_file")

print(f.attributes)
print(f.timestamps)
print(f.frame_attributes(0))
```