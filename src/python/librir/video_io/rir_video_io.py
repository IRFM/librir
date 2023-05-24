import ctypes as ct

import numpy as np

from ..low_level.misc import _video_io, toString, toArray, toBytes


FILE_FORMAT_PCR = 1
FILE_FORMAT_WEST = 2
FILE_FORMAT_PCR_ENCAPSULATED = 3
FILE_FORMAT_ZSTD_COMPRESSED = 4
FILE_FORMAT_H264 = 5


def open_camera_file(filename):
    """
    Open a video file.
    Returns an integer value representing the camera. This value can be used by the functions:
    - close_camera
    - get_camera_identifier
    - get_camera_pulse
    - get_image_count
    - get_image_time
    - get_image_size
    - supported_calibrations
    - load_image

    C signature:
    void * open_camera_file(const char * filename, int * file_format);
    """
    pulse = np.zeros(1, dtype="i")
    res = _video_io.open_camera_file(
        filename.encode(), pulse.ctypes.data_as(ct.POINTER(ct.c_int))
    )
    if res == 0:
        raise RuntimeError("cannot read file " + filename)
    return res


def video_file_format(filename):
    """
    Returns the video file format of given video file
    """
    res = _video_io.video_file_format(str(filename).encode())
    if res == 0:
        raise RuntimeError("cannot open file " + filename)
    return res


def close_camera(camera):
    """
    Close a camera previously opened with open_camera() or open_camera_file().

    C signature:
    int close_camera(void * camera)
    """
    _video_io.close_camera(camera)


def get_camera_identifier(camera):
    """
    Returns a camera unique identifier.

    C signature:
    int get_camera_identifier(void * camera, char * identifier);
    """

    unique_identifier = np.zeros(200, dtype="c")
    u_p = unique_identifier.ctypes.data_as(ct.POINTER(ct.c_char))
    res = _video_io.get_camera_identifier(camera, u_p)
    if res < 0:
        raise RuntimeError("cannot retrieve camera identifier")
    return toString(unique_identifier)


def get_filename(camera):
    """Returns the camera filename (if any)"""
    fname = np.zeros(200, dtype="c")
    u_f = fname.ctypes.data_as(ct.POINTER(ct.c_char))
    res = _video_io.get_filename(camera, u_f)
    if res < 0:
        raise RuntimeError("cannot retrieve camera filename")
    return toString(fname)


def get_image_count(camera):
    """
    Returns the number of image for given camera.

    C signature:
    int get_image_count(void * camera)
    """
    return _video_io.get_image_count(camera)


def get_image_time(camera, pos):
    """
    Returns the image timestamp at position 'pos' for given camera.

    C signature:
    int get_image_time(void * camera, int pos, int64_t * time)
    """
    time = np.zeros(1, dtype=np.int64)
    res = _video_io.get_image_time(
        camera, pos, time.ctypes.data_as(ct.POINTER(ct.c_longlong))
    )
    if res < 0:
        raise RuntimeError("cannot retrieve time for position " + str(pos))
    return time[0]


def get_image_size(camera):
    """
    Returns the tuple (image_height, image_width) for given camera.

    C signature:
    int get_image_size(void * camera, int * width, int * height);
    """
    width = np.zeros(1, dtype=np.int32)
    height = np.zeros(1, dtype=np.int32)
    res = _video_io.get_image_size(
        camera,
        width.ctypes.data_as(ct.POINTER(ct.c_int)),
        height.ctypes.data_as(ct.POINTER(ct.c_int)),
    )
    if res < 0:
        raise RuntimeError("cannot retrieve camera size")
    return height[0], width[0]


