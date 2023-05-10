import numpy as np
import os
import time
import random
from librir.signal_processing import translate
from librir.registration.masked_registration_ecc import MaskedRegistratorECC
from librir.geometry import draw_polygon


def add_noise(image, mean=0, var=0.5):
    """ Add gaussian noise to input image"""
    row, col = image.shape
    sigma = var ** 0.5
    gauss = np.random.normal(mean, sigma, (row, col))
    gauss = gauss.reshape(row, col)
    noisy = image + gauss
    return np.array(noisy, dtype=np.uint16)


# generate 100 noisy images with an average background value going from 10 to 110 by step of 1 containing a polygon translated by 1 pixel every image

images = []
translations_x = []
translations_y = []
background = np.zeros((512, 640))  # np.random.rand(512,640) * 5
polygon = [[42, 42], [100, 42], [200, 200], [80, 300]]
polygon_img = np.zeros((512, 640))
polygon_img = draw_polygon(polygon_img, polygon, 10)

for i in range(10, 110, 1):
    # define translation (dx,dy)
    dx = i - 10
    dy = i - 10
    # translate polygon image
    pimg = translate(polygon_img, dx, dy, "nearest")
    # compute image
    img = background + np.ones((512, 640)) * i + pimg
    # add noise
    img = add_noise(img, 0, 1)
    images.append(img)
    translations_x.append(dx)
    translations_y.append(dy)

# Initialize the registrator object
reg = MaskedRegistratorECC(1, 1)
# Set the first image
reg.start(images[0])
# Compute remaining images
for i in range(1, len(images)):
    reg.compute(images[i])

# Print x and y translations. We expect translations going from 0 to 99 by steps of 1
print("dx:", reg.x)
print("dy:", reg.y)
