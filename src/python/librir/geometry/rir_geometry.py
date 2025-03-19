import ctypes as ct

import numpy as np

from ..low_level.misc import _geometry, createZeroArrayHandle


def polygon_interpolate(xy1, xy2, advance):
    """
    Interpolate 2 polygons based on the advance parameter [0,1].
    If advance == 0, first polygon is returned as is, and if advance ==1 second polygon is returned as is.
    The interpolated polygon is guaranteed to have a maximum size of (len(x1) + len(x2) -2).

    Returns the new polygon points as a tuple (x,y)
    """
    _geometry.polygon_interpolate.argtypes = [
        ct.POINTER(ct.c_double),
        ct.c_int,
        ct.POINTER(ct.c_double),
        ct.c_int,
        ct.c_double,
        ct.POINTER(ct.c_double),
        ct.POINTER(ct.c_int),
    ]

    xy1 = np.array(xy1, dtype=np.float64)
    xy2 = np.array(xy2, dtype=np.float64)

    xyres = np.zeros((len(xy1) + len(xy2), 2), dtype=np.float64)
    sres = np.zeros((1), dtype=np.int32)
    sres[0] = len(xyres)

    r = _geometry.polygon_interpolate(
        xy1.ctypes.data_as(ct.POINTER(ct.c_double)),
        len(xy1),
        xy2.ctypes.data_as(ct.POINTER(ct.c_double)),
        len(xy2),
        float(advance),
        xyres.ctypes.data_as(ct.POINTER(ct.c_double)),
        sres.ctypes.data_as(ct.POINTER(ct.c_int32)),
    )

    if r < 0:
        raise RuntimeError("An error occured while calling polygon_interpolate")

    return xyres[0 : sres[0]]


def rdp_simplify_polygon(xy, epsilon: float = 0):
    """
    Simplify polygon using Ramer-Douglas-Peucker algorithm.
    """
    _geometry.rdp_simplify_polygon.argtypes = [
        ct.POINTER(ct.c_double),
        ct.c_int,
        ct.POINTER(ct.c_double),
        ct.POINTER(ct.c_int),
        ct.c_double,
    ]

    xy = np.array(xy, dtype=np.float64)

    xyres = np.zeros((len(xy) + 2, 2), dtype=np.float64)
    sres = np.zeros((1), dtype=np.int32)
    sres[0] = len(xyres)

    tmp = _geometry.rdp_simplify_polygon(
        xy.ctypes.data_as(ct.POINTER(ct.c_double)),
        len(xy),
        xyres.ctypes.data_as(ct.POINTER(ct.c_double)),
        sres.ctypes.data_as(ct.POINTER(ct.c_int32)),
        epsilon,
    )

    if tmp == -2:
        xyres = np.zeros((sres[0], 2), dtype=np.float64)
        tmp = _geometry.rdp_simplify_polygon(
            xy.ctypes.data_as(ct.POINTER(ct.c_double)),
            len(xy),
            xyres.ctypes.data_as(ct.POINTER(ct.c_double)),
            sres.ctypes.data_as(ct.POINTER(ct.c_int32)),
            epsilon,
        )
    if tmp < 0:
        raise RuntimeError("An error occured while calling rdp_simplify_polygon")

    return xyres[0 : sres[0]]


def rdp_simplify_polygon2(xy, max_points):
    """
    Simplify given polygon using Ramer-Douglas-Peucker algorithm.
    Unlinke rdp_simplify_polygon, this function takes a maximum number of points as input.
    """
    _geometry.rdp_simplify_polygon2.argtypes = [
        ct.POINTER(ct.c_double),
        ct.c_int,
        ct.POINTER(ct.c_double),
        ct.POINTER(ct.c_int),
        ct.c_int,
    ]

    xy = np.array(xy, dtype=np.float64)

    xyres = np.zeros((max_points, 2), dtype=np.float64)
    sres = np.zeros((1), dtype=np.int32)
    sres[0] = len(xyres)

    r = _geometry.rdp_simplify_polygon2(
        xy.ctypes.data_as(ct.POINTER(ct.c_double)),
        len(xy),
        xyres.ctypes.data_as(ct.POINTER(ct.c_double)),
        sres.ctypes.data_as(ct.POINTER(ct.c_int32)),
        max_points,
    )

    if r < 0:
        raise RuntimeError("An error occured while calling rdp_simplify_polygon2")

    return xyres[0 : sres[0]]


def draw_polygon(img, polygon, fill_value):
    """
    Fill polygon with given value inside input image.
    - img: input image
    - polygon: list of pair (x,y)
    - fill_value: filling value
    Returns the new image.

    Note that, if possible, this function directly writes to the input array if possible (to avoid unecessary copy)
    """

    if len(img.shape) != 2:
        raise RuntimeError("draw_polygon: wrong image size")

    polygon = np.array(polygon, dtype=np.float64)
    fill_value = float(fill_value)
    img = np.array(img, order="C", copy=False)
    data_type = img.dtype.name.encode()

    _geometry.draw_polygon.argtypes = [
        ct.c_void_p,
        ct.POINTER(ct.c_char),
        ct.c_int,
        ct.c_int,
        ct.POINTER(ct.c_double),
        ct.c_int,
        ct.c_double,
    ]

    tmp = _geometry.draw_polygon(
        img.ctypes.data_as(ct.c_void_p),
        data_type,
        img.shape[1],
        img.shape[0],
        polygon.ctypes.data_as(ct.POINTER(ct.c_double)),
        polygon.shape[0],
        fill_value,
    )

    if tmp == -1:
        raise RuntimeError("draw_polygon: unknown error")
    return img