def supported_calibrations(camera):
    """
    Returns a list of supported calibrations for given camera.
    Most cameras will return ["Digital Level","Apparent T(C)"].

    C signature:
    int supported_calibrations(void * camera, int * count);
    int calibration_name(void * camera, int calibration, char * name);
    """
    count = np.zeros(1, dtype=np.int32)
    res = _video_io.supported_calibrations(
        camera, count.ctypes.data_as(ct.POINTER(ct.c_int))
    )
    if res < 0:
        raise RuntimeError("cannot retrieve camera calibrations")

    res = []
    for i in range(count[0]):
        name = np.zeros(100, dtype="c")
        tmp = _video_io.calibration_name(
            camera, i, name.ctypes.data_as(ct.POINTER(ct.c_char))
        )
        if tmp < 0:
            raise RuntimeError("cannot retrieve camera calibrations")
        res.append(toString(name))

    return res


def load_image(camera, pos, calibration):
    """
    Returns the image at position 'pos' for given camera.
    The image is calibrated depending on the 'calibration' integer value.
    'calibration' is the index of the calibration method (usually 0 or 1)
    in the list of calibrations as returned by supported_calibrations().

    To get an IR image in digital levels (raw data), use calibration = 0.
    To get an IR image in apparent temperature (celsius), use calibration = 1.

    C signature:
    int load_image(void * camera, int pos, int calibration, unsigned short * pixels);
    """
    size = get_image_size(camera)
    pixels = np.zeros(size, dtype=np.ushort)

    res = _video_io.load_image(
        camera,
        ct.c_int32(pos),
        calibration,
        pixels.ctypes.data_as(ct.POINTER(ct.c_ushort)),
    )
    if res < 0:
        raise RuntimeError(
            "cannot retrieve camera image for position "
            + str(pos)
            + " and calibration "
            + str(calibration)
        )

    return pixels


def set_global_emissivity(camera, emi_value):
    """Set the global scene emissivity for given camera file"""
    _video_io.set_global_emissivity.argtypes = [ct.c_int, ct.c_float]
    res = _video_io.set_global_emissivity(camera, emi_value)
    if res < 0:
        raise RuntimeError(
            "An error occured while calling 'set_global_emissivity' with emissivity "
            + str(emi_value)
        )


def get_global_emissivity(camera):
    """Returns the emissivity map for given camera"""
    pixels = np.zeros(1, dtype=np.float32)
    _video_io.get_emissivity.argtypes = [ct.c_int, ct.POINTER(ct.c_float), ct.c_int]
    res = _video_io.get_emissivity(
        camera, pixels.ctypes.data_as(ct.POINTER(ct.c_float)), 1
    )
    if res < 0:
        raise RuntimeError("An error occured while calling 'get_emissivity'")
    return pixels[0]


def set_emissivity(camera, emissivity_array):
    """Set the scene emissivity per pixel for given camera file"""
    ar = np.array(emissivity_array, dtype=np.float)
    res = _video_io.set_emissivity(
        camera, ar.ctypes.data_as(ct.POINTER(ct.c_float)), ar.size
    )
    if res < 0:
        raise RuntimeError("An error occured while calling 'set_emissivity'")


def get_emissivity(camera):
    """Returns the emissivity map for given camera"""
    size = get_image_size(camera)
    pixels = np.zeros(size, dtype=np.float32)
    _video_io.get_emissivity.argtypes = [ct.c_int, ct.POINTER(ct.c_float), ct.c_int]
    s = pixels.size
    ptr = pixels.ctypes.data_as(ct.POINTER(ct.c_float))
    res = _video_io.get_emissivity(camera, ptr, int(s))
    if res < 0:
        raise RuntimeError("An error occured while calling 'get_emissivity'")
    return pixels


def support_emissivity(camera):
    """Returns True if given camera supports setting a custom emissivity value, False otherwise"""
    res = _video_io.support_emissivity(int(camera))
    if res == 1:
        return True
    return False


def camera_saturate(camera):
    """
    Returns true if setting the emissivity saturates the temperature calibration for the last call
    to load_image, False otherwise.
    """
    res = _video_io.camera_saturate(int(camera))
    if res == 1:
        return True
    return False


