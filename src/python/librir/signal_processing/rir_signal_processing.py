import ctypes as ct
import sys

import numpy as np

from ..low_level.misc import _signal_processing, toCharP


def translate(image, dx, dy, strategy=str(), background=None):
    """
    Translate input image by a floating point offset (dx,dy).

    strategy controls the way border pixels are managed.
    If strategy is empty, border pixels are set to the original image ones.
    If strategy is "constant", border pixels are set to the given background value.
    If strategy is "nearest", border pixels are set to closest valid pixels.
    If strategy is "wrap", border pixels are extended by wrapping around to the opposite edge.
    """
    _signal_processing.translate.argtypes = [
        ct.c_int,
        ct.c_void_p,
        ct.c_void_p,
        ct.c_int,
        ct.c_int,
        ct.c_float,
        ct.c_float,
        ct.c_void_p,
        ct.c_char_p,
    ]

    if len(image.shape) != 2:
        raise RuntimeError("translate: wrong input image dimension")
    if strategy == "background" and background is None:
        raise RuntimeError("translate: wrong background value")

    strategy = toCharP(strategy)
    if strategy == b"constant":
        strategy = b"background"
    r = -1
    img = np.copy(image, "C")
    res = np.copy(image, "C")
    src = img.ctypes.data_as(ct.c_void_p)
    dst = res.ctypes.data_as(ct.c_void_p)
    _back = np.zeros((1), dtype=img.dtype)
    if background is not None:
        _back[0] = background
    _tr = np.zeros((2), dtype=np.float32)
    _tr[0] = dx
    _tr[1] = dy
    back = _back.ctypes.data_as(ct.c_void_p)

    if image.dtype == np.bool_:
        r = _signal_processing.translate(
            ord("?"),
            src,
            dst,
            img.shape[1],
            img.shape[0],
            _tr[0],
            _tr[1],
            back,
            strategy,
        )
    elif image.dtype == np.int8:
        r = _signal_processing.translate(
            ord("b"),
            src,
            dst,
            img.shape[1],
            img.shape[0],
            _tr[0],
            _tr[1],
            back,
            strategy,
        )
    elif image.dtype == np.uint8:
        r = _signal_processing.translate(
            ord("B"),
            src,
            dst,
            img.shape[1],
            img.shape[0],
            _tr[0],
            _tr[1],
            back,
            strategy,
        )
    elif image.dtype == np.int16:
        r = _signal_processing.translate(
            ord("h"),
            src,
            dst,
            img.shape[1],
            img.shape[0],
            _tr[0],
            _tr[1],
            back,
            strategy,
        )
    elif image.dtype == np.uint16:
        r = _signal_processing.translate(
            ord("H"),
            src,
            dst,
            img.shape[1],
            img.shape[0],
            _tr[0],
            _tr[1],
            back,
            strategy,
        )
    elif image.dtype == np.int32:
        r = _signal_processing.translate(
            ord("i"),
            src,
            dst,
            img.shape[1],
            img.shape[0],
            _tr[0],
            _tr[1],
            back,
            strategy,
        )
    elif image.dtype == np.uint32:
        r = _signal_processing.translate(
            ord("I"),
            src,
            dst,
            img.shape[1],
            img.shape[0],
            _tr[0],
            _tr[1],
            back,
            strategy,
        )
    elif image.dtype == np.int64:
        r = _signal_processing.translate(
            ord("l"),
            src,
            dst,
            img.shape[1],
            img.shape[0],
            _tr[0],
            _tr[1],
            back,
            strategy,
        )
    elif image.dtype == np.int64:
        r = _signal_processing.translate(
            ord("L"),
            src,
            dst,
            img.shape[1],
            img.shape[0],
            _tr[0],
            _tr[1],
            back,
            strategy,
        )
    elif image.dtype == np.float32:
        r = _signal_processing.translate(
            ord("f"),
            src,
            dst,
            img.shape[1],
            img.shape[0],
            _tr[0],
            _tr[1],
            back,
            strategy,
        )
    elif image.dtype == np.float64:
        r = _signal_processing.translate(
            ord("d"),
            src,
            dst,
            img.shape[1],
            img.shape[0],
            _tr[0],
            _tr[1],
            back,
            strategy,
        )
    else:
        raise RuntimeError("An error occured while calling 'translate'")

    if r < 0:
        raise RuntimeError("An error occured while calling 'translate'")
    return res


