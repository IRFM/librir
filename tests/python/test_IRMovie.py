import random

import numpy as np
import numpy.testing as npt
import pytest
from librir import IRMovie
from librir.video_io.IRMovie import CalibrationNotFound

@pytest.mark.instanstiation
def test_IRMovie_with_filename_as_input(filename):
    mov = IRMovie.from_filename(filename)
    assert type(mov) is IRMovie


@pytest.mark.instanstiation
def test_IRMovie_instantiation_with_2D_numpy_array(valid_2D_array):
    mov = IRMovie.from_numpy_array(valid_2D_array)
    expected = valid_2D_array[np.newaxis, :]
    npt.assert_array_equal(mov.data, expected)


@pytest.mark.instanstiation
def test_IRMovie_instantiation_with_3D_numpy_array(valid_3d_array):
    mov = IRMovie.from_numpy_array(valid_3d_array)
    npt.assert_array_equal(mov.data, valid_3d_array)


@pytest.mark.instanstiation
def test_IRMovie_instantiation_with_bad_numpy_array(bad_array):
    with pytest.raises(ValueError) as e:
        mov = IRMovie.from_numpy_array(bad_array)


@pytest.mark.h264
def test_save_movie_to_h264(filename):
    mov = IRMovie.from_filename(filename)
    dest_filename = f"{filename}_scratch.h264"

    mov.to_h264(dest_filename)
    mov2 = IRMovie.from_filename(dest_filename)
    npt.assert_array_equal(mov.data, mov2.data)


@pytest.mark.h264
# @pytest.mark.parametrize("filename", tfc.filenames)
def test_save_movie_with_pcr2h264(filename):
    mov = IRMovie.from_filename(filename)
    dest_filename = f"{filename}_scratch.h264"

    mov.pcr2h264(outfile=dest_filename)
    mov2 = IRMovie.from_filename(dest_filename)
    npt.assert_array_equal(mov.data, mov2.data)


@pytest.mark.thermavip
def test_show_movie_on_thermavip(filename):
    # import Thermavip as th
    mov = IRMovie.from_filename(filename)
    player_id = mov.to_thermavip()


@pytest.mark.h264
def test_save_movie_to_h264_from_slice(movie: IRMovie):
    # mov = IRMovie.from_filename(filename)
    start_image = random.choice(range(movie.images - 1 if movie.images > 1 else movie.images))
    dest_filename = f"{movie.filename}_scratch"

    movie.to_h264(dest_filename, start_img=start_image)
    mov2 = IRMovie.from_filename(dest_filename)
    npt.assert_array_equal(movie.data[start_image:], mov2.data)


def test_set_calibration(movie: IRMovie):
    movie.calibration = 'DL'
    assert movie._calibration_index == 0
    with pytest.raises(CalibrationNotFound):
        movie.calibration = 'T'
        
    assert movie.calibration == 'DL'
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


@pytest.mark.instanstiation
def test_from_handle(movie):
    handle = movie.handle
    new_mov = IRMovie.from_handle(handle)
    assert new_mov.handle == movie.handle


@pytest.mark.instanstiation
def test_from_filename(filename):
    mov = IRMovie.from_filename(filename)
    assert mov.filename == filename


@pytest.mark.io
@pytest.mark.needs_improvement
def test_close(array):
    mov = IRMovie.from_numpy_array(array)
    del mov