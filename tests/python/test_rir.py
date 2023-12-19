from librir.video_io import IRMovie, IRSaver
import numpy as np

import librir.geometry as ge
import librir.signal_processing as sp
import librir.signal_processing.BadPixels as bp
from librir.video_io import video_file_format, get_filename
import librir.tools.FileAttributes as fa
from librir import tools as rts
import pytest


def test_zstd_compress_bound():
    print(rts.zstd_compress_bound(100))


@pytest.fixture
def data_bytes():
    return b"toto"


@pytest.fixture
def c(data_bytes):
    return rts.zstd_compress(data_bytes)


def test_zstd_compress(data_bytes):
    c = rts.zstd_compress(data_bytes)
    print(len(c))


def test_zstd_decompress(c, data_bytes):
    rts.zstd_decompress(c) == data_bytes


def test_blosc_compress_zstd(data_bytes):
    rts.blosc_compress_zstd(data_bytes, 2, rts.BLOSC_SHUFFLE, 1)


@pytest.fixture
def compressed_blosc(data_bytes):
    return rts.blosc_compress_zstd(data_bytes, 2, rts.BLOSC_SHUFFLE, 1)


def test_blosc_decompress_zstd(compressed_blosc):
    print(rts.blosc_decompress_zstd(compressed_blosc))


def test_file_attributes():
    attrs = fa.FileAttributes("attrs.test")
    attrs.attributes = {"toto": 2, "tutu": "tata"}
    attrs.timestamps = range(11)
    attrs.set_frame_attributes(10, {"toto": 2, "tutu": "tata"})
    attrs.close()

    attrs = fa.FileAttributes("attrs.test")
    print(attrs.attributes)
    print(attrs.frame_count())
    print(attrs.frame_attributes(0))
    print(attrs.frame_attributes(10))


polygon = [[0, 0], [1, 0], [1.2, 0.2], [1.8, 0.1], [5, 5], [3.2, 5], [0, 9]]
polygon2 = np.array(polygon, dtype=np.float64) * 1.5


def test_polygon_interpolate():
    print(ge.polygon_interpolate(polygon, polygon2, 0.5))


# img = np.zeros((10, 10), dtype=np.int32)


def test_draw_polygon(img):
    im = ge.draw_polygon(img, polygon, 1)
    print(im)


def test_extract_polygon(img):
    print(ge.extract_polygon(img, 1))


def tesextract_convex_hull():
    print(ge.extract_convex_hull(polygon))


def test_rdp_simplify_polygon():
    print(ge.rdp_simplify_polygon(polygon, 0))


def test_rdp_simplify_polygon2():
    print(ge.rdp_simplify_polygon2(polygon, 5))


def test_minimum_area_bbox():
    print(ge.minimum_area_bbox(polygon))
    assert ge.minimum_area_bbox(polygon) != ([np.nan, np.nan], 0.0, 0.0, np.nan, np.nan)


def test_translate():
    global img
    img = np.ones((12, 12), dtype=np.float64)
    img = sp.translate(img, 1.2, 1.3, "constant", 0)
    print(img)
    with pytest.raises(RuntimeError):
        _img = np.ones((12, 12, 12), dtype=np.float64)
        _img = sp.translate(_img, 1.2, 1.3, "constant", 0)


def test_gaussian_filter():
    global img
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


@pytest.fixture
def jpegls_encoded(img):
    return sp.jpegls_encode(img)


def test_jpegls_encode(img):
    c = sp.jpegls_encode(img)
    print(len(c))


def test_jpegls_decode(img, jpegls_encoded):
    _img = sp.jpegls_decode(jpegls_encoded, 12, 20)
    assert (_img == img).all()
    print(img)


def test_bad_pixels():
    img = np.ones((20, 12), dtype=np.uint16)
    b = bp.BadPixels(img)
    print(b.correct(img))
    del b


def test_label_image():
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
