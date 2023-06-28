#include "Polygon.h"

extern "C"
{
#include <float.h>
}

namespace rir
{
	double distanceToSegment(double x, double y, double x1, double y1, double x2, double y2)
	{

		double A = x - x1;
		double B = y - y1;
		double C = x2 - x1;
		double D = y2 - y1;

		double dot = A * C + B * D;
		double len_sq = C * C + D * D;
		double param = -1;
		if (len_sq != 0) // in case of 0 length line
			param = dot / len_sq;

		double xx, yy;

		if (param < 0)
		{
			xx = x1;
			yy = y1;
		}
		else if (param > 1)
		{
			xx = x2;
			yy = y2;
		}
		else
		{
			xx = x1 + param * C;
			yy = y1 + param * D;
		}

		double dx = x - xx;
		double dy = y - yy;
		return std::sqrt(dx * dx + dy * dy);
	}

	/*static double prevTime(const double* iter)
	{
		return *(iter - 1);
	}*/

	/**
	\internal
	Extract the time vector for several std::vector<double>.
	Returns a growing time vector containing all different time values of given std::vector<double> samples.
	*/
	inline std::vector<double> extractTimes(const std::vector<double> &v1, const std::vector<double> &v2)
	{
		std::vector<double> res;
		res.reserve(v1.size());

		const double *vectors[2] = {v1.data(), v2.data()};

		size_t vector_count = 2;
		size_t vector_sizes[2] = {(size_t)v1.size(), (size_t)v2.size()};

		// create our time iterators
		std::vector<const double *> iters, ends;
		for (size_t i = 0; i < vector_count; ++i)
		{
			// search for nan time value
			const double *v = vectors[i];
			const size_t size = vector_sizes[i];
			size_t p = 0;
			for (; p < size; ++p)
			{
				if (std::isnan(v[p]))
					break;
			}
			if (p != size)
			{
				// split the vector in 2 vectors
				iters.push_back(vectors[i]);
				ends.push_back(vectors[i]);
				ends.back() += p;
				iters.push_back(vectors[i]);
				iters.back() += p + 1;
				ends.push_back(vectors[i] + vector_sizes[i]);
			}
			else
			{
				iters.push_back(vectors[i]);
				ends.push_back(vectors[i] + vector_sizes[i]);
			}
		}

		while (iters.size())
		{
			// find minimum time among all time vectors
			double min_time = *iters.front();
			for (size_t i = 1; i < iters.size(); ++i)
				min_time = std::min(min_time, *iters[i]);

			// increment each iterator equal to min_time
			for (size_t i = 0; i < iters.size(); ++i)
			{
				if (iters[i] != ends[i])
					if (*iters[i] == min_time)
						if (++iters[i] == ends[i])
						{
							iters.erase(iters.begin() + i);
							ends.erase(ends.begin() + i);
							--i;
						}
			}
			res.push_back(min_time);
		}

		return res;
	}

	static std::vector<PointF> resampleSignal(const double *sample_x, const PointF *sample_y, size_t size, const double *times, size_t times_size)
	{
		std::vector<PointF> res(times_size);

		const double *iters_x = sample_x;
		const double *ends_x = sample_x + size;
		const PointF *iters_y = sample_y;
		// const double* ends_y = sample_y + size;

		for (size_t t = 0; t < times_size; ++t)
		{
			const double time = times[t];

			// we already reached the last sample value
			if (iters_x == ends_x)
			{
				res[t] = sample_y[size - 1];
				continue;
			}

			const double samp_x = *iters_x;
			const PointF samp_y = *iters_y;

			// same time: just add the sample
			if (time == samp_x)
			{
				res[t] = samp_y;
				++iters_x;
				++iters_y;
			}
			// we are before the sample
			else if (time < samp_x)
			{
				// sample starts after
				if (iters_x == sample_x)
				{
					res[t] = samp_y;
				}
				// in between 2 values
				else
				{
					const double *prev_x = iters_x - 1;
					const PointF *prev_y = iters_y - 1;

					double factor = (double)(time - *prev_x) / (double)(samp_x - *prev_x);
					res[t] = samp_y * factor + (1 - factor) * (*prev_y);
				}
			}
			else
			{
				// we are after the sample, increment until this is not the case
				while (iters_x != ends_x && *iters_x < time)
				{
					++iters_x;
					++iters_y;
				}
				if (iters_x != ends_x)
				{
					if (*iters_x == time)
					{
						res[t] = *iters_y;
					}
					else
					{
						const double *prev_x = iters_x - 1;
						const PointF *prev_y = iters_y - 1;

						// interpolation
						double factor = (double)(time - *prev_x) / (double)(*iters_x - *prev_x);
						res[t] = *iters_y * factor + (1 - factor) * *prev_y;
					}
				}
				else
				{
					// reach sample end
					res[t] = sample_y[size - 1];
				}
			}
		}
		return res;
	}