def gaussian_filter(image, sigma=1.0):
    """
    Apply a gaussian filter on input image with given sigma value.
    The result image is always of type np.float32.
    """
    _signal_processing.gaussian_filter.argtypes = [
        ct.POINTER(ct.c_float),
        ct.POINTER(ct.c_float),
        ct.c_int,
        ct.c_int,
        ct.c_float,
    ]
    if len(image.shape) != 2:
        raise RuntimeError("gaussian_filter: wrong input image dimension")

    img = np.array(image, dtype=np.float32)
    res = np.zeros(image.shape, dtype=np.float32)

    tmp = _signal_processing.gaussian_filter(
        img.ctypes.data_as(ct.POINTER(ct.c_float)),
        res.ctypes.data_as(ct.POINTER(ct.c_float)),
        img.shape[1],
        img.shape[0],
        sigma,
    )
    if tmp < 0:
        raise RuntimeError("An error occured while calling gaussian_filter")

    return res


def find_median_pixel(image, percent=0.5, mask=None):
    """
    Find the pixel value from which at least percent*image.size pixels are included
    """
    _signal_processing.find_median_pixel.argtypes = [
        ct.POINTER(ct.c_uint16),
        ct.c_int,
        ct.c_float,
    ]
    _signal_processing.find_median_pixel_mask.argtypes = [
        ct.POINTER(ct.c_uint16),
        ct.POINTER(ct.c_uint8),
        ct.c_int,
        ct.c_float,
    ]
    if len(image.shape) != 2:
        raise RuntimeError("find_median_pixel: wrong input image dimension")

    image = image.astype(dtype=np.uint16, copy=False)
    if mask is not None:
        mask = mask.astype(dtype=np.uint8, copy=False)
        res = _signal_processing.find_median_pixel_mask(
            image.ctypes.data_as(ct.POINTER(ct.c_uint16)),
            mask.ctypes.data_as(ct.POINTER(ct.c_uint8)),
            image.size,
            float(percent),
        )
    else:
        res = _signal_processing.find_median_pixel(
            image.ctypes.data_as(ct.POINTER(ct.c_uint16)), image.size, float(percent)
        )
    return res


def extract_times(time_series, strategy="union"):
    """
    Create a unique time vector from several ones.
    time_series is a list of input time series
    strategy is either 'union' (take the union of all time series) or 'inter'
    Returns a growing time vector containing all different time values given in
    time_series, without redundant times.
    """
    _signal_processing.extract_times.argtypes = [
        ct.POINTER(ct.c_double),
        ct.c_int,
        ct.POINTER(ct.c_int),
        ct.c_int,
        ct.POINTER(ct.c_double),
        ct.POINTER(ct.c_int),
    ]
    if len(time_series) == 0:
        raise RuntimeError("extract_times: NULL size")
    if strategy != "union" and strategy != "inter":
        raise RuntimeError("extract_times: wrong strategy")

    times = np.array(time_series[0], dtype=np.float64)
    sizes = np.zeros((len(time_series)), dtype=np.int32)
    sizes[0] = len(time_series[0])
    outsize = np.zeros((1), dtype=np.int32)
    outsize[0] = sizes[0]
    for i in range(1, len(time_series)):
        times = np.hstack((times, np.array(time_series[i], dtype=np.float64)))
        sizes[i] = len(time_series[i])
        outsize[0] += sizes[i]

    s = 0
    if strategy == "inter":
        s = 1

    out = np.zeros((outsize[0]), dtype=np.float64)

    tmp = _signal_processing.extract_times(
        times.ctypes.data_as(ct.POINTER(ct.c_double)),
        len(time_series),
        sizes.ctypes.data_as(ct.POINTER(ct.c_int)),
        s,
        out.ctypes.data_as(ct.POINTER(ct.c_double)),
        outsize.ctypes.data_as(ct.POINTER(ct.c_int)),
    )
    if tmp == -2:
        out = np.zeros((outsize[0]), dtype=np.float64)
        tmp = _signal_processing.extract_times(
            times.ctypes.data_as(ct.POINTER(ct.c_double)),
            len(time_series),
            sizes.ctypes.data_as(ct.POINTER(ct.c_int)),
            s,
            out.ctypes.data_as(ct.POINTER(ct.c_double)),
            outsize.ctypes.data_as(ct.POINTER(ct.c_int)),
        )
    if tmp == -1:
        raise RuntimeError("extract_times: unknown error")
    return out[0 : outsize[0]]


