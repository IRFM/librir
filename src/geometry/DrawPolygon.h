#pragma once

#include "Polygon.h"

/** @file

Defines helper functions to draw polygons on matrices
*/

namespace rir
{
	namespace detail
	{

#define POINT_SORT(a,b) { if ((a.y())>(b.y())) std::swap((a),(b)); }

		/*----------------------------------------------------------------------------
		Median for 3 values
		---------------------------------------------------------------------------*/
		inline void sort_3_point(Point* p)
		{
			POINT_SORT(p[0], p[1]); POINT_SORT(p[1], p[2]); POINT_SORT(p[0], p[1]);
		}

		//
		//see http://www.sunshine2k.de/coding/java/TriangleRasterization/TriangleRasterization.html
		//

		template< class T>
		static inline void fillBottomFlatTriangle(const Point& v1, const Point& v2, const Point& v3, Array2DView<T>& out, T color)
		{
			signed_integral start_y = v1.y();
			signed_integral end_y = v2.y();
			if (start_y < 0 && end_y < 0) return;
			else if (start_y >= out.height && end_y >= out.height) return;
			if (start_y < 0) start_y = 0;
			if (end_y >= out.height) end_y = out.height - 1;

			float invslope1 = float(v2.x() - v1.x()) / (v2.y() - v1.y());
			float invslope2 = float(v3.x() - v1.x()) / (v3.y() - v1.y());

			float curx1 = (float)v1.x();
			float curx2 = (float)v1.x();

			T* pix = out.values;

			for (signed_integral scanlineY = start_y; scanlineY <= end_y; scanlineY++)
			{
				signed_integral start, end;
				if (curx1 > curx2) {
					start = (signed_integral)std::round(curx2);
					end = (signed_integral)std::round(curx1);
				}
				else {
					start = (signed_integral)std::round(curx1);
					end = (signed_integral)std::round(curx2);
				}
				if (start < 0 && end < 0)
					continue;
				else if (start >= out.width && end >= out.width)
					continue;
				if (start < 0)
					start = 0;
				else if (start >= out.width)
					start = out.width - 1;
				if (end < 0)
					end = 0;
				else if (end >= out.width)
					end = out.width - 1;

				std::fill_n(pix + scanlineY * out.width + (signed_integral)start, (signed_integral)(end - start + 1), color);
				curx1 += invslope1;
				curx2 += invslope2;
			}
		}
		template< class T>
		static inline void fillTopFlatTriangle(const Point& v1, const Point& v2, const Point& v3, Array2DView<T>& out, T color)
		{
			signed_integral start_y = v1.y();
			signed_integral end_y = v3.y();
			if (start_y < 0 && end_y < 0) return;
			else if (start_y >= out.height && end_y >= out.height) return;
			if (start_y < 0) start_y = 0;
			if (end_y >= out.height) end_y = out.height - 1;

			float invslope1 = float(v3.x() - v1.x()) / (v3.y() - v1.y());
			float invslope2 = float(v3.x() - v2.x()) / (v3.y() - v2.y());

			float curx1 = (float)v3.x();
			float curx2 = (float)v3.x();

			T* pix = out.values;
			for (signed_integral scanlineY = end_y; scanlineY >= start_y; scanlineY--)
			{
				signed_integral start, end;
				if (curx1 > curx2) {
					start = (signed_integral)std::round(curx2);
					end = (signed_integral)std::round(curx1);
				}
				else {
					start = (signed_integral)std::round(curx1);
					end = (signed_integral)std::round(curx2);
				}
				if (start < 0 && end < 0)
					continue;
				else if (start >= out.width && end >= out.width)
					continue;
				if (start < 0)
					start = 0;
				else if (start >= out.width)
					start = out.width - 1;
				if (end < 0)
					end = 0;
				else if (end >= out.width)
					end = out.width - 1;

				std::fill_n(pix + scanlineY * out.width + (signed_integral)start, (signed_integral)(end - start + 1), color);
				//drawLine((signed_integral)curx1, scanlineY, (signed_integral)curx2, scanlineY);
				curx1 -= invslope1;
				curx2 -= invslope2;
			}
		}
	}