	std::vector<PointF> interpolatePolygons(const std::vector<PointF> &p1, const std::vector<PointF> &p2, double advance)
	{
		if (advance >= 1)
			return p2;
		else if (advance <= 0)
			return p1;
		else if (p1.size() == 0 || p2.size() == 0)
		{
			if (advance < 0.5)
				return p1;
			else
				return p2;
		}

		std::vector<PointF> poly1;
		poly1.push_back(p1.front());
		std::vector<PointF> poly2;
		poly2.push_back(p2.front());

		// remove duplicates
		for (size_t i = 1; i < p1.size(); ++i)
		{
			if (p1[i] != p1[i - 1])
				poly1.push_back(p1[i]);
		}
		for (size_t i = 1; i < p2.size(); ++i)
		{
			if (p2[i] != p2[i - 1])
				poly2.push_back(p2[i]);
		}

		if (poly1.size() == 1)
		{
			PointF pt1 = p1.front();
			// interpolate
			for (size_t i = 0; i < poly2.size(); ++i)
			{
				poly2[i] = pt1 * (1 - advance) + poly2[i] * advance;
			}
			return poly2;
		}
		else if (poly2.size() == 1)
		{
			PointF pt2 = p2.front();
			// interpolate
			for (size_t i = 0; i < poly1.size(); ++i)
			{
				poly1[i] = poly1[i] * (1 - advance) + pt2 * advance;
			}
			return poly1;
		}

		if (isPolygonClockwise(poly1) != isPolygonClockwise(poly2))
		{
			// revsere one of the 2
			size_t size2 = poly1.size() / 2;
			for (size_t i = 0; i < size2; ++i)
			{
				std::swap(poly1[i], poly1[poly1.size() - 1 - i]);
			}
		}

		// move p2's center on the center of p1
		PointF c1 = polygonRect(p1.data(), p1.size()).center();
		PointF c2 = polygonRect(p2.data(), p2.size()).center();
		PointF diff = c2 - c1;
		for (size_t i = 0; i < p2.size(); ++i)
			poly2[i] -= diff;

		// find 2 closest points, which will become the new start point of each polygon
		size_t id1 = 0, id2 = 0;
		double len = std::numeric_limits<double>::max();
		for (size_t i = 0; i < poly1.size(); ++i)
			for (size_t j = 0; j < poly2.size(); ++j)
			{
				double _d = pointDist(poly1[i], poly2[j]);
				if (_d < len)
				{
					len = _d;
					id1 = i;
					id2 = j;
				}
			}

		// reorder
		poly1 = reorderPolygon(poly1, id1);
		poly2 = reorderPolygon(poly2, id2);

		// compute total length
		double tot_len1 = 0;
		double tot_len2 = 0;
		for (size_t i = 1; i < poly1.size(); ++i)
			tot_len1 += pointDist(poly1[i], poly1[i - 1]);
		for (size_t i = 1; i < poly2.size(); ++i)
			tot_len2 += pointDist(poly2[i], poly2[i - 1]);

		std::vector<double> len1, len2;
		len1.push_back(0);
		len2.push_back(0);

		// compute cumulated relative length
		double cum1 = 0;
		for (size_t i = 1; i < poly1.size(); ++i)
		{
			cum1 += pointDist(poly1[i], poly1[i - 1]);
			len1.push_back(cum1 / tot_len1);
		}
		double cum2 = 0;
		for (size_t i = 1; i < poly2.size(); ++i)
		{
			cum2 += pointDist(poly2[i], poly2[i - 1]);
			len2.push_back(cum2 / tot_len2);
		}

		std::vector<double> new_length = extractTimes(len1, len2);
		poly1 = resampleSignal(len1.data(), poly1.data(), poly1.size(), new_length.data(), new_length.size());
		poly2 = resampleSignal(len2.data(), poly2.data(), poly2.size(), new_length.data(), new_length.size());

		// move back p2 to its original center
		for (size_t i = 0; i < poly2.size(); ++i)
			poly2[i] += diff;

		// interpolate
		for (size_t i = 0; i < poly1.size(); ++i)
		{
			poly1[i] = poly1[i] * (1 - advance) + poly2[i] * advance;
		}
		return poly1;
	}

