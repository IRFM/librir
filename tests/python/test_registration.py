import os
from librir.geometry.rir_geometry import draw_polygon
from librir.registration.masked_registration_ecc import (
    MaskedRegistratorECC,
    manage_computation_and_tries,
)
import numpy as np
import pytest
from librir.signal_processing import translate
from librir.geometry import extract_convex_hull
from librir.video_io.IRMovie import IRMovie
from .conftest import add_noise


# background = np.zeros((512, 640))  # np.random.rand(512,640) * 5
@pytest.fixture
def background():
    return np.zeros((512, 640))


# polygon = [[42, 42], [100, 42], [200, 200], [80, 300]]
@pytest.fixture
def polygon():
    return [[42, 42], [100, 42], [200, 200], [80, 300]]


def test_extract_convex_hull(polygon):
    extract_convex_hull(polygon)


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


@pytest.fixture
def movie_with_polygon_drawn(movie: IRMovie, polygon):
    step = 10
    images = []
    translations_x = []
    translations_y = []
    for i in range(movie.images):
        _img = movie[i]
        avg_value = _img.mean().astype(int)
        draw_polygon(_img, polygon, avg_value + 50)
        dx = i - step
        dy = i - step
        pimg = translate(_img, dx, dy, "nearest")
        img = _img.mean().astype(int) + np.ones(movie.data.shape[1:]) * i + pimg
        img = add_noise(img, 0, 1)
        images.append(img)
        translations_x.append(dx)
        translations_y.append(dy)
    data = np.array(images)
    with IRMovie.from_numpy_array(data) as mov:
        yield mov


@pytest.fixture
def reg(translated_images_t):
    return MaskedRegistratorECC(1, 1)


def test_mask_registrator(reg: MaskedRegistratorECC, translated_images_t):
    images, translations_x, translations_y = translated_images_t

    # Set the first image
    reg.start(images[0])
    # Compute remaining images
    for i in range(1, len(images)):
        reg.compute(images[i])

    # reg.to_reg_file("toto.regfile")
    # Print x and y translations. We expect translations going from 0 to 99 by steps of 1
    # x = np.array(reg.x)
    # x = (x/5)*5
    # x = np.ceil(x)
    # assert all(x.astype(int) == range(100))
    # y = np.array(reg.y)
    # y = y.round()
    # assert all(x.astype(int) == range(100))
    # assert reg.y == range(100)


# def test_motion_correction()


def test_set_registration_file_to_IRMovie(
    movie_with_polygon_drawn: IRMovie, reg: MaskedRegistratorECC
):
    images = movie_with_polygon_drawn.data
    assert not movie_with_polygon_drawn.registration

    # Set the first image
    reg.start(images[0])
    # Compute remaining images
    for i in range(1, len(images)):
        reg.compute(images[i])

    reg_file = f"{movie_with_polygon_drawn.filename.stem}.regfile"
    reg.to_reg_file(reg_file)
    movie_with_polygon_drawn.registration_file = reg_file
    movie_with_polygon_drawn[0]
    assert movie_with_polygon_drawn.registration
    assert movie_with_polygon_drawn.registration_file.name == reg_file

    os.unlink(reg_file)


def test_manage_computation_and_tries(
    movie_with_polygon_drawn: IRMovie, reg: MaskedRegistratorECC
):
    images = movie_with_polygon_drawn.data

    # Set the first image
    reg.start(images[0])
    for img in images[1:]:
        manage_computation_and_tries(img, reg)