def set_optical_temperature(camera, temperature):
    """
    Set the optical temperature for given camera in degree Celsius.
    This should be the temperature of the B30.
    Not all cameras supoort this feature. Use support_optical_temperature() function to test it.
    """
    _video_io.set_optical_temperature.argtypes = [ct.c_int, ct.c_uint16]
    res = _video_io.set_optical_temperature(int(camera), np.ushort(temperature))
    if res < 0:
        raise RuntimeError("An error occured while calling 'set_optical_temperature'")


def get_optical_temperature(camera):
    """Returns the optical temperature degree Celsius for given camera"""
    res = _video_io.get_optical_temperature(camera)
    return res


def set_STEFI_temperature(camera, temperature):
    """
    Set the STEFI temperature for given camera in degree Celsius.
    Not all cameras supoort this feature. Use support_optical_temperature() function to test it.
    """
    _video_io.set_STEFI_temperature.argtypes = [ct.c_int, ct.c_uint16]
    res = _video_io.set_STEFI_temperature(int(camera), np.ushort(temperature))
    if res < 0:
        raise RuntimeError("An error occured while calling 'set_STEFI_temperature'")


def get_STEFI_temperature(camera):
    """Returns the STEFI temperature degree Celsius for given camera"""
    res = _video_io.get_STEFI_temperature(camera)
    return res


def support_optical_temperature(camera):
    """Returns True if given camera supports custom optical temperature; False otherwise"""
    res = _video_io.support_optical_temperature(camera)
    if res == 0:
        return False
    return True


def enable_bad_pixels(camera, enable=True):
    """
    Returns 0 if given camera supports bad pixels treatement
    """
    _video_io.enable_bad_pixels(camera, enable)
    # if res < 0:
    #    raise RuntimeError("An error occured while calling 'enable_bad_pixels'")


def calibrate_image(camera, image, calib):
    """
    Apply given calibration to a DL image and returns the result.
    """
    if image.dtype == "H":
        img = np.copy(image, "C")
    else:
        img = np.array(image, dtype="H")
    tmp = _video_io.calibrate_inplace(
        camera, img.ctypes.data_as(ct.POINTER(ct.c_uint16)), img.size, calib
    )
    if tmp < 0:
        raise RuntimeError("calibrate_image: unable to apply selected calibration")
    return img


def flip_camera_calibration(camera, flip_rl, flip_ud):
    """
    For given camera, flip the calibration files
    """
    ret = _video_io.flip_camera_calibration(int(camera), int(flip_rl), int(flip_ud))
    if ret < 0:
        raise RuntimeError("An error occured while calling 'flip_camera_calibration'")


def calibration_files(camera):
    """
    Returns the calibration file names for given camera.
    """
    dst = np.zeros((100), dtype="c")
    dstSize = np.zeros((1), dtype="i")
    dstSize[0] = 100
    ret = _video_io.calibration_files(
        camera,
        dst.ctypes.data_as(ct.POINTER(ct.c_char)),
        dstSize.ctypes.data_as(ct.POINTER(ct.c_int)),
    )
    if ret == -2:
        dst = np.zeros((dstSize[0]), dtype="c")
        ret = _video_io.calibration_files(
            camera,
            dst.ctypes.data_as(ct.POINTER(ct.c_char)),
            dstSize.ctypes.data_as(ct.POINTER(ct.c_int)),
        )
    if ret == -1:
        raise RuntimeError("An error occured while calling 'calibration_files'")
    cfiles = toString(dst).split("\n")
    return cfiles


