import ctypes as ct
import os
import shutil
import sys

import numpy as np

from ..low_level.misc import _tools, createZeroArrayHandle, toString, toCharP, toBytes, toArray

BLOSC_NOSHUFFLE = 0
BLOSC_SHUFFLE = 1
BLOSC_BITSHUFFLE = 2


def zstd_compress_bound(size):
    """
    Zstd interface.
    Returns the highest compressed size for given input data size.
    """
    _tools.zstd_compress_bound.argtypes = [ct.c_longlong]
    return _tools.zstd_compress_bound(size)


def zstd_compress(src, level=0):
    """
    Zstd interface.
    Compress a bytes object and return the result.
    """
    _tools.zstd_compress.argtypes = [
        ct.POINTER(ct.c_char),
        ct.c_longlong,
        ct.POINTER(ct.c_char),
        ct.c_longlong,
        ct.c_int,
    ]

    src = bytes(src)

    outsize = zstd_compress_bound(len(src))

    if sys.version_info[0] > 2:
        out = bytes(outsize)
    else:
        out = "\00" * outsize
    ret = _tools.zstd_compress(src, len(src), out, outsize, level)
    if ret < 0:
        raise RuntimeError("'zstd_compress': unknown error")
    return out[0:ret]


def zstd_decompress(src):
    """
    Zstd interface.
    Decompress a bytes object (previously compressed with zstd_compress) and return the result.
    """
    _tools.zstd_decompress.argtypes = [
        ct.POINTER(ct.c_char),
        ct.c_longlong,
        ct.POINTER(ct.c_char),
        ct.c_longlong,
    ]
    _tools.zstd_decompress_bound.argtypes = [ct.POINTER(ct.c_char), ct.c_longlong]

    src = bytes(src)

    outsize = _tools.zstd_decompress_bound(src, len(src))
    if outsize < 0:
        raise RuntimeError("'zstd_decompress': wrong input format")

    if sys.version_info[0] > 2:
        out = bytes(outsize)
    else:
        out = "\00" * outsize

    ret = _tools.zstd_decompress(src, len(src), out, outsize)
    if ret < 0:
        raise RuntimeError("'zstd_decompress': unknown error")
    return out[0:ret]


def blosc_compress_zstd(src, typesize, shuffle, level=1):
    """
    Zstd interface.
    Compress a bytes object and return the result.
    """
    _tools.blosc_compress_zstd.argtypes = [
        ct.POINTER(ct.c_char),
        ct.c_longlong,
        ct.POINTER(ct.c_char),
        ct.c_longlong,
        ct.c_int,
        ct.c_int,
        ct.c_int,
    ]

    src = bytes(src)

    outsize = zstd_compress_bound(len(src))

    if sys.version_info[0] > 2:
        out = bytes(outsize)
    else:
        out = "\00" * outsize
    ret = _tools.blosc_compress_zstd(
        src, len(src), out, outsize, int(typesize), int(shuffle), level
    )
    if ret < 0:
        raise RuntimeError("'blosc_compress_zstd': unknown error")
    return out[0:ret]


def blosc_decompress_zstd(src):
    """
    Zstd interface.
    Decompress a bytes object (previously compressed with zstd_compress) and return the result.
    """
    _tools.blosc_decompress_zstd.argtypes = [
        ct.POINTER(ct.c_char),
        ct.c_longlong,
        ct.POINTER(ct.c_char),
        ct.c_longlong,
    ]
    _tools.zstd_decompress_bound.argtypes = [ct.POINTER(ct.c_char), ct.c_longlong]

    src = bytes(src)

    outsize = _tools.zstd_decompress_bound(src, len(src)) + 64
    if outsize < 0:
        raise RuntimeError("'blosc_decompress_zstd': wrong input format")

    if sys.version_info[0] > 2:
        out = bytes(outsize)
    else:
        out = "\00" * outsize

    ret = _tools.blosc_decompress_zstd(src, len(src), out, outsize)
    if ret < 0:
        raise RuntimeError("'zstd_decompress': unknown error")
    return out[0:ret]


def attrs_open_file(filename):
    """
    Open attribute file and returns a handle to it
    """
    _tools.attrs_open_file.argtypes = [ct.c_char_p]
    tmp = _tools.attrs_open_file(str(filename).encode("ascii"))
    if tmp < 0:
        raise RuntimeError("An error occured while calling 'attrs_open_file'")
    return tmp


def attrs_close(handle):
    """
    Close attribute file and write remaining data
    """
    _tools.attrs_close.argtypes = [ct.c_int]
    _tools.attrs_close(int(handle))


def attrs_discard(handle):
    """
    Discard all changes that were not flushed yet
    """
    _tools.attrs_discard.argtypes = [ct.c_int]
    _tools.attrs_discard(int(handle))


