# from .rir import *
import logging

from .geometry import rir_geometry
from .low_level import misc
from .signal_processing import rir_signal_processing
from .signal_processing.BadPixels import BadPixels
from .tools import rir_tools
from .video_io import rir_video_io
from .video_io.IRMovie import IRMovie
from .video_io.IRSaver import IRSaver

logging.basicConfig()
logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)
