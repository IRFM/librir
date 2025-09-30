import shutil
import librir.geometry as ge
from librir.low_level.misc import get_memory_folder, toArray, toCharP, toString
import librir.signal_processing as sp
import librir.signal_processing.BadPixels as bp
from librir.signal_processing.rir_signal_processing import (
    bad_pixels_correct,
    keep_largest_area,
    label_image,
)
import librir.tools.FileAttributes as fa
import numpy as np
import numpy.testing as npt
import pytest
from librir.video_io import IRMovie, IRSaver, get_filename, video_file_format

from librir import tools as rts
from tests.python.conftest import suppress_stdout_stderr


def test_memory_folder():
    folder = get_memory_folder()
    shutil.rmtree(folder)
    assert not folder.exists()
    folder = get_memory_folder()
    assert folder.exists()


def test_to_string():
    with pytest.raises(TypeError):
        assert toString("a") == "a"
    assert toString(b"a") == "a"
    assert toString(b"\xe2\x82\xac") == "€"


def test_to_array():
    npt.assert_array_equal(toArray("a"), np.array(("a",), dtype="c"))


def test_to_charp():
    # with pytest.raises(TypeError):
    assert toCharP("a") == b"a"
    assert toCharP(b"a") == b"a"
    assert toCharP(1) == b"\x00"


def test_zstd_compress_bound():
    print(rts.zstd_compress_bound(100))


@pytest.fixture
def data_bytes():
    return b"toto"


@pytest.fixture
def c(data_bytes):
    return rts.zstd_compress(data_bytes)


def test_zstd_compress(data_bytes):
    rts.zstd_compress(data_bytes)
    rts.zstd_compress(data_bytes.decode())
    # FIXME: didn't find a way to make zstd_compress fail
    # with pytest.raises(RuntimeError):
    #     rts.zstd_compress()


def test_zstd_decompress(c, data_bytes):
    rts.zstd_decompress(c) == data_bytes
    with pytest.raises(RuntimeError):
        rts.zstd_decompress(b"garbage")
    with pytest.raises(RuntimeError):
        rts.zstd_decompress("garbage")


@pytest.fixture
def file_attributes(movie: IRMovie):
    filename = movie.filename
    attrs = fa.FileAttributes.from_filename(filename)
    return attrs


@pytest.fixture
def filled_file_attributes(file_attributes: fa.FileAttributes):
    file_attributes.attributes = {"toto": 2, "tutu": "tata"}
    return file_attributes


def test_file_attribute_is_open(file_attributes: fa.FileAttributes):
    assert file_attributes.is_open()


def test_file_attribute_discard(file_attributes: fa.FileAttributes):
    assert file_attributes.handle > 0

    file_attributes.attributes = {"toto": 2, "tutu": "tata"}
    assert file_attributes.attributes == {"toto": 2, "tutu": "tata"}
    file_attributes.discard()
    assert file_attributes.handle == 0
    assert file_attributes.discard() is None
    assert not file_attributes.is_open()

    # assert file_attributes.attributes == {}


def test_file_attribute_from_buffer(movie_as_buffer: IRMovie):
    fa.FileAttributes.from_buffer(movie_as_buffer)


def test_file_attribute_from_context(movie: IRMovie):
    with fa.FileAttributes.from_filename(movie.filename) as file_attributes:
        file_attributes


def test_file_attributes_with_movie_data(movie: IRMovie):
    filename = movie.filename
    attrs = fa.FileAttributes.from_filename(filename)
    attrs.attributes = {"toto": 2, "tutu": "tata"}
    attrs.timestamps = range(movie.images)
    npt.assert_array_equal(attrs.timestamps, np.array(range(movie.images)))
    attrs.set_frame_attributes(movie.images - 1, {"toto": 2, "tutu": "tata"})
    attrs.close()

    attrs = fa.FileAttributes.from_filename(filename)
    assert attrs.attributes == {"toto": b"2", "tutu": b"tata"}
    assert attrs.frame_count() == movie.images
    assert attrs.frame_attributes(movie.images - 1) == {"toto": b"2", "tutu": b"tata"}
    attrs.close()

    # os.unlink(filename)


def test_file_attributes_from_buffer(movie: IRMovie):
    movie


polygon = [[0, 0], [1, 0], [1.2, 0.2], [1.8, 0.1], [5, 5], [3.2, 5], [0, 9]]
polygon2 = np.array(polygon, dtype=np.float64) * 1.5


def test_polygon_interpolate():
    print(ge.polygon_interpolate(polygon, polygon2, 0.5))


# img = np.zeros((10, 10), dtype=np.int32)


def test_draw_polygon(img):
    im = ge.draw_polygon(img, polygon, 1)
    print(im)
    bad_img = img[np.newaxis, :]
    with pytest.raises(RuntimeError):
        with suppress_stdout_stderr():
            ge.draw_polygon(bad_img, polygon, 1)
    bad_img = img.astype(object)
    with pytest.raises(RuntimeError):
        ge.draw_polygon(bad_img, polygon, 1)


