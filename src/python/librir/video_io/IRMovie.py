import functools
import logging
import math
import os
import tempfile
from pathlib import Path
from typing import List, Optional, Union

import numpy as np
import pandas as pd

from librir.tools.FileAttributes import FileAttributes
from librir.tools._thermavip import init_thermavip, unbind_thermavip_shared_mem
from librir.video_io.rir_video_io import (
    enable_motion_correction,
    load_motion_correction_file,
    motion_correction_enabled,
    video_file_format,
)

from .IRSaver import IRSaver
from .rir_video_io import (
    FileFormat,
    calibrate_image,
    calibration_files,
    close_camera,
    enable_bad_pixels,
    flip_camera_calibration,
    get_attributes,
    get_emissivity,
    get_filename,
    get_global_attributes,
    get_global_emissivity,
    get_image_count,
    get_image_size,
    get_image_time,
    load_image,
    open_camera_file,
    open_camera_memory,
    set_emissivity,
    set_global_emissivity,
    support_emissivity,
    supported_calibrations,
)

logging.basicConfig()
logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)


class CalibrationNotFound(Exception):
    pass


class InvalidMovie(Exception):
    pass


def create_pcr_header(rows, columns, frequency=50, bits=16):
    pcr_header = np.zeros((256,), dtype=np.uint32)
    pcr_header[2] = columns
    pcr_header[3] = rows
    pcr_header[5] = bits  # bits
    pcr_header[7] = frequency
    pcr_header[9] = rows * columns * 2
    pcr_header[10] = columns
    pcr_header[11] = rows
    return pcr_header