def resample_time_serie(x, y, time_vector, padd=None, interp=True):
    """
    Resample a time serie based on a new time vector
    - x: time vector of the time serie
    - y: values associated to the time serie
    - time_vector: new time vector
    - padd: if not None, padd the output serie with this value at boundaries
    - interp: if True, interpolate values.
    Returns the new y values corresponding to the new time vector.
    """
    if len(x) != len(y) or len(x) == 0:
        raise RuntimeError("resample_time_serie: wrong input serie size")

    if len(time_vector) == 0:
        raise RuntimeError("resample_time_serie: wrong time vector size")

    # format inputs
    interp = bool(interp)
    x = np.array(x, dtype=np.float64)
    y = np.array(y, dtype=np.float64)
    time_vector = np.array(time_vector, dtype=np.float64)
    s = 0
    if padd is not None:
        s |= 2
    if interp:
        s |= 4
    if padd is None:
        padd = 0
    padd = float(padd)

    outsize = np.zeros((1), dtype=np.int32)
    outsize[0] = len(x) * 2
    out = np.zeros((outsize[0]), dtype=np.float64)

    _signal_processing.resample_time_serie.argtypes = [
        ct.POINTER(ct.c_double),
        ct.POINTER(ct.c_double),
        ct.c_int,
        ct.POINTER(ct.c_double),
        ct.c_int,
        ct.c_int,
        ct.c_double,
        ct.POINTER(ct.c_double),
        ct.POINTER(ct.c_int),
    ]

    tmp = _signal_processing.resample_time_serie(
        x.ctypes.data_as(ct.POINTER(ct.c_double)),
        y.ctypes.data_as(ct.POINTER(ct.c_double)),
        len(x),
        time_vector.ctypes.data_as(ct.POINTER(ct.c_double)),
        len(time_vector),
        s,
        padd,
        out.ctypes.data_as(ct.POINTER(ct.c_double)),
        outsize.ctypes.data_as(ct.POINTER(ct.c_int)),
    )

    if tmp == -2:
        out = np.zeros((outsize[0]), dtype=np.float64)
        tmp = _signal_processing.resample_time_serie(
            x.ctypes.data_as(ct.POINTER(ct.c_double)),
            y.ctypes.data_as(ct.POINTER(ct.c_double)),
            len(x),
            time_vector.ctypes.data_as(ct.POINTER(ct.c_double)),
            len(time_vector),
            s,
            padd,
            out.ctypes.data_as(ct.POINTER(ct.c_double)),
            outsize.ctypes.data_as(ct.POINTER(ct.c_int)),
        )
    if tmp == -1:
        raise RuntimeError("resample_time_serie: unknown error")
    return out[0 : outsize[0]]


def bad_pixels_create(first_image):
    """
    Create an object meant to correct bad pixels inside IR videos.
    The list of bad pixels is constructed from the first image.
    Returns the object handle.
    """
    img = np.array(first_image, dtype=np.uint16)
    _signal_processing.bad_pixels_create.argtypes = [
        ct.POINTER(ct.c_ushort),
        ct.c_int,
        ct.c_int,
    ]
    ret = _signal_processing.bad_pixels_create(
        img.ctypes.data_as(ct.POINTER(ct.c_ushort)), img.shape[1], first_image.shape[0]
    )
    return ret


def bad_pixels_destroy(handle):
    """
    Destroy bad pixel object
    """
    _signal_processing.bad_pixels_destroy(handle)


def bad_pixels_correct(handle, img):
    """
    Corrects input image from bad pixels and returns the result.
    """
    img = np.array(img, dtype=np.uint16)
    out = np.zeros(img.shape, dtype=np.uint16)
    _signal_processing.bad_pixels_correct.argtypes = [
        ct.c_int,
        ct.POINTER(ct.c_ushort),
        ct.POINTER(ct.c_ushort),
    ]
    res = _signal_processing.bad_pixels_correct(
        handle,
        img.ctypes.data_as(ct.POINTER(ct.c_ushort)),
        out.ctypes.data_as(ct.POINTER(ct.c_ushort)),
    )
    if res < 0:
        raise RuntimeError("'bad_pixels_correct': unknown error")
    return out


