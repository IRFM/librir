import os
import tempfile
import time
from pathlib import Path

import numpy as np
import pytest
from librir.video_io import IRMovie, IRSaver
from librir.video_io.rir_video_io import (
    FILE_FORMAT_H264,
    FILE_FORMAT_PCR,
    correct_PCR_file,
    get_emissivity,
    h264_get_high_errors,
    h264_get_low_errors,
)


@pytest.mark.slow
def test_record_movie_with_lossless_compression(images):
    temp_folder = Path(tempfile.gettempdir())

    start = time.time()

    # create saver by specifying the output filename (any suffix), width and height
    s = IRSaver(temp_folder / "my_file.bin", 640, 512)

    # set some parameters
    s.set_parameter("compressionLevel", 8)  # maximum compression
    s.set_parameter("GOP", 20)  # Group Of Pictures set to 20
    s.set_parameter("threads", 4)  # Compress using 4 threads
    # Define 4 slices to allow multithreaded video reading
    s.set_parameter("slices", 4)

    # add global attributes
    s.set_global_attributes(
        {
            "Title": "A new video!",
            "Description": "Test case for lossless video compression",
        }
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
    file_size = os.stat(temp_folder / "my_file.bin").st_size

    print(
        "Compression took {} seconds ({} frames/s)".format(
            elapsed, len(images) / elapsed
        )
    )
    print("Compression factor is {}".format(float(theoric_size) / file_size))
    m = IRMovie.from_filename(temp_folder / "my_file.bin")

    assert m.images == len(images)
    assert m.image_size == (512, 640)
    assert m.filename == Path(temp_folder / "my_file.bin")

    print("Number of images: ", m.images)
    print("First timestamps (s): ", m.timestamps[0:10])
    print("Frame size: ", m.image_size)
    print("Filename: ", m.filename)
    print("Global attributes: ", m.attributes)
    print("Image as position 5: ", m.load_pos(5))
    print("Image as t = 0s: ", m.load_secs(0))


@pytest.mark.slow
def test_record_movie_with_lossy_compression(images):
    #########
    # Record video using lossy compression
    #########
    temp_folder = Path(tempfile.gettempdir())
    start = time.time()

    # create saver by specifying the output filename (any suffix), width and height
    s = IRSaver(temp_folder / "my_file2.bin", 640, 512)

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
    low_errors = h264_get_low_errors(s.handle)
    high_errors = h264_get_high_errors(s.handle)
    # close video
    s.close()

    elapsed = time.time() - start
    theoric_size = len(images) * images[0].shape[0] * images[0].shape[1] * 2
    file_size = os.stat(temp_folder / "my_file2.bin").st_size

    print(
        "Lossy compression took {} seconds ({} frames/s)".format(
            elapsed, len(images) / elapsed
        )
    )
    print("Lossy compression factor is {}".format(float(theoric_size) / file_size))


def test_calibration_files(movie: IRMovie):
    cfiles = movie.calibration_files


@pytest.fixture(scope="session")
def pcr_filename(images):
    temp_folder = Path(tempfile.gettempdir())
    filename = temp_folder / "tmp.pcr"
    header = np.zeros((256,), dtype=np.uint32)
    data = np.array(images, dtype=np.uint16)
    with open(filename, "wb") as f:
        f.write(header)
        f.write(data)
    correct_PCR_file(filename, data.shape[0], data.shape[1], 50)
    yield filename
    os.unlink(filename)


def test_correct_PCR_file(pcr_filename, images):
    data = np.array(images, dtype=np.uint16)
    correct_PCR_file(pcr_filename, data.shape[0], data.shape[1], 50)
    with IRMovie.from_filename(pcr_filename) as mov:
        assert mov.video_file_format == FILE_FORMAT_PCR


def test_pcr2h264(pcr_filename):
    with IRMovie.from_filename(pcr_filename) as mov:
        res = mov.pcr2h264()

    with IRMovie.from_filename(res) as mov:
        assert mov.video_file_format == FILE_FORMAT_H264


def test_get_emissivity(movie: IRMovie):
    get_emissivity(movie.handle)
