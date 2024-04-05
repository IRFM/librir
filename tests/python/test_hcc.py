import os
from pathlib import Path
from typing import Optional
from librir.video_io import IRMovie
from librir.video_io.rir_video_io import change_hcc_external_blackbody_temperature
import pytest


@pytest.fixture
def hcc_filename():
    p = os.environ.get("HCC_FILE_TEST", None)
    if p is not None:
        return Path(p)


def test_read_hcc_file(hcc_filename):
    if hcc_filename is None or not hcc_filename.exists():
        pytest.skip("No HCC test file")
    with IRMovie.from_filename(hcc_filename) as mov:
        mov[0]
        mov.frame_attributes
        mov.attributes


@pytest.mark.parametrize("temperature", [300])
def test_change_hcc_external_blackbody_temperature(
    hcc_filename: Optional[Path], temperature
):
    if hcc_filename is None or not hcc_filename.exists():
        pytest.skip("No HCC test file")
    with IRMovie.from_filename(hcc_filename) as mov:
        old_temperature = mov.attributes.get("ExternalBlackBodyTemperature (cC)", 0)
    old_temperature = float(old_temperature)
    change_hcc_external_blackbody_temperature(hcc_filename, temperature)
    with IRMovie.from_filename(hcc_filename) as mov:
        new_temperature = mov.attributes.get("ExternalBlackBodyTemperature (cC)", 0)
    assert float(new_temperature) == temperature
    assert temperature != old_temperature

    change_hcc_external_blackbody_temperature(hcc_filename, old_temperature)