	static inline std::vector<double> line_dists(const PointF *points, size_t size, const PointF &start, const PointF &end)
	{
		// Returns the signed distances of each point to start and end
		if (start == end)
		{
			std::vector<double> res;
			for (size_t i = 0; i < size; ++i)
			{
				PointF diff = points[i] - start;
				res.push_back(std::sqrt(diff.x() * diff.x() + diff.y() * diff.y()));
			}
			return res;
		}

		PointF vec = start - end;
		double vec_norm = std::sqrt(vec.x() * vec.x() + vec.y() * vec.y());

		std::vector<double> res;
		for (size_t i = 0; i < size; ++i)
		{
			PointF diff = start - points[i];
			// cross product
			double cross = vec.x() * diff.y() - vec.y() * diff.x();
			res.push_back(cross / vec_norm);
		}
		return res;
	}

	static inline LineF glue(const LineF &seg1, const LineF &seg2)
	{
		/*
		Glues two segments together
		In other words, given segments Aand B which have endpoints
		that are close together, computes a "glue point" that both segments
		can be extended to in order to intersect.
		Assumes seg1 and seg2 are arrays containing two points each,
		and that if seg1 = [a, b], it can be extended in the direction of b,
		and if seg2 = [c, d], it can be extended in the direction of c.
		*/

		PointF x1 = seg1.p1();
		PointF dir1 = seg1.p2() - x1;
		PointF x2 = seg2.p1();
		PointF dir2 = seg2.p2() - x2;

		// Solve for the vector [t, -s]
		Matrix mat(dir1.x(), dir2.x(), dir1.y(), dir2.y());
		double _t_s[2];
		double *t_s = _t_s;
		bool ok;
		mat = mat.inverted(&ok);
		if (ok)
		{
			PointF diff = x2 - x1;
			t_s[0] = mat.m11 * diff.x() + mat.m12 * diff.y();
			t_s[1] = mat.m21 * diff.x() + mat.m22 * diff.y();
			// Recall that we can't make a segment go in a backwards direction
			// So we must have t >= 0 and s <= 1. However, since we solved for[t, -s]
			// we want that t_s[0] >= 0 and t_s[1] >= -1. If this fails, set t_s to None
			// Also, don't allow segments to more than double
			if (!((0 <= t_s[0] && t_s[0] <= 2) && (-1 <= t_s[1] && t_s[1] <= 1)))
				t_s = NULL;
		}
		else // Singular matrix i.e.parallel
			t_s = NULL;
		if (!t_s)
			// Just connect them with a line
			return LineF(seg1.p2(), seg2.p1());
		else
		{
			PointF res(x1 + dir1 * t_s[0]);
			return LineF(res, res);
		}
	}

	static inline size_t max_index_abs(const std::vector<double> &v)
	{
		double max = std::abs(v.front());
		size_t index = 0;
		for (size_t i = 1; i < v.size(); ++i)
		{
			double tmp = std::abs(v[i]);
			if (tmp > max)
			{
				max = tmp;
				index = i;
			}
		}
		return (size_t)index;
	}
	static inline size_t argmin(const std::vector<double> &v)
	{
		double min = (v.front());
		size_t index = 0;
		for (size_t i = 1; i < v.size(); ++i)
		{
			double tmp = (v[i]);
			if (tmp < min)
			{
				min = tmp;
				index = i;
			}
		}
		return (size_t)index;
	}

