import logging
from pathlib import Path
from typing import List

from .IRMovie import IRMovie
from .IRSaver import IRSaver

logging.basicConfig()
logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)


def split_rush(filename, index=None, step=30, dest_folder=None) -> List[Path]:
    """
    Splits a long movie into smaller movies according to a fixed number of images.

    @param filename: movie filename
    @param index: list of names for all newly created files
    @param step: image interval between all movies
    @param dest_folder: where splitted movies will be copied
    @return: a list of movie_filenames
    """
    filename = Path(filename)
    movie = IRMovie.from_filename(filename)
    if dest_folder is None:
        dest_folder = filename.parent
    if index is None:
        index = range(movie.images // step)

    res = []
    data = movie.data
    chunks = [data[i : i + step] for i in range(0, len(data), step)]
    for arr, idx in zip(chunks, index):
        if isinstance(idx, float):
            idx = round(idx, 2)
        result_filename = dest_folder / "{}.h264".format(idx)
        result_filename.parent.mkdir(exist_ok=True, parents=True)

        if not result_filename.exists():
            with IRSaver(result_filename, height=movie.height, width=movie.width) as s:
                for i, img in enumerate(arr):
                    s.add_image(img, i * 20e6)
        res.append(result_filename)
    return res


def check_ir_file(filename):
    with IRMovie.from_filename(filename) as mov:
        pass


def is_ir_file_corrupted(filename):
    """
    Returns False if IRMovie file is sane, True if corrupted.
    :param filename:
    :return:
    """
    try:
        check_ir_file(filename)
    except RuntimeError as e:
        logger.warning(f"filename '{filename}' could not be opened : {e}")
        return True
    else:
        return False