def attrs_flush(handle):
    """
    Flush all changes
    """
    _tools.attrs_flush.argtypes = [ct.c_int]
    tmp = _tools.attrs_flush(int(handle))
    if tmp < 0:
        raise RuntimeError("An error occured while calling 'attrs_flush'")


def attrs_image_count(handle):
    _tools.attrs_image_count.argtypes = [ct.c_int]
    tmp = _tools.attrs_image_count(int(handle))
    if tmp < 0:
        raise RuntimeError("An error occured while calling 'attrs_image_count'")
    return tmp


def attrs_global_attribute_count(handle):
    _tools.attrs_global_attribute_count.argtypes = [ct.c_int]
    tmp = _tools.attrs_global_attribute_count(int(handle))
    if tmp < 0:
        raise RuntimeError(
            "An error occured while calling 'attrs_global_attribute_count'"
        )
    return tmp


def attrs_frame_attribute_count(handle, pos):
    _tools.attrs_frame_attribute_count.argtypes = [ct.c_int, ct.c_int]
    tmp = _tools.attrs_frame_attribute_count(int(handle), int(pos))
    if tmp < 0:
        raise RuntimeError(
            "An error occured while calling 'attrs_frame_attribute_count'"
        )
    return tmp


def attrs_global_attribute_name(handle, index):
    _tools.attrs_global_attribute_name.argtypes = [
        ct.c_int,
        ct.c_int,
        ct.c_char_p,
        ct.POINTER(ct.c_int),
    ]
    klen = np.zeros((1), dtype="i")
    key = np.zeros((200), dtype="c")
    tmp = _tools.attrs_global_attribute_name(
        handle,
        index,
        key.ctypes.data_as(ct.POINTER(ct.c_char)),
        klen.ctypes.data_as(ct.POINTER(ct.c_int32)),
    )
    if tmp == -2:
        key = np.zeros((klen[0]), dtype="c")
        tmp = _tools.attrs_global_attribute_name(
            handle,
            index,
            key.ctypes.data_as(ct.POINTER(ct.c_char)),
            klen.ctypes.data_as(ct.POINTER(ct.c_int32)),
        )
        if tmp < 0:
            raise RuntimeError(
                "An error occured while calling 'attrs_global_attribute_name'"
            )
    return toString(key)


def attrs_global_attribute_value(handle, index):
    _tools.attrs_global_attribute_value.argtypes = [
        ct.c_int,
        ct.c_int,
        ct.c_char_p,
        ct.POINTER(ct.c_int),
    ]
    klen = np.zeros((1), dtype="i")
    key = np.zeros((200), dtype="c")
    tmp = _tools.attrs_global_attribute_value(
        handle,
        index,
        key.ctypes.data_as(ct.POINTER(ct.c_char)),
        klen.ctypes.data_as(ct.POINTER(ct.c_int32)),
    )
    if tmp == -2:
        key = np.zeros((klen[0]), dtype="c")
        tmp = _tools.attrs_global_attribute_value(
            handle,
            index,
            key.ctypes.data_as(ct.POINTER(ct.c_char)),
            klen.ctypes.data_as(ct.POINTER(ct.c_int32)),
        )
        if tmp < 0:
            raise RuntimeError(
                "An error occured while calling 'attrs_global_attribute_value'"
            )
    return toBytes(key)


def attrs_frame_attribute_name(handle, frame, index):
    _tools.attrs_frame_attribute_name.argtypes = [
        ct.c_int,
        ct.c_int,
        ct.c_int,
        ct.c_char_p,
        ct.POINTER(ct.c_int),
    ]
    klen = np.zeros((1), dtype="i")
    key = np.zeros((200), dtype="c")
    tmp = _tools.attrs_frame_attribute_name(
        handle,
        frame,
        index,
        key.ctypes.data_as(ct.POINTER(ct.c_char)),
        klen.ctypes.data_as(ct.POINTER(ct.c_int32)),
    )
    if tmp == -2:
        key = np.zeros((klen[0]), dtype="c")
        tmp = _tools.attrs_frame_attribute_name(
            handle,
            frame,
            index,
            key.ctypes.data_as(ct.POINTER(ct.c_char)),
            klen.ctypes.data_as(ct.POINTER(ct.c_int32)),
        )
        if tmp < 0:
            raise RuntimeError(
                "An error occured while calling 'attrs_frame_attribute_name'"
            )
    return toString(key)