	static inline std::vector<PointF> &operator<<(std::vector<PointF> &res, const std::vector<PointF> &v2)
	{
		size_t s = res.size();
		res.resize(res.size() + v2.size());
		std::copy(v2.begin(), v2.end(), res.begin() + s);
		return res;
	}
	static inline std::vector<PointF> &operator<<(std::vector<PointF> &res, const PointF &v2)
	{
		res.push_back(v2);
		return res;
	}
	static inline std::vector<PointF> mid(const std::vector<PointF> &v, size_t start, size_t n = 0)
	{
		if (start >= v.size())
			return std::vector<PointF>();
		if (start + n > v.size() || n == 0)
			n = v.size() - start;
		std::vector<PointF> res(n);
		std::copy(v.begin() + start, v.begin() + (start + n), res.begin());
		return res;
	}

	static inline std::vector<PointF> __rdp(const std::vector<PointF> &points, double epsilon)
	{
		/*
		Computes expansion only rdp assuming a clockwise orientation
		*/
		PointF start = points.front();
		PointF end = points.back();
		std::vector<double> dists = line_dists(points.data(), (size_t)points.size(), start, end);

		// First compute the largest point away from the line just like the ordinary RDP
		size_t index = max_index_abs(dists);
		double dmax = std::abs(dists[index]);
		std::vector<PointF> result;

		if (dmax > epsilon)
		{
			std::vector<PointF> result1 = __rdp(mid(points, 0, index + 1), epsilon);
			std::vector<PointF> result2 = __rdp(mid(points, index), epsilon);
			LineF gl = glue(LineF(result1[result1.size() - 2], result1.back()), LineF(result2[0], result2[1]));
			result = mid(result1, 0, result1.size() - 1);
			if (gl.p1() == gl.p2())
				result << (gl.p1());
			else
				result << gl.p1() << gl.p2();
			result << mid(result2, 1);
		}
		else
		{
			// All points are within epsilon of the line
			// We take the point furthest* outside* the boundary(i.e.with negative distance)
			// and shift the line segment parallel to itself to intersect that point
			PointF new_start = start, new_end = end;
			PointF diff = (end - start);
			double vec_x = diff.x(), vec_y = diff.y();
			double norm = std::sqrt(vec_x * vec_x + vec_y * vec_y);
			if (norm != 0)
			{
				PointF vec_rot90(-vec_y / norm, vec_x / norm);
				// TODO -- verify that this works: can optimize so that if dists[index] < 0, no need to search again, we have index_min = index and dmin = -dmax
				size_t index_min = argmin(dists);
				double dmin = -dists[index_min];
				if (dmin > 0)
				{
					vec_rot90 *= dmin;
					new_start += vec_rot90;
					new_end += vec_rot90;
				}
			}
			result << new_start << new_end;
		}
		return result;
	}

	inline std::vector<PointF> rdp(const std::vector<PointF> &points, double epsilon = 0)
	{
		// see https://github.com/prakol16/rdp-expansion-only
		return __rdp(points, epsilon);
	}

	std::vector<PointF> RDPSimplifyPolygon(const std::vector<PointF> &points, double epsilon)
	{
		// see https://github.com/prakol16/rdp-expansion-only

		/*Ensure that expansion only rdp returns a closed loop at the end*/
		std::vector<PointF> new_points = rdp(points, epsilon);
		LineF glue_pts = glue(LineF(new_points[new_points.size() - 2], new_points.back()), LineF(new_points[0], new_points[1]));
		if (glue_pts.p1() == glue_pts.p2())
		{
			std::vector<PointF> res;
			res << glue_pts.p1() << mid(new_points, 1, new_points.size() - 2) << glue_pts.p1();
			return res;
		}
		else
		{
			return new_points << new_points[0]; // return np.vstack((new_points, [new_points[0]]))
		}
	}

	static double pow2(double x)
	{
		return x * x;
	}

	static double distSquared(const PointF &p1, const PointF &p2)
	{
		return pow2(p1.x() - p2.x()) + pow2(p1.y() - p2.y());
	}

	static double getRatio(const LineF &self, const PointF &point)
	{
		double segmentLength = distSquared(self.p1(), self.p2());
		if (segmentLength == 0)
			return distSquared(point, self.p1());
		return ((point.x() - self.p1().x()) * (self.p2().x() - self.p1().x()) +
				(point.y() - self.p1().y()) * (self.p2().y() - self.p1().y())) /
			   segmentLength;
	}