def get_attributes(camera):
    """
    Returns attributes for the last read image
    """
    count = _video_io.get_attribute_count(camera)
    if count <= 0:
        return dict()
    res = dict()
    for i in range(count):
        klen = np.zeros((1), dtype="i")
        klen[0] = 200
        vlen = np.zeros((1), dtype="i")
        vlen[0] = 200
        key = np.zeros((200), dtype="c")
        value = np.zeros((200), dtype="c")
        tmp = _video_io.get_attribute(
            camera,
            i,
            key.ctypes.data_as(ct.POINTER(ct.c_char)),
            klen.ctypes.data_as(ct.POINTER(ct.c_int32)),
            value.ctypes.data_as(ct.POINTER(ct.c_char)),
            vlen.ctypes.data_as(ct.POINTER(ct.c_int32)),
        )
        if tmp == -2:
            key = np.zeros((klen[0]), dtype="c")
            value = np.zeros((vlen[0]), dtype="c")
            tmp = _video_io.get_attribute(
                camera,
                i,
                key.ctypes.data_as(ct.POINTER(ct.c_char)),
                klen.ctypes.data_as(ct.POINTER(ct.c_int32)),
                value.ctypes.data_as(ct.POINTER(ct.c_char)),
                vlen.ctypes.data_as(ct.POINTER(ct.c_int32)),
            )
        if tmp < 0:
            raise RuntimeError("An error occured while calling 'get_attributes'")
        res[toString(key)] = toBytes(value)[0 : vlen[0]]

    return res


def get_global_attributes(camera):
    """
    Returns global attributes for given camera
    """
    count = _video_io.get_global_attribute_count(camera)
    if count <= 0:
        return dict()
    res = dict()
    for i in range(count):
        klen = np.zeros((1), dtype="i")
        klen[0] = 200
        vlen = np.zeros((1), dtype="i")
        vlen[0] = 200
        key = np.zeros((200), dtype="c")
        value = np.zeros((200), dtype="c")
        tmp = _video_io.get_global_attribute(
            camera,
            i,
            key.ctypes.data_as(ct.POINTER(ct.c_char)),
            klen.ctypes.data_as(ct.POINTER(ct.c_int32)),
            value.ctypes.data_as(ct.POINTER(ct.c_char)),
            vlen.ctypes.data_as(ct.POINTER(ct.c_int32)),
        )
        if tmp == -2:
            key = np.zeros((klen[0]), dtype="c")
            value = np.zeros((vlen[0]), dtype="c")
            tmp = _video_io.get_global_attribute(
                camera,
                i,
                key.ctypes.data_as(ct.POINTER(ct.c_char)),
                klen.ctypes.data_as(ct.POINTER(ct.c_int32)),
                value.ctypes.data_as(ct.POINTER(ct.c_char)),
                vlen.ctypes.data_as(ct.POINTER(ct.c_int32)),
            )
        if tmp < 0:
            raise RuntimeError("An error occured while calling 'get_global_attributes'")
        # print(toString(key),len(value),len(bytes(value)))
        res[toString(key)] = toBytes(value)[0 : vlen[0]]

    return res


def h264_open_file(filename, width, height, lossy_height=None):
    """
    Open output video file with given width and height.
    In case of lossy compression, lossy_height  controls where the loss stops (in order to keep the last rows lossless).
    Returns file identifier on success.
    """
    _video_io.h264_open_file.argtypes = [ct.c_char_p, ct.c_int, ct.c_int, ct.c_int]
    if lossy_height is None:
        lossy_height = int(height)
    tmp = _video_io.h264_open_file(
        str(filename).encode("ascii"), int(width), int(height), int(lossy_height)
    )
    if tmp < 0:
        raise RuntimeError("An error occured while calling 'h264_open_file'")
    return tmp


def h264_close_file(saver):
    """
    Close h264 video saver
    """
    _video_io.h264_close_file(saver)


def h264_set_parameter(saver, param, value):
    """
    Set a compression parameter. Supported values:
        - compressionLevel: h264 compression level (0 for very fast, 8 for maximum compression)
        - lowValueError: for lossy compression, maximum error on temperature values < background
        - highValueError: for lossy compression, maximum error on temperature values >= background
        - autoUpdatePixelInterval: for lossy compression, force update each pixel every X frames
    """
    tmp = _video_io.h264_set_parameter(
        saver, str(param).encode("ascii"), str(value).encode("ascii")
    )
    if tmp < 0:
        raise RuntimeError("An error occured while calling 'h264_set_parameter'")


