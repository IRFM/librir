import logging
import struct

import sys
from pathlib import Path
from typing import List

import numpy as np
import pytest
from librir import IRMovie

thismodule = sys.modules[__name__]

logging.basicConfig()
logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)

base = Path(thismodule.__file__).parent

IMAGES = 10
N_TIS = 7
DL_MAX_VALUE = 8192


def add_noise(image, mean=0, var=0.5):
    """Add gaussian noise to input image"""
    row, col = image.shape
    sigma = var**0.5
    gauss = np.random.normal(mean, sigma, (row, col))
    gauss = gauss.reshape(row, col)
    noisy = image + gauss
    return np.array(noisy, dtype=np.uint16)


def generate_mock_movie_data_uniform(*shape):
    if len(shape) == 2:
        n_images = 1
    else:
        n_images = shape[0]

    dl = np.zeros(shape, dtype=np.uint16)
    tis = np.zeros(shape, dtype=np.uint16)
    step = 2**13
    if n_images > 1:
        step /= n_images - 2

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


def forge_mock_metadata_in_last_line_for_2d_arrays(array):
    metadata = np.zeros((3, array.shape[1]), dtype=array.dtype)
    return np.vstack((array, metadata))


VALID_3D_SHAPES = [
    (1, 512, 640),
    (10, 512, 640),
    # (10, 515, 640),  # pb
    (10, 240, 320),
    # (10, 243, 320),  # pb
    (10, 256, 320),
    # (10, 259, 320),
]
VALID_2D_SHAPES = [
    (512, 640),
    # (515, 640),
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
    arr = generate_mock_movie_data_uniform(*shape)

    yield arr


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
    arr = generate_mock_movie_data_uniform(*shape)
    arr = forge_mock_metadata_in_last_line_for_2d_arrays(arr)
    yield arr


@pytest.fixture(scope="session")
def movie(array):
    with IRMovie.from_numpy_array(array) as mov:
        yield mov


@pytest.fixture(scope="session")
def filename(movie: IRMovie):
    return movie.filename


@pytest.fixture(scope="session")
def wrong_filename():
    return "not_existing_filename"


@pytest.fixture(scope="session")
def timestamps(array):
    return np.arange(len(array), dtype=float) * 2


@pytest.fixture(scope="session")
def movie_with_firmware_date(valid_2D_array):
    # 23-01-2023
    day = 23
    month = 1
    year = 2023
    firmware_date = np.uint32(0)

    firmware_date = (day << 24) | (month << 16) | year

    # struct.pack("I", firmware_date)
    r = bytearray(valid_2D_array.tobytes("C"))
    v = struct.pack("I", firmware_date)
    offset = (valid_2D_array.shape[0] - 3) * valid_2D_array.shape[1] * 2
    r[(offset + 254 * 2) : (offset + 256 * 2)] = v

    arr = np.frombuffer(bytes(r), dtype=np.uint16).reshape(valid_2D_array.shape)
    arr[-3, 254:256]
    with IRMovie.from_numpy_array(arr) as mov:
        yield mov


@pytest.fixture
def images() -> List[np.ndarray]:
    # generate 100 noisy images with an average background value going from 10 to 110 by step of 1
    images = []
    background = np.random.rand(512, 640) * 1000
    for i in range(10, 110, 1):
        img = background + np.ones((512, 640)) * i
        img = add_noise(img, 0, 0.5)
        images.append(img)
    return images
