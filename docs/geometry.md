
# Geometry library

The *geometry* library provides geometry related primitives and functions mainly to work with polygons. All C++ functions/classes are defined within the *rir::* namespace. The library provides a C interface through *geometry.h*. 
the main goal of *geometry* is to provide some fast routines for polygons manipulation in pure Python scripts.
Following examples only use the Python wrapper of the geometry library.
Within the *geometry* module, a polygon is a list (or numpy array) of [x,y] points.

## Polygons interpolation

The function ```polygon_interpolate``` returns an interpolated polygon given 2 input polygons and a position value between 0 and 1. The resulting polygon is guaranteed to have a maximum size of (len(poly1) + len(poly1) -2).
Note that there are an infinit number of ways to compute the interpolation between 2 polygons. This version uses a custom algorithm that fits our needs, and you should test it first with your use case.
Usage:


```python
from librir.geometry import polygon_interpolate

# create a polygon with a rectangular shape
poly1 = [[0,0], [1,0], [1,1], [0,1]]

# create a polygon with a rectangular shape rotated by 45° around (0,0)
poly2 = [[-1,0], [0,1], [1,0], [0,-1]]

# position argument is 0, the result is equivalent to poly1
print( polygon_interpolate(poly1, poly2,0) )
# position argument is 1, the result is equivalent to poly2
print( polygon_interpolate(poly1, poly2,1) )
# position argument is 0.5, the result is in between poly1 and poly2
print( polygon_interpolate(poly1, poly2,0.5) )
```


## Polygon simplification

The functions ```rdp_simplify_polygon``` and ```rdp_simplify_polygon2``` both simplify an input polygon using Ramer-Douglas-Peucker algorithm. The first version uses an *epsilon* value for the simplification, while the former one uses a maximum number of points for the resulting polygon.
Usage:

```python
from librir.geometry import rdp_simplify_polygon2
import math

# This function gets just one pair of coordinates based on the angle theta
def get_circle_coord(theta, x_center, y_center, radius):
    x = radius * math.cos(theta) + x_center
    y = radius * math.sin(theta) + y_center
    return (x,y)

# This function gets all the pairs of coordinates
def get_all_circle_coords(x_center, y_center, radius, n_points):
    thetas = [i/n_points * math.tau for i in range(n_points)]
    circle_coords = [get_circle_coord(theta, x_center, y_center, radius) for theta in thetas]
    return circle_coords

# Create a polygon with a circular shape
circle_coords = get_all_circle_coords(x_center = 5, 
                                      y_center = 15,
                                      radius = 2.5,
                                      n_points = 1000)
                                      
# simplify it down to 10 points maximum
print(rdp_simplify_polygon2(circle_coords, 10))
```


## Polygon drawing

The function ```draw_polygon``` fills a polygon surface inside an existing numpty array.
Usage:

```python
from librir.geometry import draw_polygon
import numpy as np

img = np.zeros((10,10))
# create rectangular polygon
poly = [[1,1], [6,1], [6,6], [1,6]]

print(draw_polygon(img,poly, 1))
```


## Extract polygons from images

The function ```extract_polygon``` extract a polygon from a grayscale image.
Usage:

```python
from librir.geometry import extract_polygon, draw_polygon
import numpy as np

img = np.zeros((10,10))
# create rectangular polygon
poly = [[1,1], [6,1], [6,6], [1,6]]
# draw polygon with value 1
draw_polygon(img,poly, 1)

# extract polygon for pixels of value 1..
# note that if the input image contains multiple closed regions of value 1, only the first one is extracted (scanning line by line).
poly2 = extract_polygon(img,1)
print(poly2)
```


## Additional functions

In addition, the *geometry* module provides the following functions:

-	```extract_convex_hull```: extract convex hull polygon from a set of points
-	```minimum_area_bbox```: extract the minimum area oriented bounding box around given polygon
