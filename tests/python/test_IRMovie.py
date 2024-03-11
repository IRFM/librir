from pathlib import Path
import random
from typing import Dict
from librir.video_io.rir_video_io import FileFormat
from librir.video_io.utils import is_ir_file_corrupted, split_rush

import numpy as np
import numpy.testing as npt
import pytest
from librir import IRMovie
from librir.video_io.IRMovie import CalibrationNotFound


@pytest.mark.instantiation
def test_IRMovie_with_filename_as_input(filename):
    mov = IRMovie.from_filename(filename)
    assert type(mov) is IRMovie
    with pytest.raises(RuntimeError):
        IRMovie.from_filename("")
    assert mov.filename == filename


@pytest.mark.instantiation
def test_IRMovie_with_handle_as_input(array):
    with IRMovie.from_numpy_array(array) as movie:
        new_mov = IRMovie(movie.handle)
        assert new_mov.handle == movie.handle
        new_mov.__tempfile__ = None
        movie.__tempfile__ = None
        with pytest.raises(RuntimeError):
            IRMovie(0)


@pytest.mark.instantiation
def test_IRMovie_instantiation_with_2D_numpy_array(valid_2D_array):
    mov = IRMovie.from_numpy_array(valid_2D_array)
    expected = valid_2D_array[np.newaxis, :]
    npt.assert_array_equal(mov.data, expected)


@pytest.mark.instantiation
def test_IRMovie_instantiation_with_3D_numpy_array(valid_3d_array):
    mov = IRMovie.from_numpy_array(valid_3d_array)
    npt.assert_array_equal(mov.data, valid_3d_array)


@pytest.mark.instantiation
def test_IRMovie_instantiation_with_bad_numpy_array(bad_array):
    with pytest.raises(ValueError) as e:
        mov = IRMovie.from_numpy_array(bad_array)


@pytest.mark.h264
def test_save_movie_to_h264(movie: IRMovie):
    # mov = IRMovie.from_filename(filename)
    filename = movie.filename
    dest_filename = f"{filename}_scratch.h264"

    movie.to_h264(dest_filename)
    mov2 = IRMovie.from_filename(dest_filename)
    npt.assert_array_equal(movie.data, mov2.data)


@pytest.mark.h264
# @pytest.mark.parametrize("filename", tfc.filenames)
def test_save_movie_with_pcr2h264(filename):
    mov = IRMovie.from_filename(filename)
    dest_filename = f"{filename}_scratch.h264"

    mov.pcr2h264(outfile=dest_filename)
    mov2 = IRMovie.from_filename(dest_filename)
    npt.assert_array_equal(mov.data, mov2.data)


# @pytest.mark.thermavip
# def test_show_movie_on_thermavip(movie: IRMovie):
#     # import Thermavip as th


#     player_id = movie.to_thermavip()


@pytest.mark.h264
def test_save_movie_to_h264_from_slice(movie: IRMovie):
    # mov = IRMovie.from_filename(filename)
    start_image = random.choice(
        range(movie.images - 1 if movie.images > 1 else movie.images)
    )
    dest_filename = f"{movie.filename}_scratch"

    movie.to_h264(dest_filename, start_img=start_image)
    mov2 = IRMovie.from_filename(dest_filename)
    npt.assert_array_equal(movie.data[start_image:], mov2.data)


def test_set_calibration(movie: IRMovie):
    movie.calibration = "DL"
    assert movie._calibration_index == 0
    with pytest.raises(CalibrationNotFound):
        movie.calibration = "T"

    assert movie.calibration == "DL"
    assert movie._calibration_index == 0

    with pytest.raises(CalibrationNotFound):
        movie.calibration = 1


@pytest.mark.accessors
def test_load_pos_with_DL(movie: IRMovie):
    for i in range(movie.images):
        movie.load_pos(i)
        movie.load_pos(i, 0)
        movie.load_pos(i, "DL")
        with pytest.raises(CalibrationNotFound) as exc:
            movie.load_pos(i, 1)
        with pytest.raises(CalibrationNotFound) as exc:
            movie.load_pos(i, "T")
        with pytest.raises(CalibrationNotFound) as exc:
            movie.load_pos(i, 3)


