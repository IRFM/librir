import numpy as np

import librir.geometry as ge
import librir.signal_processing as sp
import librir.signal_processing.BadPixels as bp
import librir.video_io.IRMovie as movie
import librir.video_io.IRSaver as saver
import librir.west as west
import librir.west.WESTIRMovie as w
from librir import tools as rts


def test_temp_directory_access():
    print(rts.get_temp_directory())


def test_get_default_temp_directory():
    print(rts.get_default_temp_directory())


def test_set_temp_directory():
    print(rts.set_temp_directory(rts.get_default_temp_directory()))


def test_zstd_compress_bound():
    print(rts.zstd_compress_bound(100))


def test_zstd_compress():
    global c
    c = rts.zstd_compress(b"toto")
    print(len(c))


def test_zstd_decompress():
    print(rts.zstd_decompress(c))


def test_blosc_compress_zstd():
    global c
    c = rts.blosc_compress_zstd(b"toto", 2, rts.BLOSC_SHUFFLE, 1)
    print(len(c))


def test_blosc_decompress_zstd():
    print(rts.blosc_decompress_zstd(c))


def test_file_attributes():
    import librir.tools.FileAttributes as fa

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


img = np.zeros((10, 10), dtype=np.int32)


def test_draw_polygon():
    im = ge.draw_polygon(img, polygon, 1)
    print(im)


def test_extract_polygon():
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


def test_jpegls_encode():
    global c
    img = np.ones((20, 12), dtype=np.uint16)
    c = sp.jpegls_encode(img)
    print(len(c))


def test_jpegls_decode():
    global c
    img = sp.jpegls_decode(c, 12, 20)
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
    s = saver.IRSaver("test.h264", 20, 20, 20, 8)
    s.add_image(img0, 0)
    s.add_image(img1, 1)
    s.close()

    m = movie.IRMovie.from_filename("test.h264")
    print(m.images)
    print(m.image_size)
    print(m.timestamps)
    print(m.calibrations)
    print(m.filename)
    print(m.calibration_files)
    print(m[0])
    print(m[1])


def test_pulse_infos():
    print(west.get_camera_infos(56927))
    print(west.get_views(56927))
    print(west.ts_exists(55210, "SMAG_IP"))
    print(west.ts_date(56927))
    print(west.ts_last_pulse())
    print(west.ts_read_signal(55210, "SMAG_IP"))


def test_west_ir_movie():
    m = w.WESTIRMovie(56927, "WA")
    print(m.images)
    print(m.image_size)
    print(m.timestamps)
    print(m.calibrations)
    print(m.filename)
    print(m.calibration_files)
    print(m.rois)
