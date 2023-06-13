#ifndef GEOMETRY_H
#define GEOMETRY_H


/** @file

C interface for the geometry library
*/

#include "rir_config.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
Interpolate 2 polygons based on the advance parameter [0,1].
If advance == 0, p1 is returned as is, and if advance ==1 p2 is returned as is.
The interpolated polygon is guaranteed to have a maximum size of (s1 + s2 -2).
Returns 0 on success, -1 on error.
*/
GEOMETRY_EXPORT int polygon_interpolate(double* xy1, int s1, double* xy2, int s2, double advance, double* xyres, int* sres);

/**
Simplify polygon using Ramer-Douglas-Peucker algorithm (see https://github.com/prakol16/rdp-expansion-only for more details).
@param  epsilon: epsilon value used to simplify the polygon by the Ramer-Douglas-Peucker algorithm.
Returns 0 on success, -1 on error, -2 if output point count is too small (in which case out_point_count is set to the right value).
*/
GEOMETRY_EXPORT int rdp_simplify_polygon(double* xy, int point_count, double* out_xy, int* out_point_count, double epsilon);
/**
Simplify given polygon using Ramer-Douglas-Peucker algorithm (see https://gist.github.com/msbarry/9152218 for more details).
@param  max_points: unlinke vipRDPSimplifyPolygon, this function takes a maximum number of points as input.
Returns 0 on success, -1 on error, -2 if output point count is too small (in which case out_point_count is set to the right value).
*/
GEOMETRY_EXPORT int rdp_simplify_polygon2(double* xy, int point_count, double* out_xy, int* out_point_count, int maxPointCount);

/**
Draw a polygon on an image.
@param  img: input image data
@param  img_type: input image type, numpy style ('bool','int8',...,'float32','float64')
@param  w: image width
@param  h: image height
@param  x: polygon x values
@param  y: polygon y values
@param  point_count: number of points in polygon
@param  value: fill value
Returns 0 on success, -1 on error.
*/
GEOMETRY_EXPORT int draw_polygon(void* img, const char* img_type, int w, int h, double* xy, int point_count, double value);

/**
Extract a bounding polygon from a mask
@param  img: input image data
@param  img_type: input image type, numpy style ('bool','int8',...,'float32','float64')
@param  w: image width
@param  h: image height
@param  x: output polygon x values
@param  y: output polygon y values
@param  point_count: input/output number of points in polygon
@param  pix_value: pixel value of the mask
Returns 0 on success, -1 on error.
Returns -2 if point_count is too small, and set point_count to the right value.
*/
GEOMETRY_EXPORT int extract_polygon(void* img, const char* img_type, int w, int h, int* xy, int* point_count, double pix_value);

/**
Extract convex hull polygon from a set of points
@param x: input x values
@param y: input y values
@param point_count: input number of points
@param out_x: output polygon x values
@param out_y: output polygon y values
@param out_point_count: output polygon point number
Returns 0 on success, -1 on error.
Returns -2 if out_point_count is too small, and set out_point_count to the right value.
*/
GEOMETRY_EXPORT int extract_convex_hull(double* xy, int point_count, double* out_xy, int* out_point_count);

/**
Returns the minimum area oriented bounding box around a set of points.
This function is based the convex hull of given points.
@param x: input x values
@param y: input y values
@param point_count: input number of points
@param x_center: output BBox center x coordinate
@param y_center: output BBox center y coordinate
@param width: smaller box side
@param height: larger box side
@param widthAngle: angle between smaller box side and X axis in radians, positive value means box orientation from bottom right to top left, negative value means opposite
@param heightAngle: angle between larger box side and X axis in radians, positive value means box orientation from bottom left to top right, negative value means opposite
Returns 0 on success, -1 on error.
*/
GEOMETRY_EXPORT int minimum_area_bbox(double* xy, int point_count, double* x_center, double* y_center, double* width, double* height, double* widthAngle, double* heightAngle);

    /**
     * Returns the area inside a polygon defined by (x0,y0,..,xn,yn) points
     * @param xy: input points
     * @param point_count: input number of points
     */
    GEOMETRY_EXPORT int count_pixel_in_polygon(double *xy, int point_count, double *area);

#ifdef __cplusplus
}
#endif


#endif /*GEOMETRY_H*/


