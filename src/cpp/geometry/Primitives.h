#pragma once

#define _USE_MATH_DEFINES 
#include <cmath>
#include <vector>
#include <algorithm>
#include <limits>
#include <atomic>
#include <mutex>
#include <chrono>
#include <thread>

#include "Misc.h"

/** @file

Defines several primitive classes/functions to work with points, rectangles and polygons
*/


namespace rir
{
	using unsigned_integral = size_t; // usually 32 bits on 32 bits platform, 64 bits on 64 bits platform
	using signed_integral = typename std::make_signed<size_t>::type;

	/**
	Straightforward absolute value
	*/
	template <class T>
	inline T fastAbs(const T& t) { return t >= 0 ? t : -t; }

	/**
	Numeric equality comparison for integer types
	*/
	template <class T>
	static inline bool fuzzyCompare(const T& p1, const T& p2)
	{
		return p1 == p2;
	}
	/**
	Numeric fuzzy equality comparison for double type
	*/
	static inline bool fuzzyCompare(double p1, double p2)
	{
		return (fastAbs(p1 - p2) * 1000000000000. <= std::min(fastAbs(p1), fastAbs(p2)));
	}
	/**
	Numeric fuzzy equality comparison for float type
	*/
	static inline bool fuzzyCompare(float p1, float p2)
	{
		return (fastAbs(p1 - p2) * 100000.f <= std::min(fastAbs(p1), fastAbs(p2)));
	}

	/**
	Equality comparison to 0 for integral types
	*/
	template <class T>
	static inline bool fuzzyIsNull(const T& d)
	{
		return d == (T)0;
	}
	/**
	Fuzzy equality comparison to 0 for double type
	*/
	static inline bool fuzzyIsNull(double d)
	{
		return fastAbs(d) <= 0.000000000001;
	}
	/**
	Fuzzy equality comparison to 0 for float type
	*/
	static inline bool fuzzyIsNull(float f)
	{
		return fastAbs(f) <= 0.00001f;
	}



	/**
	Point class
	*/
	template<class T>
	struct _Point
	{
		T _x, _y;
		_Point(T __x = 0, T __y = 0)
			:_x(__x), _y(__y) {}
		template<class U>
		_Point(const _Point<U>& other)
			: _x((T)other.x()), _y((T)other.y()) {}
		T x() const { return _x; }
		T y() const { return _y; }
		T& rx() { return _x; }
		T& ry() { return _y; }

		bool operator==(const _Point<T>& other) const { return _x == other.x() && _y == other.y(); }
		bool operator!=(const _Point<T>& other) const { return _x != other.x() || _y != other.y(); }
		bool operator < (const _Point<T>& other) const {
			if (y() < other.y()) return true;
			if (y() == other.y() && x() < other.x()) return true;
			return false;
		}
	};
	typedef _Point<signed_integral> Point;
	typedef _Point<double> PointF;

	/**
	Line class
	*/
	template< class T>
	struct _Line
	{
		_Point<T> _p1, _p2;
		_Line(const _Point<T>& p1 = _Point<T>(), const _Point<T>& p2 = _Point<T>())
			:_p1(p1), _p2(p2) {}

		const _Point<T>& p1() const { return _p1; }
		const _Point<T>& p2() const { return _p2; }
		T dx() const { return _p2.x() - _p1.x(); }
		T dy() const { return _p2.y() - _p1.y(); }
		T x1() const { return _p1.x(); }
		T x2() const { return _p2.x(); }
		T y1() const { return _p1.y(); }
		T y2() const { return _p2.y(); }

		bool isNull() const {
			return fuzzyCompare(_p1.x(), _p2.x()) && fuzzyCompare(_p1.y(), _p2.y());
		}

