import ctypes as ct
import glob
import logging
import os
import sys
import tempfile
from joblib import Memory

import numpy as np

__all__ = [
    "_tools",
    "_geometry",
    "_signal_processing",
    "_video_io",
    "__groups",
    "toString",
    "toArray",
    "toBytes",
    "toCharP",
    "createZeroArrayHandle",
    "loadDlls",
    "memory",
]

logging.basicConfig()
logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)

_tools = None
_geometry = None
_signal_processing = None
_video_io = None


__groups = {}

memory = Memory(tempfile.gettempdir(), verbose=0)


def toString(ar):
    """Convert a numpy array of char to a string"""
    if sys.version_info[0] > 2:
        try:
            return bytes(ar).decode("ascii").replace("\x00", "")
        except UnicodeDecodeError:
            return bytes(ar).decode("utf8").replace("\x00", "")
    else:
        return "".join(ar)


def toArray(string):
    """Convert str or bytes object to numpy array of char"""
    res = np.zeros((len(string)), dtype="c")
    if type(string) == bytes:
        b = string
    else:
        b = str(string).encode("ascii")
    if sys.version_info[0] >= 3:
        for i in range(len(b)):
            res[i] = bytes([b[i]])
    else:
        for i in range(len(b)):
            res[i] = b[i]
    return res


def toBytes(ar):
    """Convert numpy array of char to bytes object"""
    if sys.version_info[0] > 2:
        return bytes(ar)
    else:
        return ar.tostring()


def toCharP(obj):
    """Convert bytes or str object to char pointer"""
    if type(obj) == str:
        return obj.encode("ascii")
    elif type(obj) == bytes:
        return obj
    else:
        return bytes(obj)


def createZeroArrayHandle(shape, _dtype):
    """For a given shape and dtype, create a zero filled array and its ctype handle"""
    ar = np.zeros(shape, dtype=_dtype)
    if str(ar.dtype) == "|S1":
        handle = ar.ctypes.data_as(ct.POINTER(ct.c_char))
    else:
        handle = ar.ctypes.data_as(ct.POINTER(np.ctypeslib.as_ctypes_type(ar.dtype)))
    return (ar, handle)


def loadDlls():
    """Load all librir dlls"""
    global _tools
    global _geometry
    global _signal_processing
    global _video_io

    path = os.path.abspath(__file__)
    path = path.replace("\\", "/")
    path = path.split("/")
    path = "/".join(path[0:-2])
    # dlls should be in the lib folder
    path = path + "/libs"

    suffix = "so"
    if os.name == "nt":
        suffix = "dll"

    tools = glob.glob(path + "/*tools*." + suffix)[0]
    geometry = glob.glob(path + "/*geometry*." + suffix)[0]
    signal_processing = glob.glob(path + "/*signal_processing*." + suffix)[0]
    video_io = glob.glob(path + "/*video_io*." + suffix)[0]

    # logger.info(f"tools : {tools}")
    # logger.info(f"geometry : {geometry}")
    # logger.info(f"signal_processing : {signal_processing}")
    # logger.info(f"video_io : {video_io}")
    # logger.info(f"west : {west}")

    dir_path = os.path.dirname(os.path.realpath(tools))
    old_dir = os.getcwd()
    os.chdir(dir_path)

    _tools = ct.cdll.LoadLibrary(tools)
    _geometry = ct.cdll.LoadLibrary(geometry)
    _signal_processing = ct.cdll.LoadLibrary(signal_processing)
    _video_io = ct.cdll.LoadLibrary(video_io)

    os.chdir(old_dir)


loadDlls()
