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


# def generate_mock_mo  vie_data(n_rows, n_columns, n_images):
#     dl = np.zeros((n_images, n_rows, n_columns), dtype=np.uint16)
#     tis = np.zeros((n_images, n_rows, n_columns), dtype=np.uint16)
#     step = (2**13-0)/(n_images-1)
#     for i in range(n_images):
#         tis[i] = i % N_TIS
#         dl[i] = i * step
#
#     data = dl | (tis << 13)

#     return data


def generate_random_movie_data(n_rows, n_columns, n_images):
    dl = np.random.randint(
        0, (2**13), size=(n_images, n_rows, n_columns), dtype=np.uint16
    )  # >> 16  & (2**13)-1
    tis = np.random.randint(
        0, (2**3) - 1, size=(n_images, n_rows, n_columns), dtype=np.uint16
    )  # >> 29 & (2**3)-1

    data = dl | (tis << 13)

    return data


# def create_pcr_header(rows, columns, frequency=50, bits=16):
#     pcr_header = np.zeros((256,), dtype=np.uint32)
#     pcr_header[2] = columns
#     pcr_header[3] = rows
#     pcr_header[5] = bits  # bits
#     pcr_header[7] = frequency
#     pcr_header[9] = rows * columns * 2
#     pcr_header[10] = columns
#     pcr_header[11] = rows
#
#     return pcr_header
#
#
# def create_bin_header(version, triggers, compression):
#     bin_header = np.zeros((3 + 128,), dtype=np.uint8)
#     bin_header[0] = version
#     bin_header[1] = triggers
#     bin_header[2] = compression
#
#     return bin_header
#
#
# def create_movie_test_file(filename, header):
#     # temp = np.random.randint(0, 4000, size=(IMAGES,ROWS,COLUMNS), dtype=np.uint32)
#     data = generate_random_movie_data(ROWS, COLUMNS, IMAGES)
#
#     with open(filename, 'wb') as f:
#         f.write(header)
#         f.write(data)
#
#
# bin_headers = {'nocomp': create_bin_header(9, 0, 1),
#                'comp': create_bin_header(1, 1, 1)}
# pcr_headers = {'nocomp': create_pcr_header(ROWS, COLUMNS, 50, 16)}
#
# FILENAMES = []
#
# # for prefix, header in bin_headers.items():
# #     filename = base / (prefix + ".bin")
# #     create_movie_test_file(filename, header)
# #     FILENAMES.append(filename)
#
# for prefix, header in pcr_headers.items():
#     filename = base / (prefix + ".pcr")
#     create_movie_test_file(filename, header)
#     FILENAMES.append(filename)

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


class TestFilesContext(object):
    def __init__(self):
        self.uniform_movies = [
            IRMovie.from_numpy_array(arr) for arr in VALID_UNIFORM_NUMPY_ARRAYS
        ]
        self.constant_masks = VALID_MASKS

    @property
    def valid_arrays(self):
        return self.valid_2D_arrays + self.valid_3D_arrays

    @property
    def valid_2D_arrays(self):
        return VALID_UNIFORM_2D_NUMPY_ARRAYS

    @property
    def valid_3D_arrays(self):
        return VALID_UNIFORM_3D_NUMPY_ARRAYS

    @property
    def invalid_arrays(self):
        return [arr for arr in INVALID_UNIFORM_NUMPY_ARRAYS]

    @property
    def valid_movies(self):
        return self.uniform_movies

    @property
    def failing_movies(self):
        return []

    @property
    def movies(self):
        return self.valid_movies + self.failing_movies

    @property
    def filenames(self):
        uniform_filenames = [mov.filename for mov in self.uniform_movies]

        complete = uniform_filenames
        return complete

    # def __del__(self):
    #     for m in self.movies:
    #         m.close()
    #     for f in self.filenames:
    #         logger.debug(f"deleting file {f}")
    #         os.unlink(f)