		/**
		Returns the angle of the line in degrees.

		The return value will be in the range of values from 0.0 up to but not
		including 360.0. The angles are measured counter-clockwise from a point
		on the x-axis to the right of the origin (x > 0).
		*/
		double angle() const {
			const double dx = _p2.x() - _p1.x();
			const double dy = _p2.y() - _p1.y();
			const double theta = std::atan2(-dy, dx) * 360.0 / (M_PI * 2.);
			const double theta_normalized = theta < 0 ? theta + 360 : theta;
			if (fuzzyCompare(theta_normalized, double(360)))
				return double(0);
			else
				return theta_normalized;
		}
		/**
		Returns the angle (in degrees) from this line to the given \a
		line, taking the direction of the lines into account. If the lines
		do not intersect within their range, it is the intersection point of
		the extended lines that serves as origin (see
		QLineF::UnboundedIntersection).

		The returned value represents the number of degrees you need to add
		to this line to make it have the same angle as the given \a line,
		going counter-clockwise.
		*/
		double angleTo(const _Line<T>& l) const {
			if (isNull() || l.isNull())
				return 0;
			const double a1 = angle();
			const double a2 = l.angle();
			const double delta = a2 - a1;
			const double delta_normalized = delta < 0 ? delta + 360 : delta;
			if (fuzzyCompare(delta, double(360)))
				return 0;
			else
				return delta_normalized;
		}
	};

	typedef _Line<signed_integral> Line;
	typedef _Line<double> LineF;

	template< class T> inline _Point<T> operator+(const _Point<T>& p1, const _Point<T>& p2) { return _Point<T>(p1.x() + p2.x(), p1.y() + p2.y()); }
	template< class T> inline _Point<T> operator+(const _Point<T>& p1, T p2) { return _Point<T>(p1.x() + p2, p1.y() + p2); }
	template< class T> inline _Point<T> operator+(T p1, const _Point<T>& p2) { return _Point<T>(p1 + p2.x(), p1 + p2.y()); }
	template< class T> inline _Point<T>& operator+=(_Point<T>& p1, const _Point<T>& p2) { p1.rx() += p2.x(); p1.ry() += p2.y(); return p1; }
	template< class T> inline _Point<T>& operator+=(_Point<T>& p1, T p2) { p1.rx() += p2; p1.ry() += p2; return p1; }

	template< class T> inline _Point<T> operator-(const _Point<T>& p1, const _Point<T>& p2) { return _Point<T>(p1.x() - p2.x(), p1.y() - p2.y()); }
	template< class T> inline _Point<T> operator-(const _Point<T>& p1, T p2) { return _Point<T>(p1.x() - p2, p1.y() - p2); }
	template< class T> inline _Point<T>& operator-=(_Point<T>& p1, const _Point<T>& p2) { p1.rx() -= p2.x(); p1.ry() -= p2.y(); return p1; }
	template< class T> inline _Point<T>& operator-=(_Point<T>& p1, T p2) { p1.rx() -= p2; p1.ry() -= p2; return p1; }

	template< class T> inline _Point<T> operator*(const _Point<T>& p1, const _Point<T>& p2) { return _Point<T>(p1.x() * p2.x(), p1.y() * p2.y()); }
	template< class T> inline _Point<T> operator*(const _Point<T>& p1, T p2) { return _Point<T>(p1.x() * p2, p1.y() * p2); }
	template< class T> inline _Point<T> operator*(T p1, const _Point<T>& p2) { return _Point<T>(p1 * p2.x(), p1 * p2.y()); }
	template< class T> inline _Point<T>& operator*=(_Point<T>& p1, const _Point<T>& p2) { p1.rx() *= p2.x(); p1.ry() *= p2.y(); return p1; }
	template< class T> inline _Point<T>& operator*=(_Point<T>& p1, T p2) { p1.rx() *= p2; p1.ry() *= p2; return p1; }