def h264_set_global_attributes(saver, attributes):
    """
    Set file global attributes.
    attributes should be a dictionary of str -> str.
    """
    keys = bytes()
    values = bytes()
    klens = []
    vlens = []
    for k, v in attributes.items():
        if type(k) == bytes:
            ks = k
        elif type(k) == str:
            ks = k.encode("ascii")
        else:
            ks = str(k).encode("ascii")
        if type(v) == bytes:
            vs = v
        elif type(v) == str:
            vs = v.encode("ascii")
        else:
            vs = str(v).encode("ascii")
        klens.append(len(ks))
        vlens.append(len(vs))
        keys += ks
        values += vs

    _keys = toArray(keys)
    _values = toArray(values)
    _klens = np.array(klens, dtype="i")
    _vlens = np.array(vlens, dtype="i")
    tmp = _video_io.h264_set_global_attributes(
        saver,
        len(attributes),
        _keys.ctypes.data_as(ct.POINTER(ct.c_char)),
        _klens.ctypes.data_as(ct.POINTER(ct.c_int32)),
        _values.ctypes.data_as(ct.POINTER(ct.c_char)),
        _vlens.ctypes.data_as(ct.POINTER(ct.c_int32)),
    )
    if tmp < 0:
        raise RuntimeError(
            "An error occured while calling 'h264_set_global_attributes'"
        )


def h264_add_image_lossless(saver, image, timestamp, attributes=None):
    """
    Add and compress (lossless) an image to the file.
    """

    if attributes is None:
        attributes = {}

    _video_io.h264_add_image_lossless.argtypes = [
        ct.c_int,
        ct.POINTER(ct.c_uint16),
        ct.c_longlong,
        ct.c_int,
        ct.POINTER(ct.c_char),
        ct.POINTER(ct.c_int),
        ct.POINTER(ct.c_char),
        ct.POINTER(ct.c_int),
    ]
    timestamp = np.int64(timestamp)
    keys = str()
    values = str()
    klens = []
    vlens = []
    for k, v in attributes.items():
        ks = str(k)
        vs = str(v)
        klens.append(len(ks))
        vlens.append(len(vs))
        keys += ks
        values += vs

    _keys = toArray(keys)
    _values = toArray(values)
    _klens = np.array(klens, dtype="i")
    _vlens = np.array(vlens, dtype="i")
    image = np.array(image, dtype="H")
    tmp = _video_io.h264_add_image_lossless(
        saver,
        image.ctypes.data_as(ct.POINTER(ct.c_uint16)),
        timestamp,
        len(attributes),
        _keys.ctypes.data_as(ct.POINTER(ct.c_char)),
        _klens.ctypes.data_as(ct.POINTER(ct.c_int32)),
        _values.ctypes.data_as(ct.POINTER(ct.c_char)),
        _vlens.ctypes.data_as(ct.POINTER(ct.c_int32)),
    )
    if tmp < 0:
        raise RuntimeError("An error occured while calling 'h264_add_image_lossless'")


