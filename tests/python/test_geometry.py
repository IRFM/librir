from librir.geometry.rir_geometry import count_pixel_in_polygon
import pytest


@pytest.fixture()
def points():
    return [
        (562, 231),
        (562, 271),
        (522, 271),
        (522, 230),
        (562, 230),
    ]


def test_count_pixel_in_polygon(points):
    area = count_pixel_in_polygon(points)
    assert area == 1640