def jpegls_encode(image, error=0):
    """
    Compress image using jpegls format
    """
    _signal_processing.jpegls_encode.argtypes = [
        ct.POINTER(ct.c_uint16),
        ct.c_int,
        ct.c_int,
        ct.c_int,
        ct.POINTER(ct.c_char),
        ct.c_int,
    ]
    image = np.array(image, dtype="H")
    if sys.version_info[0] > 2:
        out = bytes(image.size * 2)
    else:
        out = "\00" * image.size * 2
    tmp = _signal_processing.jpegls_encode(
        image.ctypes.data_as(ct.POINTER(ct.c_uint16)),
        image.shape[1],
        image.shape[0],
        error,
        out,
        len(out),
    )
    if tmp < 0:
        raise RuntimeError("An error occured while calling 'jpegls_encode'")
    out = out[0:tmp]
    return out


def jpegls_decode(buff, width, height):
    """
    Decode a jpegls compressed image
    """
    _signal_processing.jpegls_decode.argtypes = [
        ct.POINTER(ct.c_char),
        ct.c_int,
        ct.POINTER(ct.c_uint16),
        ct.c_int,
        ct.c_int,
    ]
    image = np.zeros((height, width), dtype="H")
    tmp = _signal_processing.jpegls_decode(
        buff, len(buff), image.ctypes.data_as(ct.POINTER(ct.c_uint16)), width, height
    )
    if tmp < 0:
        raise RuntimeError("An error occured while calling 'jpegls_decode'")
    return image


def label_image(image, background_value=0):
    """
    Closed Component Labelling algorithm
    Returns a tuple (image,areas, first_points), each index of the list corresponding
    to the label value.
    The index 0 corresponds to the background, and does not contain meaningful
    information.
    """

    _signal_processing.label_image.argtypes = [
        ct.c_int,
        ct.c_void_p,
        ct.POINTER(ct.c_int),
        ct.c_int,
        ct.c_int,
        ct.c_void_p,
        ct.POINTER(ct.c_double),
        ct.POINTER(ct.c_int),
    ]

    if len(image.shape) != 2:
        raise RuntimeError("translate: wrong input image dimension")

    r = -1
    img = image.astype(
        image.dtype, order="C", casting="unsafe", subok=False, copy=False
    )
    res = np.zeros(image.shape, dtype=np.int32)
    background = np.zeros(1, dtype=image.dtype)
    background[0] = background_value
    areas = np.zeros(img.size, dtype=np.int32)
    xy = np.zeros((img.size, 2), dtype=np.float64)

    src = img.ctypes.data_as(ct.c_void_p)
    dst = res.ctypes.data_as(ct.POINTER(ct.c_int))
    back = background.ctypes.data_as(ct.c_void_p)
    dareas = areas.ctypes.data_as(ct.POINTER(ct.c_int))
    dxy = xy.ctypes.data_as(ct.POINTER(ct.c_double))

    if image.dtype == np.bool_:
        r = _signal_processing.label_image(
            ord("?"), src, dst, img.shape[1], img.shape[0], back, dxy, dareas
        )
    elif image.dtype == np.int8:
        r = _signal_processing.label_image(
            ord("b"), src, dst, img.shape[1], img.shape[0], back, dxy, dareas
        )
    elif image.dtype == np.uint8:
        r = _signal_processing.label_image(
            ord("B"), src, dst, img.shape[1], img.shape[0], back, dxy, dareas
        )
    elif image.dtype == np.int16:
        r = _signal_processing.label_image(
            ord("h"), src, dst, img.shape[1], img.shape[0], back, dxy, dareas
        )
    elif image.dtype == np.uint16:
        r = _signal_processing.label_image(
            ord("H"), src, dst, img.shape[1], img.shape[0], back, dxy, dareas
        )
    elif image.dtype == np.int32:
        r = _signal_processing.label_image(
            ord("i"), src, dst, img.shape[1], img.shape[0], back, dxy, dareas
        )
    elif image.dtype == np.uint32:
        r = _signal_processing.label_image(
            ord("I"), src, dst, img.shape[1], img.shape[0], back, dxy, dareas
        )
    elif image.dtype == np.int64:
        r = _signal_processing.label_image(
            ord("l"), src, dst, img.shape[1], img.shape[0], back, dxy, dareas
        )
    elif image.dtype == np.int64:
        r = _signal_processing.label_image(
            ord("L"), src, dst, img.shape[1], img.shape[0], back, dxy, dareas
        )
    elif image.dtype == np.float32:
        r = _signal_processing.label_image(
            ord("f"), src, dst, img.shape[1], img.shape[0], back, dxy, dareas
        )
    elif image.dtype == np.float64:
        r = _signal_processing.label_image(
            ord("d"), src, dst, img.shape[1], img.shape[0], back, dxy, dareas
        )
    else:
        raise RuntimeError("An error occured while calling 'label_image'")

    if r < 0:
        raise RuntimeError("An error occured while calling 'label_image'")

    areas = areas[0:r]
    xy = xy[0:r]
    return (res, areas, xy)


