#pragma once

#include "Primitives.h"

#include <map>

/** @file

Defines several utility functions to work with polygons
*/

namespace rir
{
	/**
	* Reorder polygon by making index new_start the new polygon start
	*/
	template<class T>
	std::vector<_Point<T> > reorderPolygon(const std::vector<_Point<T> >& p, size_t new_start)
	{
		std::vector<PointF> res(p.size());
		size_t pos = (size_t)new_start;
		for (size_t i = 0; i < p.size(); ++i, ++pos) {
			if (pos >= p.size())
				pos = 0;
			res[i] = p[pos];
		}
		return res;
	}

	/**
	* Reverse polygon
	*/
	template<class T>
	std::vector<_Point<T> > reversePolygon(const std::vector<_Point<T> >& poly)
	{
		if (poly.empty()) return poly;
		std::vector<_Point<T> > res(poly.size());
		std::copy(poly.begin(), poly.end(), res.rbegin());
		return res;
	}

	/**
	* Close the polygon if necessary (last point == first point)
	*/
	template<class T>
	void closePolygon(std::vector<_Point<T> >& poly)
	{
		if (poly.empty()) return;
		if (poly.back() != poly.front())
			poly.push_back(poly.front());
	}

	/**
	* Open the polygon if necessary (last point != first point)
	*/
	template<class T>
	void openPolygon(std::vector<_Point<T> >& poly)
	{
		if (poly.empty()) return ;
		if (poly.back() == poly.front())
			poly.pop_back();
	}

	/**
	* Remove points from polygon that do not change the overall shape (basically points within a straigth line).
	*/
	template<class T>
	std::vector<_Point<T> > simplifyPolygon(const std::vector<_Point<T> >& polygon)
	{
		if (polygon.size() < 3)
			return polygon;

		std::vector<_Point<T> > res;
		res.push_back(polygon.front());

		double angle = 0;
		for (size_t i = 1; i < polygon.size() - 1; ++i) {
			if (polygon[i] == polygon[i - 1])
				continue;
			angle = LineF(polygon[i - 1], polygon[i]).angle();
			double new_angle;
			while (i < polygon.size() - 1 && fuzzyCompare(new_angle = LineF(polygon[i], polygon[i + 1]).angle(), angle))
				++i;
			if (i < polygon.size()) {
				res.push_back(polygon[i]);
				angle = new_angle;
			}
		}

		if (polygon.front() == polygon.back()) {
			if (res.back() != polygon.back() && !fuzzyCompare(LineF(polygon[polygon.size() - 2], polygon[polygon.size() - 1]).angle(), angle))
				res.push_back(polygon.back());
		}
		else {
			const _Point<T>& pt1 = polygon[polygon.size() - 1];
			const _Point<T>& pt2 = polygon.front();
			if (res.back() != polygon.back() && !fuzzyCompare(LineF(pt1, pt2).angle(), angle))
				res.push_back(polygon.back());
		}

		return res;
	}
	
	/**
	* Check if polygon is rectangular
	*/
	template<class T>
	bool polygonIsRect(const std::vector<_Point<T> >& _p, _Rect<T>* rect)
	{
		if (_p.size() < 4) {
			if (rect) *rect = _Rect<T>();
			return false;
		}

		const std::vector<_Point<T> > p = simplifyPolygon(_p);

		T posx[2] = { p.front().x(),0 };
		T posy[2] = { p.front().y(),0 };
		int posxcount = 1;
		int posycount = 1;

		for (size_t i = 1; i < p.size(); ++i) {
			const _Point<T>& pt = p[i];

			if (posxcount == 1) {
				if (pt.x() != posx[0]) {
					posx[1] = pt.x();
					++posxcount;
				}
			}
			else { //posxcount == 2
				if (pt.x() != posx[0] && pt.x() != posx[1]) {
					if (rect) *rect = _Rect<T>();
					return false;
				}
			}

			if (posycount == 1) {
				if (pt.y() != posy[0]) {
					posy[1] = pt.y();
					++posycount;
				}
			}
			else { //posycount == 2
				if (pt.y() != posy[0] && pt.y() != posy[1]) {
					if (rect) *rect = _Rect<T>();
					return false;
				}
			}

		}

		if (posxcount == 1)
			posx[1] = posx[0];
		else {
			if (posx[0] > posx[1])
				std::swap(posx[0], posx[1]);
		}
		if (posycount == 1)
			posy[1] = posy[0];
		else {
			if (posy[0] > posy[1])
				std::swap(posy[0], posy[1]);
		}

		if (rect) *rect = _Rect<T>(posx[0], posy[0], posx[1] , posy[1]);
		return true;
	}

