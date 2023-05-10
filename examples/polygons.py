#######
# Test polygon_interpolate
#######

from librir.geometry import polygon_interpolate

# create a polygon with a rectangular shape
poly1 = [[0, 0], [1, 0], [1, 1], [0, 1]]

# create a polygon with a rectangular shape rotated by 45° around (0,0)
poly2 = [[-1, 0], [0, 1], [1, 0], [0, -1]]

# position argument is 0, the result is equivalent to poly1
print(polygon_interpolate(poly1, poly2, 0))
# position argument is 1, the result is equivalent to poly2
print(polygon_interpolate(poly1, poly2, 1))
# position argument is 0.5, the result is in between poly1 and poly2
print(polygon_interpolate(poly1, poly2, 0.5))


#######
# Test rdp_simplify_polygon2
#######

from librir.geometry import rdp_simplify_polygon2
import math

# This function gets just one pair of coordinates based on the angle theta
def get_circle_coord(theta, x_center, y_center, radius):
    x = radius * math.cos(theta) + x_center
    y = radius * math.sin(theta) + y_center
    return (x, y)


# This function gets all the pairs of coordinates
def get_all_circle_coords(x_center, y_center, radius, n_points):
    thetas = [i / n_points * math.tau for i in range(n_points)]
    circle_coords = [
        get_circle_coord(theta, x_center, y_center, radius) for theta in thetas
    ]
    return circle_coords


# Create a polygon with a circular shape
circle_coords = get_all_circle_coords(
    x_center=5, y_center=15, radius=2.5, n_points=1000
)

# simplify it down to 10 points maximum
print(rdp_simplify_polygon2(circle_coords, 10))


#######
# Test draw_polygon
#######

from librir.geometry import draw_polygon
import numpy as np

img = np.zeros((10, 10))
# create rectangular polygon
poly = [[2, 1], [6, 1], [6, 6], [2, 6]]

print(draw_polygon(img, poly, 1))


#######
# Test extract_polygon
#######

from librir.geometry import extract_polygon, draw_polygon
import numpy as np

img = np.zeros((10, 10))
# create rectangular polygon
poly = [[1, 1], [6, 1], [6, 6], [1, 6]]
# draw polygon with value 1
draw_polygon(img, poly, 1)

# extract polygon for pixels of value 1..
# note that if the input image contains multiple closed regions of value 1, only the first one is extracted (scanning line by line).
poly2 = extract_polygon(img, 1)
print(poly2)