def keep_largest_area(image, background_value=0, foreground_value=1):
    """
    Returns an image where the largest closed region of input image is set to
    foreground_value,
    the rest to background_value
    """

    _signal_processing.keep_largest_area.argtypes = [
        ct.c_int,
        ct.c_void_p,
        ct.POINTER(ct.c_int),
        ct.c_int,
        ct.c_int,
        ct.c_void_p,
        ct.c_int,
    ]

    if len(image.shape) != 2:
        raise RuntimeError("translate: wrong input image dimension")

    r = -1
    img = image.astype(
        image.dtype, order="C", casting="unsafe", subok=False, copy=False
    )
    res = np.zeros(image.shape, dtype=np.int32)
    background = np.zeros(1, dtype=image.dtype)
    background[0] = background_value

    src = img.ctypes.data_as(ct.c_void_p)
    dst = res.ctypes.data_as(ct.POINTER(ct.c_int))
    back = background.ctypes.data_as(ct.c_void_p)

    if image.dtype == np.bool_:
        r = _signal_processing.keep_largest_area(
            ord("?"), src, dst, img.shape[1], img.shape[0], back, foreground_value
        )
    elif image.dtype == np.int8:
        r = _signal_processing.keep_largest_area(
            ord("b"), src, dst, img.shape[1], img.shape[0], back, foreground_value
        )
    elif image.dtype == np.uint8:
        r = _signal_processing.keep_largest_area(
            ord("B"), src, dst, img.shape[1], img.shape[0], back, foreground_value
        )
    elif image.dtype == np.int16:
        r = _signal_processing.keep_largest_area(
            ord("h"), src, dst, img.shape[1], img.shape[0], back, foreground_value
        )
    elif image.dtype == np.uint16:
        r = _signal_processing.keep_largest_area(
            ord("H"), src, dst, img.shape[1], img.shape[0], back, foreground_value
        )
    elif image.dtype == np.int32:
        r = _signal_processing.keep_largest_area(
            ord("i"), src, dst, img.shape[1], img.shape[0], back, foreground_value
        )
    elif image.dtype == np.uint32:
        r = _signal_processing.keep_largest_area(
            ord("I"), src, dst, img.shape[1], img.shape[0], back, foreground_value
        )
    elif image.dtype == np.int64:
        r = _signal_processing.keep_largest_area(
            ord("l"), src, dst, img.shape[1], img.shape[0], back, foreground_value
        )
    elif image.dtype == np.int64:
        r = _signal_processing.keep_largest_area(
            ord("L"), src, dst, img.shape[1], img.shape[0], back, foreground_value
        )
    elif image.dtype == np.float32:
        r = _signal_processing.keep_largest_area(
            ord("f"), src, dst, img.shape[1], img.shape[0], back, foreground_value
        )
    elif image.dtype == np.float64:
        r = _signal_processing.keep_largest_area(
            ord("d"), src, dst, img.shape[1], img.shape[0], back, foreground_value
        )
    else:
        raise RuntimeError("An error occured while calling 'keep_largest_area'")

    if r < 0:
        raise RuntimeError("An error occured while calling 'keep_largest_area'")

    return res