	/**
	* Remove consecutive duplicated points from polygon
	*/
	template<class T>
	std::vector<_Point<T> >  removeConsecutiveDuplicates(const std::vector<_Point<T> >& poly)
	{
		std::vector<_Point<T> >  res;
		res.reserve(poly.size());
		res.push_back(poly.front());
		for (size_t i = 1; i < poly.size(); ++i) {
			if (poly[i] != poly[i - 1]) {
				res.push_back(poly[i]);
			}
		}
		return res;
	}

	/**
	* Round floating point polygon
	*/
	inline std::vector<Point> roundPolygon(const std::vector<PointF>& p)
	{
		std::vector<Point> res(p.size());
		for (size_t i = 0; i < p.size(); ++i) {
			res[i] = Point((signed_integral)std::round(p[i].x()), (signed_integral)std::round(p[i].y()));
		}
		return res;
	}

	/**
	* Returns true if given polygon is in clockwise order, false otherwise.
	*/
	template< class T >
	bool isPolygonClockwise(const std::vector<_Point<T> >& poly)
	{
		double signedArea = 0.0;
		for (size_t i = 0; i < poly.size(); ++i) {
			const _Point<T>& point = poly[i];
			const _Point<T>& next = i == poly.size() - 1 ? poly[0] : poly[i + 1];
			signedArea += (point.x() * next.y() - next.x() * point.y());
		}
		return signedArea < 0;
	}

	/**
	* Returns the polygon centroid
	*/
	template< class T >
	PointF polygonCentroid(const std::vector<_Point<T> >& poly)
	{
		double A = polygonArea(poly);
		double gx = 0;
		double gy = 0;
		for (size_t i = 0; i < poly.size() - 1; ++i) {

			gx += (poly[i].x() + poly[i + 1].x()) * (poly[i].x() * poly[i + 1].y() - poly[i + 1].x() * poly[i].y());
			gy += (poly[i].y() + poly[i + 1].y()) * (poly[i].x() * poly[i + 1].y() - poly[i + 1].x() * poly[i].y());
		}
		gx /= 6 * A;
		gy /= 6 * A;
		return PointF(gx, gy);
	}

	/**
	* Set the polygon orientation (clockwise or anti-clockwise).
	* This function reverses the polygon if its orientation is not the right one.
	*/
	template< class T >
	std::vector<_Point<T> > setPolygonOrientation(const std::vector<_Point<T> >& poly, bool clockwise)
	{
		if (poly.empty()) return poly;
		if (isPolygonClockwise(poly)) {
			if (clockwise)
				return poly;
			else
				return reversePolygon(poly);
		}
		else {
			if (clockwise)
				return reversePolygon(poly);
			else
				return poly;
		}
	}


	/**
	* Compute polygon area using the Shoelace Algorithm
	*/
	template<class T>
	double polygonArea(const std::vector<_Point<T> >& poly)
	{
		//see https://www.101computing.net/the-shoelace-algorithm/#:~:text=The%20shoelace%20formula%20or%20shoelace,polygon%20to%20find%20its%20area.
		const std::vector<_Point<T> > p = setPolygonOrientation(poly, false);
		double sum1 = 0;
		double sum2 = 0;

		for (size_t i = 0; i < poly.size() - 1; ++i) {
			sum1 += poly[i].x() * poly[i + 1].y();
			sum2 += poly[i].y() * poly[i + 1].x();
		}

		// Add xn.y1
		sum1 += poly.back().x() * poly[0].y();
		// Add x1.yn
		sum2 += poly[0].x() * poly.back().y();

		double area = std::abs(sum1 - sum2) / 2;
		return area;
	}


