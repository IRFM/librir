
# Signal processing library

The *signal_processing* library provides a few commonly used signal/image processing functions mainly to avoid introducing huge dependencies like opencv. All C++ functions/classes are defined within the *rir::* namespace. The library provides a C interface through *signal_processing.h*. 
The Python module *signal_processing* provides the following functions:

-	```translate```: Translate input image by a floating point offset (dx,dy)
-	```gaussian_filter```: Apply a gaussian filter on input image with given sigma value
-	```find_median_pixel```: Find the pixel value from which at least percent*image.size pixels are included
-	```extract_times```: Create a unique time vector from several ones
-	```resample_time_serie```: Resample a time serie based on a new time vector
-	```jpegls_encode```: Compress a 16 bits grayscale image using jpegls algorithm
-	```jpegls_decode```: Decode a jpegls compressed image
-	```label_image```: Closed Component Labelling algorithm