class IRMovie(object):
    _header_offset = 1024
    __tempfile__ = None
    _file_attributes: Union[FileAttributes, None] = None
    handle = -1
    _calibration_nickname_mapper = {"DL": "Digital Level"}
    _th = None

    @classmethod
    def from_filename(cls, filename):
        """Create an IRMovie object from a local filename"""
        handle = open_camera_file(str(filename))
        instance = IRMovie(handle)
        instance._file_attributes = FileAttributes.from_filename(filename)
        instance._file_attributes.attributes = get_global_attributes(instance.handle)
        return instance

    @classmethod
    def from_bytes(cls, data: bytes):
        """_summary_

        Args:
            data (bytes): _description_
            times (List[float], optional): timestamps in seconds. Defaults to None.

        Returns:
            _type_: _description_
        """

        handle = open_camera_memory(data)
        instance = cls(handle)
        instance._file_attributes = FileAttributes.from_buffer(data)
        instance._file_attributes.attributes = get_global_attributes(instance.handle)

        return instance

    @classmethod
    def from_numpy_array(
        cls, arr: np.ndarray, attrs=None, times=None, cthreads: int = 8
    ):
        """
        Create a IRMovie object via numpy arrays. It creates non-pulse indexed IRMovie
        object.
        Hence, calibration data are not available.
        :param arr: data
        :return:
        """
        if len(arr.shape) == 2:
            rows, columns = arr.shape
            header = create_pcr_header(rows, columns)
        elif len(arr.shape) == 3:
            images, rows, columns = arr.shape
            header = create_pcr_header(rows, columns)
        else:
            raise ValueError("mismatch array shape. Must be 2D or 3D")
        data = header.astype(np.uint32).tobytes() + arr.astype(np.uint16).tobytes()

        with tempfile.NamedTemporaryFile("wb", delete=False) as f:
            filename = Path(f.name)
            f.write(data)

        with cls.from_filename(filename) as _instance:
            _instance.__tempfile__ = filename
            dst = Path(filename).parent / (f"{filename.stem}.h264")
            _instance.to_h264(dst, times=times, cthreads=cthreads)

        instance = cls.from_filename(dst)

        if attrs is not None:
            instance.attributes = attrs
            instance._file_attributes.flush()

        return instance

    def __init__(self, handle):
        """
        Create a IRMovie object from an unique identifier as returned by open_camera()
        """
        # check valid handle identifier
        if get_image_count(handle) < 0:
            raise InvalidMovie("Invalid ir_movie descriptor")

        self.handle = handle
        self.times = None
        self._bad_pixels_correction = False
        self._last_lines = None

        self.__tempfile__ = ""
        self._calibration_index = 0
        self._timestamps = None
        self._camstatus = None
        self._frame_attributes_d = {}
        self._registration_file = None

        self.calibration = "DL"

    @property
    def calibration(self):
        return list(self._calibration_nickname_mapper.keys())[self._calibration_index]

    def _parse_calibration_index(self, value: Union[str, int]):
        searching_keys = self.calibrations + list(
            self._calibration_nickname_mapper.keys()
        )
        _calibrations = self.calibrations

        if isinstance(value, int):
            if value >= len(_calibrations):
                raise CalibrationNotFound(
                    f"Available calibrations : {self.calibrations}. "
                    f"Calibration index out of range : {self._calibration_index}"
                )

            return value

        if value not in searching_keys:
            raise CalibrationNotFound(
                f"{value} not in available calibrations : {searching_keys}"
            )
        lists = list(self._calibration_nickname_mapper), self.calibrations
        idx = None
        for _list in lists:
            try:
                idx = _list.index(value)
                return idx
            except ValueError:
                pass

    @calibration.setter
    def calibration(self, value: Union[str, int]):
        _old_calib_idx = self._calibration_index
        _calibration_index = self._parse_calibration_index(value)
        if _old_calib_idx != _calibration_index:
            self._calibration_index = _calibration_index
            self._payload = None
            # clearing lru cache of IRMovie
            IRMovie.data.fget.cache_clear()
        if _calibration_index is None:
            self._calibration_index = _old_calib_idx
            raise CalibrationNotFound(f"calibration '{value}' is not registered")

    def __enter__(self):
        """
        Allows to use 'with' statement
        """
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """
        When you exit the with statement
        """
        return self.close()

    def __del__(self):
        self.close()

    def close(self):
        if self.handle > 0:
            # self._file_attributes.close()
            close_camera(self.handle)
            self.handle = -1

        # destroying files if instantiated in memory.
        if self.__tempfile__:
            try:
                os.unlink(self.__tempfile__)
                logger.debug(f"deleting {self.__tempfile__}")
            except FileNotFoundError as exc:
                logger.warning(exc)
            except PermissionError as p_exc:
                logger.warning(p_exc)

            self.__tempfile__ = None

    @property
    def registration_file(self) -> Optional[Path]:
        """
        Returns the registration file name for this camera, and tries to download it
        from ARCADE if not already done.
        """
        if self._registration_file:
            return self._registration_file

    @registration_file.setter
    def registration_file(self, value) -> None:
        value = Path(value)
        if not value.exists():
            raise FileNotFoundError(f"{value} doesn't exist")
        self._registration_file = value
        load_motion_correction_file(self.handle, self._registration_file)
        self.registration = True

    @property
    def registration(self) -> bool:
        """
        Returns True is video registration is activated, false otherwise
        """
        if self._registration_file is not None and self._registration_file.exists():
            return motion_correction_enabled(self.handle)
        return False

    @registration.setter
    def registration(self, value: bool) -> None:
        """
        Enable/disable video registration.
        Throws if the registration file cannot be found for this video.
        """

        enable_motion_correction(self.handle, bool(value))

    def flip_calibration(self, flip_rl, flip_ud):
        flip_camera_calibration(self.handle, flip_rl, flip_ud)

    @property
    def images(self):
        return get_image_count(self.handle)  # number of images

    @property
    def image_size(self):
        return get_image_size(self.handle)

    @property
    def calibrations(self):
        return supported_calibrations(
            self.handle
        )  # possible calibrations (usually 'Digital Level' and 'Temperature(C)')

    # def frame_attributes(self, frame_index):
    #     return self._file_attributes.frame_attributes(frame_index)

    def load_pos(self, pos, calibration=None) -> np.ndarray:
        """
        Returns the image at given position using given calibration index (integer)
        """
        if calibration is None:
            calibration = 0
        if self._parse_calibration_index(calibration) != self._calibration_index:
            self.calibration = self._parse_calibration_index(calibration)
        res = load_image(self.handle, pos, self._calibration_index)
        self._frame_attributes_d[pos] = get_attributes(self.handle)
        # self.frame_attributes = get_attributes(self.handle)
        return res

    def load_secs(self, time, calibration=None):
        """
        Returns the image at given time in seconds using given calibration index
        (integer)
        """
        if calibration is None:
            calibration = 0
        if self.times is None:
            self.times = np.array(list(self.timestamps), dtype=np.float64)
        self.calibration = calibration or self.calibration
        index = np.argmin(np.abs(self.times - time))
        res = load_image(self.handle, int(index), self._calibration_index)
        self._frame_attributes_d[int(index)] = get_attributes(self.handle)
        # self.frame_attributes =get_attributes(self.handle)
        return res

    @property
    def bad_pixels_correction(self):
        return self._bad_pixels_correction

    @bad_pixels_correction.setter
    def bad_pixels_correction(self, value):
        self._bad_pixels_correction = bool(value)
        enable_bad_pixels(self.handle, self._bad_pixels_correction)

    @property
    def filename(self):
        _f = get_filename(self.handle)
        return Path(_f) if _f else None  # local filename

    @property
    def calibration_files(self) -> List[str]:
        try:
            return calibration_files(self.handle)
        except RuntimeError:
            return list()

    @property
    def frame_attributes(self):
        return get_attributes(self.handle)

    @property
    def attributes(self):
        # return self._file_attributes.attributes
        # self._file_attributes.attributes = get_global_attributes(self.handle)
        # d.update(self.additional_attributes)
        return self._file_attributes.attributes

    #
    @attributes.setter
    def attributes(self, value):
        self._file_attributes.attributes = value

    @property
    def is_file_uncompressed(self):
        if self.filename is None:
            return False

        statinfo = os.stat(self.filename)
        filesize = statinfo.st_size
        theoritical_uncompressed = (
            self.images * ((self.image_size[1] * self.image_size[0] * 2) / 1e9)
            + 1024 / 1e9
        )
        return (filesize / 1e9) == theoritical_uncompressed

    @property
    def width(self):
        return self.image_size[1]

    @property
    def height(self):
        return self.image_size[0]

    @property
    @functools.lru_cache(maxsize=None)
    def data(self) -> np.ndarray:
        """
        Accessor to data contained in movie file.

        @return: 3D np.array representing IR data movie
        """
        return self[:]

    @property
    def support_emissivity(self):
        return support_emissivity(self.handle)

    @property
    def global_emissivity(self):
        if not self.support_emissivity:
            return 1.0
        return get_global_emissivity(self.handle)

    @global_emissivity.setter
    def global_emissivity(self, value):
        if not self.support_emissivity:
            raise RuntimeError("Cannot set custom emissivity value for this handle")
        set_global_emissivity(self.handle, value)

    @property
    def emissivity(self):
        if not self.support_emissivity:
            return np.ones(self.image_size, dtype=np.float32)
        return get_emissivity(self.handle)

    @emissivity.setter
    def emissivity(self, emissivity_array):
        if not self.support_emissivity:
            raise RuntimeError("Cannot set custom emissivity value for this handle")
        set_emissivity(self.handle, emissivity_array)

    @property
    @functools.lru_cache()
    def tis(self):
        """
        Gives matrix of used integrations times for all images.
        :return:
        """
        if self.calibration == "Digital Level":
            tis = (self.data & (2**16 - 2**13)) >> 13
        else:
            old_calib = self.calibration
            self.calibration = "DL"
            tis = (self.data & (2**16 - 2**13)) >> 13
            self.calibration = old_calib
        return tis

    def __getitem__(self, item):
        if isinstance(item, slice):
            item = slice(item.start or 0, item.stop or self.images, item.step or 1)
            if item.stop < 0:
                item = slice(item.start, self.images + item.stop, item.step)

            if item.start < 0:
                item = slice(self.images + item.start, item.stop, item.step)

            shape = (math.ceil((item.stop - item.start) / item.step),) + self.image_size
            arr = np.empty(shape, dtype=np.uint16)
            for idx, i in enumerate(range(item.start, item.stop, item.step)):
                arr[idx] = self.load_pos(i, self._calibration_index)

            return arr

        elif isinstance(item, tuple) and len(item) >= 2 and isinstance(item[0], int):
            return self.__getitem__(item[0])[item[1:]]

        elif isinstance(item, int):
            if item < 0:
                item = self.images + item
            return self.load_pos(item, self._calibration_index)

        elif isinstance(item, float):
            return self.load_secs(item, self._calibration_index)

        elif isinstance(item, (list)):
            return np.array([self.__getitem__(e) for e in item])

        elif isinstance(item, np.ndarray) and (item.ndim == 1):
            return np.array([self.__getitem__(e) for e in item])

    def __iter__(self):
        for i in range(self.images):
            yield self.load_pos(i, self._calibration_index)

    def __len__(self):
        return self.images

    @property
    def timestamps(self):
        if self._timestamps is None:
            try:
                self._timestamps = np.array(
                    [get_image_time(self.handle, i) * 1e-9 for i in range(self.images)],
                    dtype=np.float64,
                )
            except RuntimeError as e:
                logger.warning(f"No timestamps in {self}")
        return self._timestamps

    @timestamps.setter
    def timestamps(self, value: List[float]):
        """_summary_

        Args:
            value (List[float]): in nanoseconds
        """
        value = np.array(value) * 1e-9
        self._timestamps = value

    @property
    def frame_period(self) -> float:
        return np.diff(self.timestamps).mean().round(3)

    @property
    def duration(self):
        last_time = get_image_time(self.handle, self.images - 1)
        first_time = get_image_time(self.handle, 0)
        return (last_time - first_time) * 1e-9  # movie duration in seconds

    def calibrate(self, image, calib):
        """
        Apply given calibration to a DL image and returns the result.
        """
        return calibrate_image(self.handle, image, calib)

    @property
    def video_file_format(self):
        return video_file_format(self.filename)

    def pcr2h264(self, outfile=None, overwrite=False, **kwargs):
        """
        If movie is stored into a PCR file, it converts it into h264
        @param outfile: destination filename
        @param overwrite: if you want to overwrite destination file
        @param kwargs: keyword arguments passed to `to_h264` method
        @return: destination filename
        """
        outfile = outfile or self._build_outfile()
        if not os.path.exists(outfile) or overwrite:
            self.to_h264(outfile, **kwargs)
        return outfile

    def _build_outfile(self) -> str:
        f = self.__tempfile__ or self.filename

        # pathlib is not used because this class must be compatible with python 2
        # TODO: go with pathlib, no need for python 2 compatibility
        _, suffix = os.path.splitext(f)
        parent, basename = os.path.split(f)
        stem = basename.replace(suffix, "")

        if self.video_file_format != FileFormat.H264:
            return os.path.abspath((os.path.join(parent, (stem + ".h264"))))

        return self.filename

    def to_h264(
        self,
        dst_filename: Union[str, Path],
        start_img=0,
        count=-1,
        clevel=8,
        attrs=None,
        times=None,
        frame_attributes=None,
        cthreads=8,
        cfiles: List[str] = None,
    ):
        """
        Exports movie into h264 file.
        @param dst_filename: destination file
        @param start_img: image index to start export
        @param count: number of frame to export
        @param clevel: compression level
        @return:
        """
        # set image count
        if count < 0:
            count = self.images
        if start_img + count > self.images:
            count = self.images - start_img
        if count == 0:
            raise RuntimeError("No images in selected range to save")

        logger.info(
            "Start saving in {} from {} to {}".format(
                dst_filename, start_img, start_img + count
            )
        )

        # retrieve attributes
        if attrs is None:
            attrs = self.attributes
        if (frame_attributes is not None) and (len(frame_attributes) != count):
            raise RuntimeError(
                "Given frame attributes are not equal to the number of saved images"
            )

        # adding custom attribute --> must be a dict[str]=str
        # attrs.update(self.additional_attributes)

        logger.info("Found keys {}".format(list(attrs.keys())))

        # # set calibration files (if needed)
        # if cfiles is None:
        #     cfiles = []

        # check if file is saved in temperature
        try:
            attrs.pop("MIN_T")
            attrs.pop("MIN_T_HEIGHT")
            attrs.pop("STORE_IT")
            logger.info("Images saved in temperature, switch to DL")
        except KeyError:
            logger.info("Images saved in DL")
            pass

        h, w = self.image_size
        if times is None:
            times = (
                list(t * 1e9 for t in self.timestamps)
                if self.timestamps is not None
                else range(start_img, start_img + count)
            )
        with IRSaver(dst_filename, w, h, h, clevel) as s:
            s.set_global_attributes(attrs)
            s.set_parameter("threads", cthreads)
            # Force codec to be h264
            s.set_parameter("codec", "h264")
            saved = 0
            for i in range(start_img, start_img + count):
                img = self.load_pos(i, 0)

                _frame_attributes = (
                    self.frame_attributes
                    if frame_attributes is None
                    else frame_attributes[saved]
                )

                s.add_image(img, times[i], attributes=_frame_attributes)

                saved += 1
                if saved % 100 == 0:
                    logger.info("Saved {} images...".format(saved))
            logger.info("{} image(s) saved".format(count))

    def __repr__(self):
        return "IRMovie({})".format(self.filename)

    @property
    @functools.lru_cache()
    def frames_attributes(self) -> pd.DataFrame:
        if len(self._frame_attributes_d) != self.images:
            for i in range(self.images):
                self.load_pos(i, self._calibration_index)
        df = pd.DataFrame(self._frame_attributes_d).T
        return df

    def _frame_attribute_getter(self, key) -> np.ndarray:
        values = []
        try:
            values = self.frames_attributes[key]
        except KeyError:
            logger.warning(f"attribute '{key}' not found in movie !")
        finally:
            return np.array(values, dtype=float)

    def to_thermavip(self, th_instance="Thermavip-1", player_id=0):
        th = init_thermavip(th_instance)
        if th:
            filename = self.filename
            # TODO: remove when thermavip will ignore file extensions when opening files
            # if not self.filename.suffix:
            #     dest = str(self.filename.absolute()) + '.h264'
            #     shutil.copy(self.filename, dest)
            #     filename = dest

            player_id = th.open(
                f"WEST_BIN_PCR_Device:{str(filename)}", player=player_id
            )
            unbind_thermavip_shared_mem(th)
            return player_id
        else:
            logger.warning("Thermavip not found")
