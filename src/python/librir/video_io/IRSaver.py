# -*- coding: utf-8 -*-
"""
Created on Mon Jan 13 17:53:46 2020

@author: VM213788
"""

from .. import low_level
from ..low_level.misc import *
from .rir_video_io import (
    h264_open_file,
    h264_close_file,
    h264_set_parameter,
    h264_set_global_attributes,
    h264_add_image_lossless,
    h264_add_image_lossy,
    h264_add_loss,
    h264_get_low_errors,
    h264_get_high_errors,
)


class IRSaver(object):
    """
    Small class to save IR videos in compressed h264 or hevc format and using MP4 container.
    It generates MP4 video files containing IR images on 16 bits per pixel, and any kind of global/frame attributes (see FileAttributes class for more details).

    The output video is compressed with either H264 or HEVC video codec (based onf ffmpeg library) using its lossless compression mode. The potential losses are
    added by the IRSaver itself in order to maximize the video compressibility. The optional temperature losses are bounded and cannot be exceeded.

    In lossy mode, the maximum temperature error is controlled through the parameters "lowValueError"  (error under background value, default to 6°C) and highValueError (error above background, default to 2°C).

    The compression level is controlled through the parameter "compressionLevel" with range from 0 (fastest)
    to 8 (highest compression) with a default value of 0.

    The file format supports global attributes as well as frame attributes on the form of a dict.

    This class should not be used to mix lossy and lossless frames within the same video file.
    """

    handle: int = 0
    width: int = 0
    height: int = 0
    global_attrs = {}
    params = {}

    def __init__(
        self, outfile=None, width=None, height=None, lossy_height=None, clevel=0
    ):
        """
        Constructor.
        If outfile, width and height are not NULL, open the file.
        """

        if outfile is not None and width is not None and height is not None:
            self.filename = outfile
            self.open(outfile, width, height, lossy_height)
            self.set_parameter("compressionLevel", str(clevel))

    def __del__(self):
        """
        Destructor, close the file
        """
        self.close()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        return self.close()

    def is_open(self):
        """
        Returns true if the output file is open
        """
        return self.handle > 0

    def close(self):
        """
        Close output file.
        Note that you MUST close the file after writting frames and before using it, as it will write
        the file trailer (image timestamps and attributes).
        """
        if self.handle > 0:
            h264_close_file(self.handle)
            self.handle = 0

    def open(self, outfile, width, height, lossy_height=None):
        """
        Open output video file.
        @param outfile output video file
        @param width output video width
        @param height output video height
        @param lossy_height image height after which the image is compressed in a lossless way.
        This is used to keep the last rows untouched as they contain additional metadata. By
        default, lossy_height is equal to height.
        """
        self.close()
        self.handle = h264_open_file(outfile, width, height, lossy_height)
        self.width = width
        self.height = height
        self.lossy_height = lossy_height
        self.filename = outfile

        # set attributes and parameters (if any)
        if len(self.global_attrs) > 0:
            h264_set_global_attributes(self.handle, self.global_attrs)
        if len(self.params) > 0:
            for k in self.params:
                h264_set_parameter(self.handle, k, self.params[k])
        self.global_attrs = {}
        self.params = {}

    def set_parameter(self, param, value):
        """
        Set a compression parameter. Supported values:
                - compressionLevel: h264/hevc compression level (0 for very fast, 8 for maximum compression, corresponding to the presets ultrafast, superfast, veryfast, faster, fast, medium, slow, slower, veryslow)
                - lowValueError: for lossy compression, maximum error on temperature values < background
                - highValueError: for lossy compression, maximum error on temperature values >= background
                - codec: either "h264" or "h265" for hevc using kvazaar library
                - GOP: Group Of Picture defining the interval between key frames. Default to 50.
                - threads: number of threads used for compression and loss introduction algorithm. Default to 1.
                - slices: number of slices used by the video codec. This allows multithreaded decompression at the frame level. Default to 1.
                - stdFactor: factor used by the error reduction mechanism as described in []. Default to 5.
                - inputCamera: input camera identifier used for the DL to temperature calibration (see addImageLossy() function). Default to 0 (disabled).
                - removeBadPixels: remove images bad pixels if set to 1. Default to 0 (disabled)
                - runningAverage: running average length as described in [], used for lossy compression. Default to 32.
        """
        if self.is_open():
            h264_set_parameter(self.handle, param, str(value))
        else:
            self.params[param] = str(value)

    def set_global_attributes(self, attributes):
        """
        Set output file global attributes using given dict.
        Note that only the pairs (key,value) convertible to a string are stored.
        """
        if self.is_open():
            h264_set_global_attributes(self.handle, attributes)
        else:
            self.global_attrs = attributes

    def add_image(self, image, timestamp, attributes=dict()):
        """
        Add an image compressed in a lossless way.
        """
        if (
            len(image.shape) != 2
            or image.shape[1] != self.width
            or image.shape[0] != self.height
        ):
            raise RuntimeError("wrong image dimension")

        h264_add_image_lossless(self.handle, image, timestamp, attributes)

    def add_image_lossy(self, image_DL, timestamp, attributes=dict()):
        """
        Add an image compressed in a lossy way.
        """
        if (
            len(image_DL.shape) != 2
            or image_DL.shape[1] != self.width
            or image_DL.shape[0] != self.height
        ):
            raise RuntimeError("wrong DL image dimension")
        # if len(image_T.shape) != 2 or image_T.shape[1] != self.width or image_T.shape[0] != self.height:
        #    raise RuntimeError("wrong T image dimension")

        h264_add_image_lossy(self.handle, image_DL, timestamp, attributes)

    def add_loss(self, image):
        """
        Add loss to image (without writing it) and returns the result
        """
        if (
            len(image.shape) != 2
            or image.shape[1] != self.width
            or image.shape[0] != self.height
        ):
            raise RuntimeError("wrong DL image dimension")
        # if len(image_T.shape) != 2 or image_T.shape[1] != self.width or image_T.shape[0] != self.height:
        #    raise RuntimeError("wrong T image dimension")

        return h264_add_loss(self.handle, image)

    def get_low_errors(self):
        return h264_get_low_errors(self.handle)

    def get_high_errors(self):
        return h264_get_high_errors(self.handle)
