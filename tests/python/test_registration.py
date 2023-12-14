from librir.geometry.rir_geometry import draw_polygon
from librir.registration.masked_registration_ecc import MaskedRegistratorECC
import numpy as np
import pytest
from librir.signal_processing import translate

from .conftest import add_noise


# background = np.zeros((512, 640))  # np.random.rand(512,640) * 5
@pytest.fixture
def background():
    return np.zeros((512, 640))


# polygon = [[42, 42], [100, 42], [200, 200], [80, 300]]
@pytest.fixture
def polygon():
    return [[42, 42], [100, 42], [200, 200], [80, 300]]


# polygon_img = np.zeros((512, 640))
# polygon_img = draw_polygon(polygon_img, polygon, 10)
@pytest.fixture
def polygon_img(polygon):
    _polygon_img = np.zeros((512, 640))
    return draw_polygon(_polygon_img, polygon, 10)


@pytest.fixture
def translated_images_t(background, polygon_img):
    images = []
    translations_x = []
    translations_y = []
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
    return images, translations_x, translations_y


def test_mask_registrator(translated_images_t):
    images, translations_x, translations_y = translated_images_t
    reg = MaskedRegistratorECC(1, 1)
    # Set the first image
    reg.start(images[0])
    # Compute remaining images
    for i in range(1, len(images)):
        reg.compute(images[i])
    # Print x and y translations. We expect translations going from 0 to 99 by steps of 1
    # x = np.array(reg.x)
    # x = (x/5)*5
    # x = np.ceil(x)
    # assert all(x.astype(int) == range(100))
    # y = np.array(reg.y)
    # y = y.round()
    # assert all(x.astype(int) == range(100))
    # assert reg.y == range(100)
