import os
import time

import numpy as np

from librir.video_io.IRMovie import IRMovie
from librir.video_io.IRSaver import IRSaver


def add_noise(image, mean=0, var=0.5):
    """Add gaussian noise to input image"""
    row, col = image.shape
    sigma = var**0.5
    gauss = np.random.normal(mean, sigma, (row, col))
    gauss = gauss.reshape(row, col)
    noisy = image + gauss
    return np.array(noisy, dtype=np.uint16)


# generate 100 noisy images with an average background value going from 10 to 110 by step of 1
images = []
background = np.random.rand(512, 640) * 1000
for i in range(10, 110, 1):
    img = background + np.ones((512, 640)) * i
    img = add_noise(img, 0, 0.5)
    images.append(img)


#########
# Record video using lossless compression
#########

start = time.time()

# create saver by specifying the output filename (any suffix), width and height
s = IRSaver("my_file.bin", 640, 512)

# set some parameters
s.set_parameter("compressionLevel", 8)  # maximum compression
s.set_parameter("GOP", 20)  # Group Of Pictures set to 20
s.set_parameter("threads", 4)  # Compress using 4 threads
# Define 4 slices to allow multithreaded video reading
s.set_parameter("slices", 4)

# add global attributes
s.set_global_attributes(
    {"Title": "A new video!", "Description": "Test case for lossless video compression"}
)

# add images
for i in range(len(images)):
    # The second argument is the image timestamp in nanoseconds
    # Wa could also add frame attributes by passing a dict as third argument
    s.add_image(images[i], i * 1e6)

# close video
s.close()

elapsed = time.time() - start
theoric_size = len(images) * images[0].shape[0] * images[0].shape[1] * 2
file_size = os.stat("my_file.bin").st_size

print(
    "Compression took {} seconds ({} frames/s)".format(elapsed, len(images) / elapsed)
)
print("Compression factor is {}".format(float(theoric_size) / file_size))


#########
# Record video using lossy compression
#########


start = time.time()

# create saver by specifying the output filename (any suffix), width and height
s = IRSaver("my_file2.bin", 640, 512)

# set some parameters
s.set_parameter("compressionLevel", 8)  # maximum compression
s.set_parameter("GOP", 20)  # Group Of Pictures set to 20
s.set_parameter("threads", 4)  # Compress using 4 threads
# Define 4 slices to allow multithreaded video reading
s.set_parameter("slices", 4)
s.set_parameter("lowValueError", 3)  # Set low and high compression errors to 3
# Set low and high compression errors to 3
s.set_parameter("highValueError", 3)
# For this example, remove the error reduction factor
s.set_parameter("stdFactor", 0)

# add images
for i in range(len(images)):
    s.add_image_lossy(images[i], i * 1e6)

# close video
s.close()

elapsed = time.time() - start
theoric_size = len(images) * images[0].shape[0] * images[0].shape[1] * 2
file_size = os.stat("my_file2.bin").st_size

print(
    "Lossy compression took {} seconds ({} frames/s)".format(
        elapsed, len(images) / elapsed
    )
)
print("Lossy compression factor is {}".format(float(theoric_size) / file_size))


#########
# Read back
#########

m = IRMovie.from_filename("my_file.bin")
print("Number of images: ", m.images)
print("First timestamps (s): ", m.timestamps[0:10])
print("Frame size: ", m.image_size)
print("Filename: ", m.filename)
print("Global attributes: ", m.attributes)
print("Image as position 5: ", m.load_pos(5))
print("Image as t = 0s: ", m.load_secs(0))