	/**
	* Draw triangle on image \a out with given \a color
	*/
	template< class T>
	static inline void drawTriangle(Point* p, Array2DView<T>& out, T color)
	{
		/* at first sort the three vertices by y-coordinate ascending so v1 is the topmost vertice */
		detail::sort_3_point(p);

		/* here we know that v1.y <= v2.y <= v3.y */
		/* check for trivial case of bottom-flat triangle */
		if (p[1].y() == p[2].y())
		{
			fillBottomFlatTriangle(p[0], p[1], p[2], out, color);
		}
		/* check for trivial case of top-flat triangle */
		else if (p[0].y() == p[1].y())
		{
			fillTopFlatTriangle(p[0], p[1], p[2], out, color);
		}
		else
		{
			/* general case - split the triangle in a topflat and bottom-flat one */
			Point v4((signed_integral)(p[0].x() + ((float)(p[1].y() - p[0].y()) / (float)(p[2].y() - p[0].y())) * (p[2].x() - p[0].x())), p[1].y());
			fillBottomFlatTriangle(p[0], p[1], v4, out, color);
			fillTopFlatTriangle(p[1], v4, p[2], out, color);
		}
	}





	/**
	Generic fill polygon function.
	For each points inside given polygon, call the functor \a f with arguments (signed_integral x, signed_integral y).
	If rect is not null, the functor will only be called for points inside \a rect.
	Returns the number of filled points.

	NOTE: this function also works for 1 point polygons (draw a single point) or 2 points polygons (draw a line).
	*/
	template< class Functor>
	size_t fillPolygonFunctor(const Point* pts, size_t size, Functor& f, const Rect& rect)
	{
		if (size == 0)
			return 0;
		else if (size == 1) {
			// Fill point
			size_t res = 0;
			if (rect.isNull() || rect.contains(pts[0])) {
				f(pts[0].x(), pts[0].y());
				++res;
			}
			return res;
		}
		else if (size == 2) {
			size_t res = 0;
			// Fill line
			Line line(pts[0], pts[1]);
			if (line.dx() == 0) {
				signed_integral stepy = (line.dy() > 0 ? 1 : -1);
				for (signed_integral y = line.y1(); y != line.y2(); y += stepy) {
					if (rect.isNull() || rect.contains(line.x1(), y)) {
						f(line.x1(), y);
						++res;
					}
				}
				if (rect.isNull() || rect.contains(pts[1].x(), pts[1].y())) {
					f(pts[1].x(), pts[1].y());
					++res;
				}
				return res;
			}
			else if (line.dy() == 0) {
				signed_integral stepx = (line.dx() > 0 ? 1 : -1);
				for (signed_integral x = line.x1(); x != line.x2(); x += stepx) {
					if (rect.isNull() || rect.contains(x, line.y1())) {
						f(x, line.y1());
						++res;
					}
				}
				if (rect.isNull() || rect.contains(pts[1].x(), pts[1].y())) {
					f(pts[1].x(), pts[1].y());
					++res;
				}
				return res;
			}
			else {
				//use affine function to extract pixels
				double a = (double(line.dy()) / double(line.dx()));
				double b = double(line.y1()) - a * line.x1();

				//loop on dx or dy
				if (std::abs(line.dx()) > std::abs(line.dy()))
				{
					//loop on dx
					signed_integral stepx = (line.dx() > 0 ? 1 : -1);
					for (signed_integral x = line.x1(); x != line.x2(); x += stepx) {
						signed_integral y = (signed_integral)std::round(x * a + b);
						if (rect.isNull() || rect.contains(x, y)) {
							f(x, y);
							++res;
						}
					}
				}
				else
				{
					//loop on dy
					signed_integral stepy = (line.dy() > 0 ? 1 : -1);
					for (signed_integral y = line.y1(); y != line.y2(); y += stepy) {
						signed_integral x = (signed_integral)std::round((y - b) / a);
						if (rect.isNull() || rect.contains(x, y)) {
							f(x, y);
							++res;
						}
					}
				}
				if (rect.isNull() || rect.contains(pts[1].x(), pts[1].y())) {
					f(pts[1].x(), pts[1].y());
					++res;
				}
				return res;
			}
		}


		//Works for all polygons, simplier and faster than delaunay based method
		//see https://developer.blender.org/diffusion/B/browse/master/source/blender/blenlib/intern/math_geom.c;3b4a8f1cfa7339f3db9ddd4a7974b8cc30d7ff0b$2411
		//and http://alienryderflex.com/polygon_fill/

		Rect pr = polygonRect(pts, size);
		if (!rect.isNull()) {
			if (pr.xmax <= rect.xmin || pr.xmin >= rect.xmax || pr.ymax <= rect.ymin || pr.ymin >= rect.ymax)
				//polygon does not intersect rect
				return 0;

			if (pr.xmin < rect.xmin) pr.xmin = rect.xmin;
			if (pr.xmax > rect.xmax) pr.xmax = rect.xmax;
			if (pr.ymin < rect.ymin) pr.ymin = rect.ymin;
			if (pr.ymax > rect.ymax) pr.ymax = rect.ymax;
		}

		size_t res = 0;

		/* originally by Darel Rex Finley, 2007 */

		signed_integral  nodes, pixel_y, i, j, nr = size;
		std::vector<signed_integral> node_x((size_t)(nr + 1), 0);

		/* Loop through the rows of the image. */
		for (pixel_y = pr.ymin; pixel_y < pr.ymax; pixel_y++) {

			/* Build a list of nodes. */
			nodes = 0; j = nr - 1;
			if (pixel_y > pr.ymin) {
				for (i = 0; i < nr; i++) {
					if (((pts[i].y() < pixel_y && pts[j].y() >= pixel_y) ||
						(pts[j].y() < pixel_y && pts[i].y() >= pixel_y)) && pts[j].y() != pts[i].y()) {
						node_x[nodes++] = (signed_integral)std::round(pts[i].x() +
							((double)(pixel_y - pts[i].y()) / (pts[j].y() - pts[i].y())) *
							(pts[j].x() - pts[i].x()));
					}
					j = i;
				}
			}
			else {
				//first row: different treatment
				for (i = 0; i < nr; i++) {
					if (((pts[i].y() <= pixel_y && pts[j].y() >= pixel_y) ||
						(pts[j].y() <= pixel_y && pts[i].y() >= pixel_y)) && pts[j].y() != pts[i].y()) {
						node_x[nodes++] = (signed_integral)std::round(pts[i].x() +
							((double)(pixel_y - pts[i].y()) / (pts[j].y() - pts[i].y())) *
							(pts[j].x() - pts[i].x()));
					}
					j = i;
				}
			}

			/* Sort the nodes, via a simple "Bubble" sort. */
			i = 0;
			while (i < nodes - 1) {
				if (node_x[i] > node_x[i + 1]) {
					std::swap(node_x[i], node_x[i + 1]);
					if (i) i--;
				}
				else {
					i++;
				}
			}

			if (pixel_y == pr.ymin) {
				//remove duplicates, only for the first row
				signed_integral snodes = nodes;
				nodes = 1;
				for (i = 1; i < snodes; ++i) {
					if (node_x[i] != node_x[i - 1])
						node_x[nodes++] = node_x[i];
				}
			}

			/* Fill the pixels between node pairs. */
			for (i = 0; i < nodes; i += 2) {

				if (node_x[i] >= pr.xmax) break;
				if (node_x[i + 1] >= pr.xmin) {
					if (node_x[i] < pr.xmin) node_x[i] = pr.xmin;
					if (node_x[i + 1] >= pr.xmax) node_x[i + 1] = pr.xmax - 1;
					for (j = node_x[i]; j <= node_x[i + 1]; j++) {
						f(j, pixel_y);
						++res;
					}
				}
			}
		}

		return res;
	}


	namespace detail
	{
		template<class T>
		struct FillImage
		{
			Array2DView<T> img;
			T color;
			void operator()(signed_integral x, signed_integral y) {
				img((size_t)y, (size_t)x) = color;
			}
		};

	}

	/**
	Draw given polygon on \a img with given \a color.
	Returns the number of drawn points.
	Note that the polygon might contain points outside destination image.
	*/
	template< class T>
	size_t drawPolygon(Array2DView<T> img, const Point* pts, size_t size, T color)
	{
		detail::FillImage<T> fill;
		fill.img = img;
		fill.color = color;
		return fillPolygonFunctor(pts, size, fill, Rect(0, (signed_integral)img.width, 0, (signed_integral)img.height));
	}

	/**
	Returns the polygon area in pixels.
	*/
	GEOMETRY_EXPORT size_t polygonArea(const Point* pts, size_t size, const Rect& r = Rect());

}
