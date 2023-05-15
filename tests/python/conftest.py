import logging
import os
import sys
from pathlib import Path

import numpy as np
import pytest
from librir import IRMovie

thismodule = sys.modules[__name__]

logging.basicConfig()
logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)

pulse = 56927
base = Path(thismodule.__file__).parent

ROWS = 512
COLUMNS = 640
IMAGES = 10
N_TIS = 7
DL_MAX_VALUE = 8192


def generate_mock_movie_data_uniform(*shape):
    if len(shape) == 2:
        n_images = 1
    else:
        n_images = shape[0]

    dl = np.zeros(shape, dtype=np.uint16)
    tis = np.zeros(shape, dtype=np.uint16)
    step = 2**13
    if n_images > 1:
        step /= n_images - 1

    for i in range(n_images):
        tis[i] = i % N_TIS
        dl[i] = i * step

    data = dl | (tis << 13)

    return data


def generate_constant_mask_array(*shape):
    # shape = (n_images, n_rows, n_columns)
    mask = np.zeros(shape, dtype=bool)
    mask[..., mask.shape[-1] // 2 :] = True
    return mask


def generate_random_movie_data(n_rows, n_columns, n_images):
    dl = np.random.randint(
        0, (2**13), size=(n_images, n_rows, n_columns), dtype=np.uint16
    )  # >> 16  & (2**13)-1
    tis = np.random.randint(
        0, (2**3) - 1, size=(n_images, n_rows, n_columns), dtype=np.uint16
    )  # >> 29 & (2**3)-1

    data = dl | (tis << 13)

    return data


VALID_3D_SHAPES = [
    (1, 512, 640),
    (10, 512, 640),
    (10, 515, 640),
    (10, 240, 320),
    (10, 243, 320),
    (10, 256, 320),
    (10, 259, 320),
]
VALID_2D_SHAPES = [
    (512, 640),
]

VALID_SHAPES = VALID_2D_SHAPES + VALID_3D_SHAPES

INVALID_SHAPES = [
    (512,),
    (512, 640, 5, 2),
]




VALID_UNIFORM_2D_NUMPY_ARRAYS = [
    generate_mock_movie_data_uniform(*shape) for shape in VALID_2D_SHAPES
]

VALID_UNIFORM_3D_NUMPY_ARRAYS = [
    generate_mock_movie_data_uniform(*shape) for shape in VALID_3D_SHAPES
]

VALID_UNIFORM_NUMPY_ARRAYS = [
    generate_mock_movie_data_uniform(*shape) for shape in VALID_SHAPES
]

INVALID_UNIFORM_NUMPY_ARRAYS = [
    generate_mock_movie_data_uniform(*shape) for shape in INVALID_SHAPES
]

VALID_MASKS = [generate_constant_mask_array(*shape) for shape in VALID_3D_SHAPES]



# @pytest.fixture(scope="module")
# def uniform_movies():
#     return [IRMovie.from_numpy_array(arr) for arr in VALID_UNIFORM_NUMPY_ARRAYS]



@pytest.fixture(scope="session", params=VALID_SHAPES)
def array(request):
    shape = request.param
    yield generate_mock_movie_data_uniform(*shape)
    
@pytest.fixture(scope="session", params=VALID_3D_SHAPES)
def valid_3d_array(request):
    shape = request.param
    yield generate_mock_movie_data_uniform(*shape)
    
    
@pytest.fixture(scope="session", params=INVALID_SHAPES)
def bad_array(request):
    shape = request.param
    yield generate_mock_movie_data_uniform(*shape)
    

@pytest.fixture(scope="session", params=VALID_2D_SHAPES)
def valid_2D_array(request):
    shape = request.param
    yield generate_mock_movie_data_uniform(*shape)

@pytest.fixture(scope="session")
def movie(array):
    return IRMovie.from_numpy_array(array)

@pytest.fixture(scope="session")
def filename(movie):
    return movie.filename

# filenames = pytest.mark.parametrize("filenames", [mov.filename for mov in uniform_movies])