	static double distanceToSquared(const LineF &self, const PointF &point)
	{
		double t = getRatio(self, point);

		if (t < 0)
			return distSquared(point, self.p1());
		if (t > 1)
			return distSquared(point, self.p2());

		return distSquared(point, PointF(
									  self.p1().x() + t * (self.p2().x() - self.p1().x()),
									  self.p1().y() + t * (self.p2().y() - self.p1().y())));
	}

	/*static double distanceTo(const LineF& self, const PointF& point) {
		return std::sqrt(distanceToSquared(self, point));
	}*/

	static void douglasPeucker(size_t start, size_t end, const PolygonF &points, std::vector<double> &weights)
	{
		if (end > start + 1)
		{
			LineF line(points[start], points[end]);
			double maxDist = -1;
			size_t maxDistIndex = 0;

			for (size_t i = start + 1; i < end; ++i)
			{
				double dist = distanceToSquared(line, points[i]);
				if (dist > maxDist)
				{
					maxDist = dist;
					maxDistIndex = i;
				}
			}
			weights[maxDistIndex] = maxDist;

			douglasPeucker(start, maxDistIndex, points, weights);
			douglasPeucker(maxDistIndex, end, points, weights);
		}
	}

	PolygonF RDPSimplifyPolygon2(const PolygonF &_points, size_t max_points)
	{
		// see https://gist.github.com/msbarry/9152218

		const PolygonF points = removeConsecutiveDuplicates(_points);
		std::vector<double> weights(points.size(), 0.);

		douglasPeucker(0, (size_t)points.size() - 1, points, weights);
		weights[0] = std::numeric_limits<double>::infinity();
		weights.back() = std::numeric_limits<double>::infinity();
		std::vector<double> weightsDescending = weights;
		std::sort(weightsDescending.begin(), weightsDescending.end());
		// reverse
		// for (int i = 0; i < weightsDescending.size() / 2; ++i)
		//	std::swap(weightsDescending[i], weightsDescending[weightsDescending.size() - i - 1]);

		double maxTolerance = weightsDescending[weightsDescending.size() - max_points]; // max_points - 1];

		PolygonF res;
		for (size_t i = 0; i < points.size(); ++i)
		{
			if (weights[i] >= maxTolerance)
				res.push_back(points[i]);
		}
		return res;
	}

	static bool toleranceCompare(double x, double y)
	{
		double maxXYOne = std::max({1.0, std::fabs(x), std::fabs(y)});
		return std::fabs(x - y) <= std::numeric_limits<double>::epsilon() * maxXYOne;
	}

	PolygonF convexHull(const PolygonF &poly)
	{
		if (poly.size() < 3)
			return poly;

		PolygonF p;

		// find bottom most point, remove duplicated points and unclose polygon
		size_t bottom_i = 0;
		double bottom = poly.front().y();
		p.push_back(poly.front());
		for (size_t i = 1; i < poly.size() - 1; ++i)
		{
			if (poly[i] != poly[i - 1])
			{
				p.push_back(poly[i]);
				if (p.back().y() > bottom)
				{
					bottom = p.back().y();
					bottom_i = (size_t)p.size() - 1;
				}
			}
		}
		if (poly.back() != poly.front() && poly.back() != poly[poly.size() - 2])
		{
			p.push_back(poly.back());
			if (p.back().y() > bottom)
			{
				bottom = p.back().y();
				bottom_i = (size_t)p.size() - 1;
			}
		}

		// build list of points to inspect
		PolygonF to_inspect = p;
		// std::list<PointF> to_inspect;
		// for (int i = 0; i < p.size(); ++i) to_inspect.push_back(p[i]);

		// add first point (bottom most)
		PolygonF res;
		res.push_back(p[bottom_i]);

		LineF line(p[bottom_i] - PointF(1, 0), p[bottom_i]);
		// start from bottom most
		// int count = 0;
		while (to_inspect.size())
		{

			double angle = 361;
			size_t index = 0;
			// look for the point with smallest angle
			for (size_t i = 0; i < to_inspect.size(); ++i)
			{
				if (to_inspect[i] != res.back())
				{
					double a = line.angleTo(LineF(line.p2(), to_inspect[i]));
					if (a < angle)
					{
						angle = a;
						index = i;
					}
				}
			}

			line = LineF(line.p2(), to_inspect[index]);

			if (toleranceCompare(angle, 0) && res.size() > 1)
				res.back() = to_inspect[index]; // avoid several points on the same line
			else
				res.push_back(to_inspect[index]);
			if (res.back() == res.front())
				break;
			to_inspect.erase(to_inspect.begin() + index);
		}

		return res;
	}