def attrs_frame_attribute_value(handle, frame, index):
    _tools.attrs_frame_attribute_value.argtypes = [
        ct.c_int,
        ct.c_int,
        ct.c_int,
        ct.c_char_p,
        ct.POINTER(ct.c_int),
    ]
    klen = np.zeros((1), dtype="i")
    key = np.zeros((200), dtype="c")
    tmp = _tools.attrs_frame_attribute_value(
        handle,
        frame,
        index,
        key.ctypes.data_as(ct.POINTER(ct.c_char)),
        klen.ctypes.data_as(ct.POINTER(ct.c_int32)),
    )
    if tmp == -2:
        key = np.zeros((klen[0]), dtype="c")
        tmp = _tools.attrs_frame_attribute_value(
            handle,
            frame,
            index,
            key.ctypes.data_as(ct.POINTER(ct.c_char)),
            klen.ctypes.data_as(ct.POINTER(ct.c_int32)),
        )
        if tmp < 0:
            raise RuntimeError(
                "An error occured while calling 'attrs_frame_attribute_value'"
            )
    return toBytes(key)


def attrs_frame_timestamp(handle, frame):
    _tools.attrs_frame_timestamp.argtypes = [ct.c_int, ct.c_int, ct.POINTER(ct.c_int64)]
    time = np.zeros((1), dtype=np.int64)
    tmp = _tools.attrs_frame_timestamp(
        handle, frame, time.ctypes.data_as(ct.POINTER(ct.c_int64))
    )
    if tmp < 0:
        raise RuntimeError("An error occured while calling 'attrs_frame_timestamp'")
    return time[0]


def attrs_timestamps(handle):
    _tools.attrs_timestamps.argtypes = [ct.c_int, ct.POINTER(ct.c_int64)]
    times = np.zeros((attrs_image_count(handle)), dtype=np.int64)
    tmp = _tools.attrs_timestamps(handle, times.ctypes.data_as(ct.POINTER(ct.c_int64)))
    if tmp < 0:
        raise RuntimeError("An error occured while calling 'attrs_timestamps'")
    return times


def attrs_set_times(handle, times):
    _tools.attrs_set_times.argtypes = [ct.c_int, ct.POINTER(ct.c_int64), ct.c_int]
    if type(times) != np.ndarray or times.dtype != np.int64:
        times = np.array(list(times), dtype=np.int64)
    tmp = _tools.attrs_set_times(
        handle, times.ctypes.data_as(ct.POINTER(ct.c_int64)), int(times.shape[0])
    )
    if tmp < 0:
        raise RuntimeError("An error occured while calling 'attrs_set_times'")


def attrs_set_time(handle, frame, time):
    _tools.attrs_set_time.argtypes = [ct.c_int, ct.c_int, ct.c_int64]
    tmp = _tools.attrs_set_time(int(handle), int(frame), time)
    if tmp < 0:
        raise RuntimeError("An error occured while calling 'attrs_set_time'")


def attrs_set_frame_attributes(handle, frame, attributes):
    if type(attributes) is not dict:
        raise RuntimeError(
            "attrs_set_frame_attributes: wrong attributes type (should be dict)"
        )

    _tools.attrs_set_frame_attributes.argtypes = [
        ct.c_int,
        ct.c_int,
        ct.c_char_p,
        ct.POINTER(ct.c_int),
        ct.c_char_p,
        ct.POINTER(ct.c_int),
        ct.c_int,
    ]

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
    tmp = _tools.attrs_set_frame_attributes(
        int(handle),
        int(frame),
        _keys.ctypes.data_as(ct.POINTER(ct.c_char)),
        _klens.ctypes.data_as(ct.POINTER(ct.c_int32)),
        _values.ctypes.data_as(ct.POINTER(ct.c_char)),
        _vlens.ctypes.data_as(ct.POINTER(ct.c_int32)),
        len(attributes),
    )
    if tmp < 0:
        raise RuntimeError(
            "An error occured while calling 'attrs_set_frame_attributes'"
        )


def attrs_set_global_attributes(handle, attributes):
    if type(attributes) is not dict:
        raise RuntimeError(
            "attrs_set_global_attributes: wrong attributes type (should be dict)"
        )

    _tools.attrs_set_global_attributes.argtypes = [
        ct.c_int,
        ct.c_char_p,
        ct.POINTER(ct.c_int),
        ct.c_char_p,
        ct.POINTER(ct.c_int),
        ct.c_int,
    ]

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
    tmp = _tools.attrs_set_global_attributes(
        int(handle),
        _keys.ctypes.data_as(ct.POINTER(ct.c_char)),
        _klens.ctypes.data_as(ct.POINTER(ct.c_int32)),
        _values.ctypes.data_as(ct.POINTER(ct.c_char)),
        _vlens.ctypes.data_as(ct.POINTER(ct.c_int32)),
        len(attributes),
    )
    if tmp < 0:
        raise RuntimeError(
            "An error occured while calling 'attrs_set_global_attributes'"
        )
