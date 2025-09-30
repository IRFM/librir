# -*- coding: utf-8 -*-
"""
Created on Mon Jan 13 17:53:46 2020

@author: VM213788
"""

from os import PathLike
from typing import Dict
import numpy as np
from ..low_level.misc import *  # noqa: F403
from .rir_tools import (
    attrs_open_buffer,
    attrs_close,
    attrs_discard,
    attrs_global_attribute_count,
    attrs_frame_attribute_count,
    attrs_global_attribute_name,
    attrs_global_attribute_value,
    attrs_frame_attribute_name,
    attrs_frame_attribute_value,
    attrs_open_file,
    attrs_timestamps,
    attrs_set_times,
    attrs_set_frame_attributes,
    attrs_set_global_attributes,
    attrs_flush,
)
import atexit


class FileAttributes(object):
    """
    Small class handling file attributes based on librir attributes format.
    Note that any files can have attributes as they are pushed back to the file, but it is
    currently only used for MP4 video files.

    Newly defined attributes are only written to the file when the FileAttributes object is destroyed
    or the flush() function is called.

    Use the discard() function to avoid writting attributes to the file.
    """

    _attributes: Dict[str, bytes]

    @classmethod
    def from_buffer(cls, buffer: bytes):
        handle = attrs_open_buffer(buffer)
        return cls(handle)

    @classmethod
    def from_filename(cls, filename: PathLike):
        handle = attrs_open_file(filename)
        return cls(handle)

    def __init__(self, handle):
        """
        Constructor.
        Create a FileAttributes object from a filename.
        """
        self.handle = handle

        # read timestamps and attributes
        self._timestamps = attrs_timestamps(self.handle)
        count = attrs_global_attribute_count(self.handle)
        self._attributes = {}
        for i in range(count):
            key = attrs_global_attribute_name(self.handle, i)
            value = attrs_global_attribute_value(self.handle, i)
            self._attributes[key] = value
        atexit.register(self.close)

    def close(self):
        """
        Write attributes and close the file.
        The FileAttributes object cannot be used anymore.
        """
        if self.handle:
            # write global attributes
            self.attributes = self._attributes
            # write times
            self.timestamps = self._timestamps
            attrs_close(self.handle)
            self.handle = 0

    def flush(self):
        if self.handle:
            attrs_flush(self.handle)

    def discard(self):
        """
        Close the file without writting additional attributes.
        The FileAttributes object cannot be used anymore.
        """
        if not self.handle:
            return
        attrs_discard(self.handle)
        self.handle = 0

    # def __del__(self):
    #     """
    #     Destructor, write attributes and close the file
    #     """
    #     self.close()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        return self.close()

    def is_open(self):
        """
        Returns true if the attribute file is open
        """
        return self.handle > 0

    def frame_count(self):
        return self._timestamps.shape[0]

    @property
    def timestamps(self):
        """
        Get/set the timestamps within the file
        """
        return self._timestamps

    @timestamps.setter
    def timestamps(self, times):
        attrs_set_times(self.handle, times)
        if not isinstance(times, np.ndarray) or times.dtype != np.int64:
            times = np.array(list(times), dtype=np.int64)
        self._timestamps = times

    @property
    def attributes(self) -> Dict[str, bytes]:
        """Get/set the global file attributes"""
        return self._attributes

    @attributes.setter
    def attributes(self, attributes):
        attrs_set_global_attributes(self.handle, attributes)
        self._attributes = attributes

    def frame_attributes(self, frame_index):
        """
        Returns the frame attributes for given frame index
        """
        count = attrs_frame_attribute_count(self.handle, frame_index)
        attributes = {}
        for i in range(count):
            key = attrs_frame_attribute_name(self.handle, frame_index, i)
            value = attrs_frame_attribute_value(self.handle, frame_index, i)
            attributes[key] = value
        return attributes

    def set_frame_attributes(self, frame_index, attributes):
        """
        Set the frame attributes for given frame index
        """
        attrs_set_frame_attributes(self.handle, frame_index, attributes)