	static double angleToXAxis(const LineF &s)
	{
		PointF delta = s.p1() - s.p2();
		return -std::atan(delta.y() / delta.x());
	}
	static PointF rotateToXAxis(const PointF &p, double angle)
	{
		double s = std::sin(angle);
		double c = std::cos(angle);
		double newX = p.x() * c - p.y() * s;
		double newY = p.x() * s + p.y() * c;
		return PointF(newX, newY);
	}

	OrientedRect minimumAreaBBox(const PolygonF &poly, bool check_convex)
	{
		// see https://github.com/schmidt9/MinimalBoundingBox/blob/main/MinimalBoundingBox/MinimalBoundingBox.cpp

		PolygonF hullPoints = check_convex ? (isPolygonNonConcave(poly) ? poly : convexHull(poly)) : (poly);
		openPolygon(hullPoints);

		if (hullPoints.size() <= 1)
		{
			return OrientedRect();
		}

		RectF minBox;
		double minAngle = 0;

		for (size_t i = 0; i < hullPoints.size(); ++i)
		{

			auto nextIndex = i + 1;

			auto current = hullPoints[i];
			auto next = hullPoints[nextIndex % hullPoints.size()];

			auto segment = LineF(current, next);

			// min / max points

			auto top = DBL_MAX;
			auto bottom = DBL_MAX * -1;
			auto left = DBL_MAX;
			auto right = DBL_MAX * -1;

			// get angle of segment to x axis

			auto angle = angleToXAxis(segment);
			// printf("POINT (%f;%f) DEG %f\n", current.x(), current.y(), angle * (180 / M_PI));

			// rotate every point and get min and max values for each direction

			for (auto &p : hullPoints)
			{
				auto rotatedPoint = rotateToXAxis(p, angle);

				top = std::min(top, rotatedPoint.y());
				bottom = std::max(bottom, rotatedPoint.y());

				left = std::min(left, rotatedPoint.x());
				right = std::max(right, rotatedPoint.x());
			}

			// create axis aligned bounding box

			auto box = RectF(left, top, right, bottom);

			if (minBox.isEmpty() || minBox.area() > box.area())
			{
				minBox = box;
				minAngle = angle;
			}
		}

		// get bounding box dimensions using axis aligned box,
		// larger side is considered as height

		PolygonF minBoxPoints; // = minBox.getPoints();
		minBoxPoints << minBox.topLeft() << minBox.topRight() << minBox.bottomRight() << minBox.bottomLeft();

		auto v1 = minBoxPoints[0] - minBoxPoints[1];
		auto v2 = minBoxPoints[1] - minBoxPoints[2];
		auto absX = std::abs(v1.x());
		auto absY = std::abs(v2.y());
		auto width = std::min(absX, absY);
		auto height = std::max(absX, absY);

		// rotate axis aligned box back and get center

		auto sumX = 0.0;
		auto sumY = 0.0;

		for (auto &point : minBoxPoints)
		{
			point = rotateToXAxis(point, -minAngle);
			sumX += point.x();
			sumY += point.y();
		}

		auto center = PointF(sumX / 4, sumY / 4);

		// get angles

		auto heightPoint1 = (absX > absY)
								? minBoxPoints[0]
								: minBoxPoints[1];
		auto heightPoint2 = (absX > absY)
								? minBoxPoints[1]
								: minBoxPoints[2];

		auto heightAngle = angleToXAxis(LineF(heightPoint1, heightPoint2));
		auto widthAngle = (heightAngle > 0) ? heightAngle - M_PI_2 : heightAngle + M_PI_2;

		// get alignment

		// auto tolerance = alignmentTolerance * (M_PI / 180);
		// auto isAligned = (widthAngle <= tolerance && widthAngle >= -tolerance);

		return OrientedRect(minBoxPoints, hullPoints, center, width, height, widthAngle, heightAngle);
	}

} // end namespace rir
