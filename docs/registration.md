

# Video registration

The *registration* module is a pure Python module used to compute translations within an otherwise fixed infrared movie with a sub-pixelic precision.
It relies on [opencv](https://opencv.org/) library and the function [findTransformECC](https://docs.opencv.org/3.4/dc/d6b/group__video__track.html#ga1aa357007eaec11e9ed03500ecbcbe47). Note that opencv is not listed as a librir dependency: the registration module might not be used at all, and opencv is a HUGE dependency. You will need to install opencv yourself if the registration module is to be used: ```pip install opencv-python```.

The class ```MaskedRegistratorECC``` is in charge of the registration process, and compute sub-pixels translations in a movie using a "smart" way to update the reference image from which translations are computed. Basically, the reference image is updated when the confidence value returned by ```findTransformECC``` is less than a certain threshold : min(Confidences) - 2*std(Confidences).

The returned translation values correspond to the translations from the very first image passed to ```MaskedRegistratorECC.start()```. You can use the ```translate``` function from the [signal_processing](signal_processing.md) module to correct detected vibrations.

Usage:
```python
import numpy as np
import os
import time
import random
from librir.signal_processing import translate
from librir.registration.masked_registration_ecc import MaskedRegistratorECC
from librir.geometry import draw_polygon

def add_noise(image, mean = 0, var = 0.5):
    """ Add gaussian noise to input image"""
    row,col= image.shape
    sigma = var**0.5
    gauss = np.random.normal(mean,sigma,(row,col))
    gauss = gauss.reshape(row,col)
    noisy = image + gauss
    return np.array(noisy, dtype = np.uint16)



# generate 100 noisy images with an average background value going from 10 to 110 by step of 1 containing a polygon translated by 1 pixel every image

images = []
translations_x = []
translations_y = []
background = np.zeros((512,640))#np.random.rand(512,640) * 5
polygon = [ [42,42] , [100,42], [200,200 ], [80,300] ]
polygon_img = np.zeros((512,640))
polygon_img = draw_polygon(polygon_img, polygon,10)

for i in range(10,110, 1):
    # define translation (dx,dy)
    dx = i -10 
    dy = i -10
    # translate polygon image
    pimg = translate(polygon_img,dx,dy,"nearest")
    # compute image
    img = background + np.ones((512,640))*i + pimg
    # add noise
    img = add_noise(img,0,1)
    images.append(img)
    translations_x.append(dx)
    translations_y.append(dy)

# Initialize the registrator object
reg = MaskedRegistratorECC(1,1)
# Set the first image
reg.start(images[0])
# Compute remaining images
for i in range(1,len(images)):
    reg.compute(images[i])
    
# Print x and y translations. We expect translations going from 0 to 99 by steps of 1
print("dx:" , reg.x)
print("dy:" , reg.y)
```