	template< class T> inline _Point<T> operator/(const _Point<T>& p1, const _Point<T>& p2) { return _Point<T>(p1.x() / p2.x(), p1.y() / p2.y()); }
	template< class T> inline _Point<T> operator/(const _Point<T>& p1, T p2) { return _Point<T>(p1.x() / p2, p1.y() / p2); }
	template< class T> inline _Point<T>& operator/=(_Point<T>& p1, const _Point<T>& p2) { p1.rx() /= p2.x(); p1.ry() /= p2.y(); return p1; }
	template< class T> inline _Point<T>& operator/=(_Point<T>& p1, T p2) { p1.rx() /= p2; p1.ry() /= p2; return p1; }

	/**
	* Reutns the Euclidean distance between 2 points
	*/
	template< class T>
	inline double pointDist(const _Point<T>& p1, const _Point<T>& p2)
	{
		double x = p2.x() - p1.x();
		double y = p2.y() - p1.y();
		return std::sqrt(x * x + y * y);
	}


	/**
	* Rectangle class
	*/
	template< class T>
	struct _Rect
	{
		typedef _Point<T> point;

		T xmin, xmax, ymin, ymax;
		_Rect(T xi = 0, T xa = 0, T yi = 0, T ya = 0)
			:xmin(xi), xmax(xa), ymin(yi), ymax(ya)
		{}
		_Rect normalized() const {
			_Rect r = *this;
			if (r.xmax < r.xmin) std::swap(r.xmax, r.xmin);
			if (r.ymax < r.ymin) std::swap(r.ymax, r.ymin);
			return r;
		}
		PointF center() const { return PointF(xmin + width() / 2, ymin + height() / 2); }
		T width() const { return xmax - xmin; }
		T height() const { return ymax - ymin; }
		T area() const { return width() * height(); }

		point topLeft() const { 
			return point(xmin, ymin); 
		}
		point topRight() const { 
			return point(xmax, ymin);
		}
		point bottomLeft() const { 
			return point(xmin, ymax); 
		}
		point bottomRight() const { 
			return point(xmax, ymax);
		}

		bool isNull() const { return xmin == xmax && ymin == ymax; }
		bool isEmpty() const { return xmax <= xmin || ymax <= ymin; }
		bool isValid() const { return xmax > xmin && ymax > ymin; }
		bool contains(T x, T y) const {
			return x >= xmin && y >= ymin && x < xmax&& y < ymax;
		}
		bool contains(const point& pt) const {
			return contains(pt.x(), pt.y());
		}
		_Rect intersect(const _Rect& other) const {
			_Rect r;
			r.xmin = std::max(xmin, other.xmin);
			r.xmax = std::min(xmax, other.xmax);
			if (r.xmax < r.xmin) return _Rect();
			r.ymin = std::max(ymin, other.ymin);
			r.ymax = std::min(ymax, other.ymax);
			if (r.ymax < r.ymin) return _Rect();
			return r;
		}
		_Rect unite(const _Rect& other) const {
			_Rect r;
			r.xmin = std::min(xmin, other.xmin);
			r.xmax = std::max(xmax, other.xmax);
			r.ymin = std::min(ymin, other.ymin);
			r.ymax = std::max(ymax, other.ymax);
			return r;
		}
	};

	typedef _Rect<signed_integral> Rect;
	typedef _Rect<double> RectF;

	/**
	* Extract polygon bounding rectangle.
	*/
	template<class T>
	inline _Rect<T> polygonRect(const _Point<T>* pts, size_t size)
	{
		if (size == 0) return _Rect<T>();
		_Rect<T> r;
		r.xmin = pts[0].x();
		r.ymin = pts[0].y();
		r.xmax = pts[0].x();
		r.ymax = pts[0].y();
		for (size_t i = 1; i < size; ++i) {
			r.xmin = std::min(r.xmin, pts[i].x());
			r.xmax = std::max(r.xmax, pts[i].x());
			r.ymin = std::min(r.ymin, pts[i].y());
			r.ymax = std::max(r.ymax, pts[i].y());
		}
		if (std::is_integral<T>::value) {
			r.xmax += 1;
			r.ymax += 1;
		}
		return r;
	}
	template<class T>
	inline _Rect<T> polygonRect(const std::vector<_Point<T> >& poly)
	{
		return polygonRect(poly.data(), poly.size());
	}