# @pytest.mark.accessors
# def test_load_pos_with_temperature(movie):
#     assert False
@pytest.fixture(scope="function")
def movie_as_bytes(filename) -> bytes:
    with open(filename, "rb") as f:
        data = f.read()
    return data


@pytest.mark.instantiation
def test_from_filename(filename):
    mov = IRMovie.from_filename(filename)
    assert mov.filename == filename


def test_from_bytes_with_timestamps(movie_as_bytes, timestamps):
    mov2 = IRMovie.from_bytes(movie_as_bytes, times=timestamps)
    npt.assert_array_equal(mov2.timestamps, timestamps[: mov2.images])


@pytest.mark.io
@pytest.mark.needs_improvement
def test_close(array):
    mov = IRMovie.from_numpy_array(array)
    del mov
    mov = IRMovie.from_numpy_array(array, attrs={"name": "toto"})
    assert mov.attributes["name"]
    del mov


def test_video_file_format(movie: IRMovie):
    assert movie.video_file_format == FileFormat.H264.value


def test_split_rush(movie: IRMovie):
    total = movie.images
    steps = 2
    filenames = split_rush(movie.filename, step=steps)
    assert len(filenames) == (total // steps)

    for f in filenames:
        assert not is_ir_file_corrupted(f)


def test_is_ir_file_corrupted(filename):
    assert not is_ir_file_corrupted(filename)
    assert is_ir_file_corrupted("inexistent_filename")


def test_movie_getitem(movie: IRMovie):
    img = movie[0]


def test_frames_attributes(movie: IRMovie):
    movie.frame_attributes
    movie.frames_attributes


def test_flip_camera_calibration_when_no_calibration(movie: IRMovie):
    with pytest.raises(RuntimeError):
        movie.flip_calibration(True, False)
    with pytest.raises(RuntimeError):
        movie.flip_calibration(False, False)
    with pytest.raises(RuntimeError):
        movie.flip_calibration(False, True)
    with pytest.raises(RuntimeError):
        movie.flip_calibration(True, True)


@pytest.mark.parametrize("attrs", ({}, {"additional": 123}), ids=("none", "additional"))
def test_save_subset_of_movie_to_h264(movie: IRMovie, attrs: Dict[str, int]):
    dest_filename = Path(f"{movie.filename}_subset.h264")
    count = 1 if movie.images == 1 else movie.images // 2
    frame_attributes = [{"additional_frame_attribute": i} for i in range(movie.images)]

    with pytest.raises(RuntimeError):
        movie.to_h264(
            dest_filename,
            start_img=0,
            count=movie.images // 2,
            attrs=attrs,
            frame_attributes=frame_attributes,
        )
    with pytest.raises(RuntimeError):
        movie.to_h264(
            dest_filename,
            start_img=movie.images // 2,
            count=movie.images // 2,
            attrs=attrs,
            frame_attributes=frame_attributes,
        )
    with pytest.raises(RuntimeError):
        movie.to_h264(
            dest_filename,
            start_img=movie.images // 2,
            count=0,
            attrs=attrs,
            frame_attributes=frame_attributes,
        )
    with pytest.raises(RuntimeError):
        movie.to_h264(
            dest_filename,
            start_img=movie.images // 2,
            count=0,
            attrs=attrs,
            frame_attributes=None,
        )
    # assert not dest_filename.exists()

    movie.to_h264(
        dest_filename,
        start_img=0,
        count=count,
        attrs=attrs,
        frame_attributes=[{"additional_frame_attribute": i} for i in range(count)],
    )
    # assert dest_filename.exists()
    expected_attrs = {k: str(v).encode() for k, v in attrs.items()}
    expected_attrs["GOP"] = movie.attributes["GOP"]
    # expected_attrs.update({k: str(v).encode() for k, v in attrs.items()})

    with IRMovie.from_filename(dest_filename) as subset_movie:
        # subset_movie.load_pos(0)
        assert subset_movie.attributes == expected_attrs
        assert len(subset_movie.frames_attributes) == subset_movie.images

    # dest_filename.unlink()