def extract_polygon(img, mask_value, max_size=1000):
    """
    Extract a bounding polygon from a mask
    - img: input image
    - mask_value: pixel value of the mask
    Use 0 to disable simplification.
    Returns a list of points
    """

    if len(img.shape) != 2:
        raise RuntimeError("extract_polygon: wrong image size")

    _geometry.extract_polygon.argtypes = [
        ct.c_void_p,
        ct.POINTER(ct.c_char),
        ct.c_int,
        ct.c_int,
        ct.POINTER(ct.c_int),
        ct.POINTER(ct.c_int),
        ct.c_double,
    ]

    data_type = img.dtype.name.encode()
    xy = np.zeros((1000, 2), dtype=np.int32)
    outsize = np.zeros((1), dtype=np.int32)
    outsize[0] = max_size

    tmp = _geometry.extract_polygon(
        img.ctypes.data_as(ct.c_void_p),
        data_type,
        img.shape[1],
        img.shape[0],
        xy.ctypes.data_as(ct.POINTER(ct.c_int)),
        outsize.ctypes.data_as(ct.POINTER(ct.c_int)),
        mask_value,
    )

    if tmp == -2:
        xy = np.zeros((outsize[0], 2), dtype=np.int32)
        tmp = _geometry.extract_polygon(
            img.ctypes.data_as(ct.c_void_p),
            data_type,
            img.shape[1],
            img.shape[0],
            xy.ctypes.data_as(ct.POINTER(ct.c_int)),
            outsize.ctypes.data_as(ct.POINTER(ct.c_int)),
            mask_value,
        )
    if tmp < 0:
        raise RuntimeError("extract_polygon: unknown error")

    return xy[0 : outsize[0]]


def extract_convex_hull(points):
    """
    Extract convex hull polygon from a set of points
    @param points: input points on the form [[x1,y1],...,[xn,yn]]
    """

    if len(points) == 0:
        return []

    _geometry.extract_convex_hull.argtypes = [
        ct.POINTER(ct.c_double),
        ct.c_int,
        ct.POINTER(ct.c_double),
        ct.POINTER(ct.c_int),
    ]

    xy = np.array(points, dtype=np.float64)
    outxy = np.zeros((len(points), 2), dtype=np.float64)
    outsize = np.zeros((1), dtype=np.int32)
    outsize[0] = len(points)

    tmp = _geometry.extract_convex_hull(
        xy.ctypes.data_as(ct.POINTER(ct.c_double)),
        len(points),
        outxy.ctypes.data_as(ct.POINTER(ct.c_double)),
        outsize.ctypes.data_as(ct.POINTER(ct.c_int)),
    )

    if tmp == -2:
        outxy = np.zeros((outsize[0], 2), dtype=np.float64)
        tmp = _geometry.extract_convex_hull(
            xy.ctypes.data_as(ct.POINTER(ct.c_double)),
            len(points),
            outxy.ctypes.data_as(ct.POINTER(ct.c_double)),
            outsize.ctypes.data_as(ct.POINTER(ct.c_int)),
        )
    if tmp < 0:  # pragma: no cover
        raise RuntimeError("extract_concave_hull: unknown error")

    return outxy[0 : outsize[0]]


def minimum_area_bbox(points):
    """
    Extract the minimum area oriented bounding box around given polygon.
    Returns the tuple (center,width,height,widthAngle,heightAngle) where:
        center is the bounding box center
        width is the smaller box side
        height is the larger box side
        widthAngle is the angle between smaller box side and X axis in radians, positive value means box orientation from bottom right to top left, negative value means opposite
        heightAngle is the angle between larger box side and X axis in radians, positive value means box orientation from bottom left to top right, negative value means opposite
    """
    if len(points) == 0:
        return ([0, 0], 0, 0, 0, 0)

    _geometry.minimum_area_bbox.argtypes = [
        ct.POINTER(ct.c_double),
        ct.c_int,
        ct.POINTER(ct.c_double),
        ct.POINTER(ct.c_double),
        ct.POINTER(ct.c_double),
        ct.POINTER(ct.c_double),
        ct.POINTER(ct.c_double),
        ct.POINTER(ct.c_double),
    ]

    xy = np.array(points, dtype=np.float64)
    center_x, dx = createZeroArrayHandle(1, np.float64)
    center_y, dy = createZeroArrayHandle(1, np.float64)
    width, dw = createZeroArrayHandle(1, np.float64)
    height, dh = createZeroArrayHandle(1, np.float64)
    widthAngle, dwa = createZeroArrayHandle(1, np.float64)
    heightAngle, dha = createZeroArrayHandle(1, np.float64)

    tmp = _geometry.minimum_area_bbox(
        xy.ctypes.data_as(ct.POINTER(ct.c_double)), len(xy), dx, dy, dw, dh, dwa, dha
    )
    if tmp < 0:  # pragma : no cover
        raise RuntimeError("minimum_area_bbox: unknown error")

    return (
        [center_x[0], center_y[0]],
        width[0],
        height[0],
        widthAngle[0],
        heightAngle[0],
    )


def count_pixel_in_polygon(points):
    """
    Count pixel inside a polygon
    @param points: input points on the form [[x1,y1],...,[xn,yn]]
    """

    if len(points) == 0:
        return 0

    _geometry.count_pixel_in_polygon.argtypes = [
        ct.POINTER(ct.c_double),
        ct.c_int,
        ct.POINTER(ct.c_double),
    ]

    xy = np.array(points, dtype=np.float64)
    area = np.zeros((1), dtype=np.float64)

    _tmp = _geometry.count_pixel_in_polygon(
        xy.ctypes.data_as(ct.POINTER(ct.c_double)),
        len(points),
        area.ctypes.data_as(ct.POINTER(ct.c_double)),
    )
    if _tmp < 0:  # pragma : no cover
        raise RuntimeError("count_pixel_in_polygon: unknown error")

    return area[0]