	/*
	Return True if the polynomial defined by the sequence of 2D
	points is not concave: points are valid (maybe duplicated), interior angles are strictly between zero and a straight
	angle, and the polygon does not intersect itself.

	NOTES:  Algorithm: the signed changes of the direction angles
				from one side to the next side must be all positive or
				all negative (or null), and their sum must equal plus-or-minus
				one full turn (2 pi radians).
	*/
	template<class T>
	bool isPolygonNonConcave(const std::vector<_Point<T> >& poly)
	{
		//https://stackoverflow.com/questions/471962/how-do-i-efficiently-determine-if-a-polygon-is-convex-non-convex-or-complex

	// Check for too few points
		if (poly.size() < 4)
			return true;

		// Get starting information
		_Point<T> old = poly[poly.size() - 2];
		_Point<T> new_ = poly.back();
		double new_direction = std::atan2(new_.y() - old.y(), new_.x() - old.x());
		double angle_sum = 0.0;
		double orientation = 0;
		// Check each point (the side ending there, its angle) and accum. angles
		for (size_t i = 0; i < poly.size(); ++i) {
			const _Point<T> newpoint = poly[i];
			if (newpoint == new_)
				continue;
			// Update point coordinates and side directions, check side length
			double old_direction = new_direction;
			old = new_;
			new_ = newpoint;
			new_direction = std::atan2(new_.y() - old.y(), new_.x() - old.x());
			if (old.x() == new_.x() && old.y() == new_.y())
				return false;  // repeated consecutive points
			// Calculate & check the normalized direction-change angle
			double angle = new_direction - old_direction;
			if (angle <= -M_PI)
				angle += (M_PI * 2.);  // make it in half-open interval (-Pi, Pi]
			else if (angle > M_PI)
				angle -= (M_PI * 2.);
			if (orientation == 0) {  // if first time through loop, initialize orientation
				if (angle == 0.0)
					continue;//return false;
				orientation = angle > 0. ? 1.0 : -1.0;
			}
			else {  // if other time through loop, check orientation is stable
				if (orientation * angle <= 0.0)  // not both pos. or both neg.
					return false;
			}
			// Accumulate the direction-change angle
			angle_sum += angle;
		}
		// Check that the total number of full turns is plus-or-minus 1
		return std::abs(std::round(angle_sum / (M_PI * 2.))) == 1;
	}




	/**
	* Returns the minimum distance between a point and a segment.
	*/
	GEOMETRY_EXPORT double distanceToSegment(double x, double y, double x1, double y1, double x2, double y2);
	/**
	* Overload function.
	* Returns the minimum distance between a point and a segment.
	*/
	template<class T>
	double distanceToSegment(const _Point<T>& pt, const _Line<T>& segment)
	{
		return distanceToSegment(pt.x(), pt.y(), segment.p1().x(), segment.p1().y(), segment.p2().x(), segment.p2().y());
	}



	/**
	* Interpolate 2 polygons based on the advance parameter [0,1].
	* If advance == 0, p1 is returned as is, and if advance ==1 p2 is returned as is.
	*
	* The interpolated polygon is guaranteed to have a maximum size of (p1.size()+p2.size() -2)
	*/
	GEOMETRY_EXPORT PolygonF interpolatePolygons(const PolygonF& p1, const PolygonF& p2, double advance);


	/**
	* Simplify given polygon using Ramer-Douglas-Peucker algorithm (see https://github.com/prakol16/rdp-expansion-only for more details).
	* @param  epsilon: epsilon value used to simplify the polygon by the Ramer-Douglas-Peucker algorithm.
	*/
	GEOMETRY_EXPORT PolygonF RDPSimplifyPolygon(const PolygonF& polygon, double epsilon);


	/**
	* Simplify given polygon using Ramer-Douglas-Peucker algorithm (see https://gist.github.com/msbarry/9152218 for more details).
	* @param  max_points: unlinke RDPSimplifyPolygon, this function takes a maximum number of points as input.
	*/
	GEOMETRY_EXPORT PolygonF RDPSimplifyPolygon2(const PolygonF& polygon, size_t max_points);

	/**
	* Returns the convex hull polygon of given points.
	* The result is in anti-clockwise order, and not necessarly closed.
	* This algorithm performs in O(n*log(n)).
	*/
	GEOMETRY_EXPORT PolygonF convexHull(const PolygonF& poly);



	/**
	* Oriented rect structure
	*/
	struct OrientedRect {
		PolygonF boundingPoints;
		PolygonF hullPoints;
		PointF center;
		// smaller box side
		double width;
		// larger box side
		double height;
		// angle between smaller box side and X axis in radians,
		// positive value means box orientation from bottom right to top left,
		// negative value means opposite
		double widthAngle;
		// angle between larger box side and X axis in radians
		// positive value means box orientation from bottom left to top right,
		// negative value means opposite
		double heightAngle;

		OrientedRect(const PolygonF& bp = PolygonF(), const PolygonF& hp = PolygonF(), const PointF& c = PointF(),
			double w = 0, double h = 0, double wa = 0, double ha = 0) :
			boundingPoints(bp), hullPoints(hp), center(c), width(w), height(h), widthAngle(wa), heightAngle(ha) {}
	};