	/**
	Straightforward 2*2 matrix class
	*/
	struct Matrix
	{
		double m11, m12, m21, m22;
		Matrix(double _m11 = 1, double _m12 = 0, double _m21 = 0, double _m22 = 1)
			:m11(_m11), m12(_m12), m21(_m21), m22(_m22)
		{}
		bool isInvertible() const { return !(std::abs(m11 * m22 - m12 * m21) <= 0.000000000001); }
		double determinant() const { return m11 * m22 - m12 * m21; }
		Matrix inverted(bool* invertible) const
		{
			double dtr = determinant();
			if (dtr == 0.0) {
				if (invertible)
					*invertible = false;                // singular matrix
				return Matrix();
			}
			else {                                        // invertible matrix
				if (invertible)
					*invertible = true;
				double dinv = 1.0 / dtr;
				return Matrix((m22 * dinv), (-m12 * dinv),
					(-m21 * dinv), (m11 * dinv));
			}
		}
	};

	/**
	Simple size representation for images and 2D arrays
	*/
	struct Size
	{
		
		Size(signed_integral w = -1, signed_integral h = -1)
			:width(w), height(h) {}
		signed_integral width;
		signed_integral height;
	};

	/**Vector of points*/
	typedef std::vector<PointF> SampleVector;
	/**Floating point polygon*/
	typedef std::vector<PointF> PolygonF;
	/**Integer polygon*/
	typedef std::vector<Point> Polygon;



	/// @brief Lightweight and fast spinlock implementation based on https://rigtorp.se/spinlock/
	///
	/// spinlock is a lightweight spinlock implementation following the TimedMutex requirements.
	///  
	class spinlock {

		spinlock(const spinlock&) = delete;
		spinlock(spinlock&&) = delete;
		spinlock& operator=(const spinlock&) = delete;
		spinlock& operator=(spinlock&&) = delete;

		std::atomic<bool> d_lock;

	public:
		spinlock() : d_lock(0) {}

		void lock() noexcept {
			for (;;) {
				// Optimistically assume the lock is free on the first try
				if (!d_lock.exchange(true, std::memory_order_acquire)) {
					return;
				}

				// Wait for lock to be released without generating cache misses
				while (d_lock.load(std::memory_order_relaxed)) {
					// Issue X86 PAUSE or ARM YIELD instruction to reduce contention between
					// hyper-threads

					std::this_thread::yield();
				}
			}
		}

		bool is_locked() const noexcept { return d_lock.load(std::memory_order_relaxed); }
		bool try_lock() noexcept {
			// First do a relaxed load to check if lock is free in order to prevent
			// unnecessary cache misses if someone does while(!try_lock())
			return !d_lock.load(std::memory_order_relaxed) &&
				!d_lock.exchange(true, std::memory_order_acquire);
		}

		void unlock() noexcept {
			d_lock.store(false, std::memory_order_release);
		}

		template<class Rep, class Period>
		bool try_lock_for(const std::chrono::duration<Rep, Period>& duration) noexcept
		{
			return try_lock_until(std::chrono::system_clock::now() + duration);
		}

		template<class Clock, class Duration>
		bool try_lock_until(const std::chrono::time_point<Clock, Duration>& timePoint) noexcept
		{
			for (;;) {
				if (!d_lock.exchange(true, std::memory_order_acquire))
					return true;

				while (d_lock.load(std::memory_order_relaxed)) {
					if (std::chrono::system_clock::now() > timePoint)
						return false;
					std::this_thread::yield();
				}
			}
		}
	};


}