def h264_add_image_lossy(saver, image_DL, timestamp, attributes=None):
    """
    Add and compress (lossy) an image to the file.
    """

    if attributes is None:
        attributes = {}

    _video_io.h264_add_image_lossy.argtypes = [
        ct.c_int,
        ct.POINTER(ct.c_uint16),
        ct.c_longlong,
        ct.c_int,
        ct.POINTER(ct.c_char),
        ct.POINTER(ct.c_int),
        ct.POINTER(ct.c_char),
        ct.POINTER(ct.c_int),
    ]
    timestamp = np.int64(timestamp)
    keys = str()
    values = str()
    klens = []
    vlens = []
    for k, v in attributes.items():
        ks = str(k)
        vs = str(v)
        klens.append(len(ks))
        vlens.append(len(vs))
        keys += ks
        values += vs

    _keys = toArray(keys)
    _values = toArray(values)
    _klens = np.array(klens, dtype="i")
    _vlens = np.array(vlens, dtype="i")
    image_DL = np.array(image_DL, dtype="H")
    # image_T = np.array(image_T,dtype='H')
    tmp = _video_io.h264_add_image_lossy(
        saver,
        image_DL.ctypes.data_as(ct.POINTER(ct.c_uint16)),
        timestamp,
        len(attributes),
        _keys.ctypes.data_as(ct.POINTER(ct.c_char)),
        _klens.ctypes.data_as(ct.POINTER(ct.c_int)),
        _values.ctypes.data_as(ct.POINTER(ct.c_char)),
        _vlens.ctypes.data_as(ct.POINTER(ct.c_int)),
    )
    if tmp < 0:
        raise RuntimeError("An error occured while calling 'h264_add_image_lossless'")


def h264_add_loss(saver, image):
    """
    Add and compress (lossy) an image to the file.
    """

    _video_io.h264_add_loss.argtypes = [ct.c_int, ct.POINTER(ct.c_uint16)]
    image = np.array(image, dtype="H")
    # image_T = np.array(image_T,dtype='H')
    tmp = _video_io.h264_add_loss(saver, image.ctypes.data_as(ct.POINTER(ct.c_uint16)))
    if tmp < 0:
        raise RuntimeError("An error occured while calling 'h264_add_loss'")
    return image


def h264_get_low_errors(saver):
    """
    Returns the low error vector for last saved movie

    """
    _video_io.h264_get_low_erros.argtypes = [
        ct.c_int,
        ct.POINTER(ct.c_uint16),
        ct.POINTER(ct.c_int),
    ]
    err = np.zeros((1000), dtype=np.uint16)
    size = np.zeros((1), dtype=np.int32)
    size[0] = 1000
    res = _video_io.h264_get_low_errors(
        saver,
        err.ctypes.data_as(ct.POINTER(ct.c_uint16)),
        size.ctypes.data_as(ct.POINTER(ct.c_int)),
    )
    if res == -2:
        err = np.zeros((size[0]), dtype=np.uint16)
        res = _video_io.h264_get_low_errors(
            saver,
            err.ctypes.data_as(ct.POINTER(ct.c_uint16)),
            size.ctypes.data_as(ct.POINTER(ct.c_int)),
        )
    if res < 0:
        raise RuntimeError("An error occured while calling 'h264_get_low_erros'")
    return err[0 : size[0]]


def h264_get_high_errors(saver):
    """
    Returns the high error vector for last saved movie

    """
    _video_io.h264_get_high_erros.argtypes = [
        ct.c_int,
        ct.POINTER(ct.c_uint16),
        ct.POINTER(ct.c_int),
    ]
    err = np.zeros((1000), dtype=np.uint16)
    size = np.zeros((1), dtype=np.int32)
    size[0] = 1000
    res = _video_io.h264_get_high_errors(
        saver,
        err.ctypes.data_as(ct.POINTER(ct.c_uint16)),
        size.ctypes.data_as(ct.POINTER(ct.c_int)),
    )
    if res == -2:
        err = np.zeros((size[0]), dtype=np.uint16)
        res = _video_io.h264_get_high_errors(
            saver,
            err.ctypes.data_as(ct.POINTER(ct.c_uint16)),
            size.ctypes.data_as(ct.POINTER(ct.c_int)),
        )
    if res < 0:
        raise RuntimeError("An error occured while calling 'h264_get_high_erros'")
    return err[0 : size[0]]


