import os
import shutil
import tempfile
import time
from pathlib import Path

import numpy as np
import numpy.testing as npt
import pytest
from librir.video_io import IRMovie, IRSaver
from librir.video_io.rir_video_io import (
    FileFormat,
    calibrate_image,
    camera_saturate,
    close_camera,
    correct_PCR_file,
    get_emissivity,
    get_filename,
    get_image_size,
    get_image_time,
    h264_add_loss,
    h264_close_file,
    h264_open_file,
    load_image,
    open_camera_memory,
    set_emissivity,
    h264_get_high_errors,
    h264_get_low_errors,
    set_global_emissivity,
    support_emissivity,
    supported_calibrations,
    video_file_format,
)
from tests.python.conftest import suppress_stdout_stderr


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
        s.add_image_lossy(
            images[i],
            i * 1e6,
            attributes={
                "my_int_attribute": 1,
                "my_str_attribute": "yeah",
            },
        )
    low_errors = h264_get_low_errors(s.handle)
    npt.assert_array_equal(low_errors, s.get_low_errors())
    high_errors = h264_get_high_errors(s.handle)
    npt.assert_array_equal(high_errors, s.get_high_errors())
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
    movie.calibration_files


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


def test_video_file_format(pcr_filename):
    assert video_file_format(pcr_filename) == FileFormat.PCR
    with pytest.raises(RuntimeError):
        video_file_format("nothing")


def test_correct_PCR_file(pcr_filename, images):
    data = np.array(images, dtype=np.uint16)
    correct_PCR_file(pcr_filename, data.shape[0], data.shape[1], 50)
    with IRMovie.from_filename(pcr_filename) as mov:
        assert mov.video_file_format == FileFormat.PCR


def test_pcr2h264(pcr_filename):
    with IRMovie.from_filename(pcr_filename) as mov:
        res = mov.pcr2h264()

    with IRMovie.from_filename(res) as mov:
        assert mov.video_file_format == FileFormat.H264


@pytest.mark.parametrize("emi", [0.5])
def test_set_global_emissivity(movie: IRMovie, emi):
    data = movie.data
    set_global_emissivity(movie.handle, emi)
    data_with_new_emissivity = calibrate_image(movie.handle, data, 0)
    # FIXME: Emissivity is not taken care of so far.
    # npt.assert_array_equal(data_with_new_emissivity, data / 2)


def test_set_emissivity(movie: IRMovie):
    data = movie.data.copy()
    emi = np.ones(data.shape[1:]) * 0.25

    set_emissivity(movie.handle, emi)
    readback_emi = get_emissivity(movie.handle)
    npt.assert_array_equal(emi, readback_emi)

    # data_with_new_emissivity = data / emi

    # movie.data


def test_get_emissivity(movie: IRMovie):
    get_emissivity(movie.handle)
    with pytest.raises(RuntimeError):
        with suppress_stdout_stderr():
            get_emissivity(-1)


def test_support_emissivity(movie: IRMovie):
    support_emissivity(movie.handle)


def test_enable_bad_pixels(movie: IRMovie):
    assert not movie.bad_pixels_correction
    movie.bad_pixels_correction = True
    assert movie.bad_pixels_correction
    movie.bad_pixels_correction = False
    assert not movie.bad_pixels_correction


def test_camera_saturate(movie: IRMovie):
    camera_saturate(movie.handle)


def test_open_camera_memory(movie: IRMovie):
    movie.filename
    data = movie.filename.read_bytes()
    with IRMovie.from_bytes(data):
        pass
    with pytest.raises(RuntimeError):
        open_camera_memory(bytes())


def test_get_filename(movie: IRMovie):
    get_filename(movie.handle)
    with pytest.raises(RuntimeError):
        get_filename(0)


def test_get_image_time(movie: IRMovie):
    get_image_time(movie.handle, 0)
    with pytest.raises(RuntimeError):
        with suppress_stdout_stderr():
            get_image_time(0, 0)


def test_get_image_size(movie: IRMovie):
    get_image_size(movie.handle)
    with pytest.raises(RuntimeError):
        get_image_size(0)


def test_supported_calibrations(movie: IRMovie):
    supported_calibrations(movie.handle)
    with pytest.raises(RuntimeError):
        supported_calibrations(0)


def test_load_image(movie: IRMovie):
    load_image(movie.handle, 0, 0)
    with pytest.raises(RuntimeError):
        load_image(0, 0, 0)
    with pytest.raises(RuntimeError):
        load_image(movie.handle, movie.images + 1, 0)


# def test_h264_add_loss(movie: IRMovie):
#     with tempfile.NamedTemporaryFile(delete=False) as f:
#         pass
#     saver = h264_open_file(f, movie.width, movie.height, lossy_height=None)
#     h264_add_loss(saver, movie[0])
#     h264_close_file(saver)
#     shutil.rmtree(f.name)
