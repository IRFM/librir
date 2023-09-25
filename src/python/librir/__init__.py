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

# __all__ = [
#     "rir_tools",
#     "rir_signal_processing",
#     "rir_geometry",
#     "rir_video_io",
# ]

# import importlib
# import pkgutil

# from . import plugins

# # import sys
# # if sys.version_info < (3, 10):
# #     from importlib_metadata import entry_points
# # else:
# #     from importlib.metadata import entry_points


# # discovered_plugins = entry_points(group='plugins')

# def iter_namespace(ns_pkg):
# # Specifying the second argument (prefix) to iter_modules makes the
# # returned name an absolute name instead of a relative one. This allows
# # import_module to work without having to do additional modification to
# # the name.
# return pkgutil.iter_modules(ns_pkg.__path__, ns_pkg.__name__ + ".")

# discovered_plugins = {
# name: importlib.import_module(name)
# for finder, name, ispkg
# in iter_namespace(plugins)
# }