def test_extract_polygon(img):
    print(ge.extract_polygon(img, 1))
    bad_img = img[np.newaxis, :]
    with pytest.raises(RuntimeError):
        ge.extract_polygon(bad_img, 1)
    bad_img = img.astype(object)
    with pytest.raises(RuntimeError):
        ge.extract_polygon(bad_img, 1)

    ge.extract_polygon(img, 1, max_size=1)


def test_extract_convex_hull():
    ge.extract_convex_hull(polygon)
    assert ge.extract_convex_hull([]) == []


def test_rdp_simplify_polygon():
    print(ge.rdp_simplify_polygon(polygon, 0))


def test_rdp_simplify_polygon2():
    print(ge.rdp_simplify_polygon2(polygon, 5))


def test_minimum_area_bbox():
    ge.minimum_area_bbox(polygon)
    assert ge.minimum_area_bbox([]) == ([0, 0], 0, 0, 0, 0)
    assert ge.minimum_area_bbox(polygon) != ([np.nan, np.nan], 0.0, 0.0, np.nan, np.nan)


def test_count_pixel_in_polygon():
    assert ge.count_pixel_in_polygon(polygon) >= 23
    assert ge.count_pixel_in_polygon([]) == 0


def test_translate(img):
    # global img
    img = np.ones((12, 12), dtype=np.float64)
    img = sp.translate(img, 1.2, 1.3, "constant", 0)
    print(img)
    with pytest.raises(RuntimeError):
        _img = np.ones((12, 12, 12), dtype=np.float64)
        _img = sp.translate(_img, 1.2, 1.3, "constant", 0)


def test_gaussian_filter(img):
    # global img
    img = sp.gaussian_filter(img, 0.75)
    print(img)


def test_find_median_pixel():
    img = np.array(range(100), dtype=np.uint16)
    img.shape = (10, 10)
    print(sp.find_median_pixel(img, 0.5))
    print(sp.find_median_pixel(img, 0.2))


def test_extract_times():
    times1 = [0, 0.2, 1, 1.5, 2.3, 3.3, 4, 5]
    times2 = [-1, 3, 4, 4.3, 4.7]
    print(sp.extract_times((times1, times2), "union"))
    print(sp.extract_times((times1, times2), "inter"))


def test_resample_time_serie():
    x = range(10)
    y = range(10)
    times = [0, 0.2, 1, 1.5, 2.3, 3.3, 4, 5, 5.6, 9.9, 10, 12, 13]
    print(sp.resample_time_serie(x, y, times))
    print(sp.resample_time_serie(x, y, times, 0))
    print(sp.resample_time_serie(x, y, times, None, False))


@pytest.fixture
def img():
    img = np.ones((20, 12), dtype=np.uint16)
    return img


def test_bad_pixels(img):
    img = np.ones((20, 12), dtype=np.uint16)
    b = bp.BadPixels(img)
    print(b.correct(img))
    del b


def test_bad_pixels_correct_runtime_errors():
    with pytest.raises(RuntimeError):
        bad_pixels_correct(0, 0)


def test_bad_pixels_label_image_runtime_errors():
    wrong_dimension_image = np.ndarray((10, 10, 10))
    with pytest.raises(RuntimeError):
        label_image(wrong_dimension_image, 0)

    wrong_dtype_image = np.ndarray((10, 10), dtype="object")
    with pytest.raises(RuntimeError):
        label_image(wrong_dtype_image, 0)

    # with pytest.raises(RuntimeError):
    #     label_image(np.Arr, 0)


def test_bad_pixels_keep_largest_area_runtime_errors():
    wrong_dimension_image = np.ndarray((10, 10, 10))
    with pytest.raises(RuntimeError):
        keep_largest_area(wrong_dimension_image, 0)

    wrong_dtype_image = np.ndarray((10, 10), dtype="object")
    with pytest.raises(RuntimeError):
        keep_largest_area(wrong_dtype_image, 0)

    # with pytest.raises(RuntimeError):
    #     label_image(np.Arr, 0)


def test_label_image(img):
    polygon = [[0, 0], [1, 0], [1.2, 0.2], [1.8, 0.1], [5, 5], [3.2, 5], [0, 9]]
    polygon2 = [[15, 15], [16, 17], [14, 17]]
    img = np.zeros((20, 20), np.int32)
    img = ge.draw_polygon(img, polygon, 5)
    img = ge.draw_polygon(img, polygon2, 5)
    print(img)
    print(sp.label_image(img))
    print(sp.keep_largest_area(img))


def test_ir_saver_movie():
    img0 = np.zeros((20, 20), dtype=np.int32)
    img1 = np.ones((20, 20), dtype=np.int32)
    s = IRSaver("test.h264", 20, 20, 20, 8)
    s.add_image(img0, 0)
    s.add_image(img1, 1)
    s.close()

    m = IRMovie.from_filename("test.h264")
    print(m.images)
    print(m.image_size)
    print(m.timestamps)
    print(m.calibrations)
    print(m.filename)
    print(m.calibration_files)
    print(m[0])
    print(m[1])


def test_wrong_filename_video_file_format(wrong_filename):
    with pytest.raises(RuntimeError):
        get_filename(0)
        video_file_format(wrong_filename)


def test_set_global_emissivity(movie: IRMovie):
    movie.emissivity