def correct_PCR_file(filename, width, height, frequency):
    """
    Attempt to correct an ill-formed PCR video file by rewriting the file header.
    """

    with open(filename, "rb") as f:
        data = f.read(1024)
        header = np.frombuffer(data, dtype=np.uint32)

    new_header = header.copy()
    new_header[2] = width
    new_header[3] = height
    new_header[5] = 16  # bits
    new_header[7] = frequency
    new_header[8] = 1
    new_header[9] = height * width * 2
    new_header[10] = width
    new_header[11] = height

    with open(filename, "r+b") as f:
        f.write(new_header)

    with open(filename, "rb") as f:
        data = f.read(1024)
        header_verif = np.frombuffer(data, dtype=np.uint32)

    assert header_verif[2] == width
    assert header_verif[3] == height
    assert header_verif[5] == 16  # bits
    assert header_verif[7] == frequency
    assert header_verif[8] == 1
    assert header_verif[9] == height * width * 2
    assert header_verif[10] == width
    assert header_verif[11] == height

    # res = _video_io.correct_PCR_file(filename.encode(),int(width), int(height), int(frequency))
    # if res < 0:
    #     raise RuntimeError("'correct_PCR_file': unknown error")


def bzstd_open_file(filename, width, height, rate, method, clevel):
    """
    Open output video file compressed using zstd and blosc, with given image width and height, frame rate, comrpession method and compression level.
        method == 1 means ZSTD standard compression (clevel goes from 0 to 22),
        method == 2 means blosc+ZSTD standard compression (clevel goes from 1 to 10),
        method == 3 means blosc+ZSTD advanced compression (clevel goes from 1 to 10).
    Returns file identifier on success.
    The bzstd format is provided for tests only and should not be used (prefer the h264 format instead)
    """
    _video_io.open_video_write.argtypes = [
        ct.c_char_p,
        ct.c_int,
        ct.c_int,
        ct.c_int,
        ct.c_int,
        ct.c_int,
    ]
    tmp = _video_io.open_video_write(
        str(filename).encode("ascii"),
        int(width),
        int(height),
        int(rate),
        int(method),
        int(clevel),
    )
    if tmp < 0:
        raise RuntimeError("An error occured while calling 'open_video_write'")
    return tmp


def bzstd_close_file(saver):
    """
    Close video saver
    """
    _video_io.close_video(saver)


def bzstd_add_image(saver, image, timestamp):
    """
    Add and compress (lossless) an image to the bzstd file.
    """
    _video_io.image_write.argtypes = [ct.c_int, ct.POINTER(ct.c_uint16), ct.c_longlong]
    timestamp = np.int64(timestamp)
    image = np.array(image, dtype="H")
    tmp = _video_io.image_write(
        saver, image.ctypes.data_as(ct.POINTER(ct.c_uint16)), timestamp
    )
    if tmp < 0:
        raise RuntimeError("An error occured while calling 'h264_add_image_lossless'")


def load_motion_correction_file(cam, filename):
    """
    Load registration file for given camera
    """
    _video_io.load_motion_correction_file.argtypes = [ct.c_int, ct.c_char_p]

    tmp = _video_io.load_motion_correction_file(int(cam), str(filename).encode("ascii"))
    if tmp < 0:
        raise RuntimeError(
            "An error occured while calling 'load_motion_correction_file'"
        )
    return tmp


def enable_motion_correction(cam, enable):
    """
    Enable/disable registration for given camera
    """
    _video_io.enable_motion_correction.argtypes = [ct.c_int, ct.c_int]

    tmp = _video_io.enable_motion_correction(int(cam), int(enable))
    if tmp < 0:
        raise RuntimeError("An error occured while calling 'enable_motion_correction'")
    return tmp


def motion_correction_enabled(cam):
    """
    Returns True if registration is enabled for given video, False otherwise
    """
    _video_io.motion_correction_enabled.argtypes = [ct.c_int]

    tmp = _video_io.motion_correction_enabled(int(cam))
    if tmp < 0:
        raise RuntimeError("An error occured while calling 'motion_correction_enabled'")
    if tmp == 0:
        return False
    return True