	/**
	* Returns the minimum area oriented bounding box around a set of points.
	*
	* This function is based the convex hull of given points.
	* Set check_convex to false if input polygon is already a convex one.
	*/
	GEOMETRY_EXPORT OrientedRect minimumAreaBBox(const PolygonF& poly, bool check_convex = true);



	namespace detail
	{

		static inline Point rotateClockwise45(const Point& pt)
		{
			signed_integral x = (signed_integral)std::round(0.70710678118654757 * pt.x() + -0.70710678118654757 * pt.y());
			signed_integral y = (signed_integral)std::round(0.70710678118654757 * pt.x() + 0.70710678118654757 * pt.y());
			return Point(x, y);
		}

		template< class T>
		static bool checkPoint(signed_integral x, signed_integral y, signed_integral width, signed_integral height, const T* pix, T mask_value)
		{
			//check bounds
			if (x < 0 || y < 0 || x >= width || y >= height) return false;
			if (pix[x + y * width] == mask_value) {
				//this is a foreground pixel, check that it has at least one background pixel neighbor or a border
				if (x == 0 || y == 0 || x == width - 1 || y == height - 1 ||
					pix[x - 1 + y * width] != mask_value || pix[x + 1 + y * width] != mask_value || pix[x + (y - 1) * width] != mask_value || pix[x + (y + 1) * width] != mask_value)
					return true;
			}
			return false;
		}

		template< class T>
		static inline Point nextPoint(const Point& prev, const Point& center, signed_integral width, signed_integral height, const T* pix, T mask_value)
		{
			Point diff = prev - center;

			//start from prev +1
			for (signed_integral i = 0; i < 8; ++i) {
				diff = rotateClockwise45(diff);
				const Point pt = diff + center;
				if (checkPoint(pt.x(), pt.y(), width, height, pix, mask_value))
					return pt;
			}
			//no valid neigbor found: single point
			return center;

		}

		template< class T>
		static void startPoint(Point pt, std::vector<Point>& out, signed_integral width, signed_integral height, const T* pix, T mask_value)
		{
			out.push_back(pt);

			//consider the previous to be the left point
			Point prev = pt - Point(1, 0);

			while (true) {
				//find next point 
				Point tmp = nextPoint(prev, pt, width, height, pix, mask_value);
				prev = pt;
				pt = tmp;

				//the next point is the first encountered one: close the polygon and stop
				if (pt == out.front()) {
					out.push_back(pt);
					break;
				}

				out.push_back(pt);
			}

			if (out.size() == 2) {
				//case single point: close the polygon (3 times the same point)
				out.push_back(out.front());
				return;
			}

			//filter polygon first time by removing all pixels inside vertical/horizontal lines
			if (out.size() > 3) {
				std::vector<Point> res;
				res.push_back(out.front());
				for (size_t i = 1; i < out.size() - 1; ++i) {
					Point pt = out[i];
					if ((pt.x() == out[i - 1].x() && pt.x() == out[i + 1].x()) || (pt.y() == out[i - 1].y() && pt.y() == out[i + 1].y()))
						;//skip point
					else
						res.push_back(out[i]);
				}
				res.push_back(out.back());
				out = res;
			}
		}

	} //end detail

	/**
	* Extract polygon for given mask.
	* Only extract the first encountered object with a pixel value of pix_value.
	*/
	template< class T>
	inline Polygon extractPolygon(const ConstArray2DView<T>& img, T pix_value)
	{
		for (signed_integral y = 0; y < img.height; ++y)
			for (signed_integral x = 0; x < img.width; ++x) {
				T pix = img(y, x);
				if (pix == pix_value) {
					//found pixel value!
					std::vector<Point> poly;
					detail::startPoint(Point(x, y), poly, img.width, img.height, img.values, pix_value);
					return poly;
				}
			}
		return std::vector<Point>();
	}

	/**
	* Extract polygons for given image.
	* Extract polygons for all closed components with a pixel value different from given background value.
	*/
	template< class T>
	inline std::map<signed_integral, Polygon > extractPolygons(const ConstArray2DView<T>& img, T background = 0)
	{
		std::map<signed_integral, std::vector<Point> > res;

		for (signed_integral y = 0; y < img.height; ++y)
			for (signed_integral x = 0; x < img.width; ++x) {
				T pix = img(y, x);
				if (pix != background && res.find(pix) == res.end()) {
					//found new pixel value != background
					std::vector<Point> poly;
					detail::startPoint(Point(x, y), poly, img.width, img.height, img.values, pix);
					res[pix] = poly;
				}
			}
		return res;
	}

}
