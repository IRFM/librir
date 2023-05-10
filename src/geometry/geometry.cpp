#include "geometry.h"
#include "Polygon.h"
#include "DrawPolygon.h"
#include "Log.h"

extern "C" {
#include <string.h>
}

using namespace rir;

int polygon_interpolate(double* xy1, int s1, double* xy2, int s2, double advance, double* xyres, int* sres)
{
	std::vector<PointF> p1(s1), p2(s2);
	for (int i = 0; i < s1; ++i)
		p1[i] = PointF(xy1[i*2], xy1[i*2+1]);
	for (int i = 0; i < s2; ++i)
		p2[i] = PointF(xy2[i*2], xy2[i*2+1]);

	std::vector<PointF> res = interpolatePolygons(p1, p2, advance);
	if ((int)res.size() > *sres) {
		*sres = (int)res.size();
		return -1;
	}

	*sres = (int)res.size();
	for (size_t i = 0; i < res.size(); ++i) {
		xyres[i*2] = res[i].x();
		xyres[i*2+1] = res[i].y();
	}
	return 0;
}


int rdp_simplify_polygon(double* xy, int point_count, double* out_xy, int* out_point_count, double epsilon)
{
	PolygonF poly(point_count);
	for (int i = 0; i < point_count; ++i)
		poly[i] = PointF(xy[i*2], xy[i*2+1]);

	poly = RDPSimplifyPolygon(poly, epsilon);
	if (*out_point_count < (int)poly.size()) {
		*out_point_count = (int)poly.size();
		return -2;
	}

	*out_point_count = (int)poly.size();
	for (size_t i = 0; i < poly.size(); ++i) {
		out_xy[i*2] = poly[i].x();
		out_xy[i*2+1] = poly[i].y();
	}
	return 0;
}

int rdp_simplify_polygon2(double* xy, int point_count, double* out_xy, int* out_point_count, int maxPoints)
{
	PolygonF poly(point_count);
	for (int i = 0; i < point_count; ++i)
		poly[i] = PointF(xy[i*2], xy[i*2+1]);

	poly = RDPSimplifyPolygon2(poly, maxPoints);
	if (*out_point_count < (int)poly.size()) {
		*out_point_count = (int)poly.size();
		return -2;
	}

	*out_point_count = (int)poly.size();
	for (size_t i = 0; i < poly.size(); ++i) {
		out_xy[i*2] = poly[i].x();
		out_xy[i*2+1] = poly[i].y();
	}
	return 0;
}


int draw_polygon(void* ptr, const char* img_type, int w, int h, double* xy, int point_count, double value)
{
	std::vector<Point> pts(point_count);
	for (int i = 0; i < point_count; ++i) {
		pts[i].rx() = (int)std::round(xy[i*2]);
		pts[i].ry() = (int)std::round(xy[i*2+1]);
	}

	if (strcmp(img_type, "bool") == 0) {
		Array2DView<bool> img((bool*)ptr, w, h);
		drawPolygon(img, pts.data(), (int)pts.size(), value != 0);
	}
	else if (strcmp(img_type, "int8") == 0) {
		Array2DView<char> img((char*)ptr, w, h);
		drawPolygon(img, pts.data(), (int)pts.size(), (char)value);
	}
	else if (strcmp(img_type, "uint8") == 0) {
		Array2DView<unsigned char> img((unsigned char*)ptr, w, h);
		drawPolygon(img, pts.data(), (int)pts.size(), (unsigned char)value);
	}
	else if (strcmp(img_type, "int16") == 0) {
		Array2DView<short> img((short*)ptr, w, h);
		drawPolygon(img, pts.data(), (int)pts.size(), (short)value);
	}
	else if (strcmp(img_type, "uint16") == 0) {
		Array2DView<unsigned short> img((unsigned short*)ptr, w, h);
		drawPolygon(img, pts.data(), (int)pts.size(), (unsigned short)value);
	}
	else if (strcmp(img_type, "int32") == 0) {
		Array2DView<int> img((int*)ptr, w, h);
		drawPolygon(img, pts.data(), (int)pts.size(), (int)value);
	}
	else if (strcmp(img_type, "uint32") == 0) {
		Array2DView<unsigned int> img((unsigned int*)ptr, w, h);
		drawPolygon(img, pts.data(), (int)pts.size(), (unsigned int)value);
	}
	else if (strcmp(img_type, "int64") == 0) {
		Array2DView<int64_t> img((int64_t*)ptr, w, h);
		drawPolygon(img, pts.data(), (int)pts.size(), (int64_t)value);
	}
	else if (strcmp(img_type, "uint64") == 0) {
		Array2DView<uint64_t> img((uint64_t*)ptr, w, h);
		drawPolygon(img, pts.data(), (int)pts.size(), (uint64_t)value);
	}
	else if (strcmp(img_type, "float32") == 0) {
		Array2DView<float> img((float*)ptr, w, h);
		drawPolygon(img, pts.data(), (int)pts.size(), (float)value);
	}
	else if (strcmp(img_type, "float64") == 0) {
		Array2DView<double> img((double*)ptr, w, h);
		drawPolygon(img, pts.data(), (int)pts.size(), (double)value);
	}
	else {
		logError("Wrong data type");
		return -1;
	}

	return 0;
}




int extract_polygon(void* ptr, const char* img_type, int w, int h, int* xy, int* point_count, double pix_value)
{
	std::vector<Point> res;
	if (strcmp(img_type, "bool") == 0) {
		ConstArray2DView<bool> img((bool*)ptr, w, h);
		res = extractPolygon(img, pix_value != 0);
	}
	else if (strcmp(img_type, "int8") == 0) {
		ConstArray2DView<char> img((char*)ptr, w, h);
		res = extractPolygon(img, (char)pix_value);
	}
	else if (strcmp(img_type, "uint8") == 0) {
		ConstArray2DView<unsigned char> img((unsigned char*)ptr, w, h);
		res = extractPolygon(img, (unsigned char)pix_value);
	}
	else if (strcmp(img_type, "int16") == 0) {
		ConstArray2DView<short> img((short*)ptr, w, h);
		res = extractPolygon(img, (short)pix_value);
	}
	else if (strcmp(img_type, "uint16") == 0) {
		ConstArray2DView<unsigned short> img((unsigned short*)ptr, w, h);
		res = extractPolygon(img, (unsigned short)pix_value);
	}
	else if (strcmp(img_type, "int32") == 0) {
		ConstArray2DView<int> img((int*)ptr, w, h);
		res = extractPolygon(img, (int)pix_value);
	}
	else if (strcmp(img_type, "uint32") == 0) {
		ConstArray2DView<unsigned int> img((unsigned int*)ptr, w, h);
		res = extractPolygon(img, (unsigned int)pix_value);
	}
	else if (strcmp(img_type, "int64") == 0) {
		ConstArray2DView<int64_t> img((int64_t*)ptr, w, h);
		res = extractPolygon(img, (int64_t)pix_value);
	}
	else if (strcmp(img_type, "uint64") == 0) {
		ConstArray2DView<uint64_t> img((uint64_t*)ptr, w, h);
		res = extractPolygon(img, (uint64_t)pix_value);
	}
	else if (strcmp(img_type, "float32") == 0) {
		ConstArray2DView<float> img((float*)ptr, w, h);
		res = extractPolygon(img, (float)pix_value);
	}
	else if (strcmp(img_type, "float64") == 0) {
		ConstArray2DView<double> img((double*)ptr, w, h);
		res = extractPolygon(img, (double)pix_value);
	}
	else {
		logError("Wrong data type");
		return -1;
	}

	if (*point_count < (int)res.size()) {
		*point_count = (int)res.size();
		return -2;
	}

	*point_count = (int)res.size();
	for (size_t i = 0; i < res.size(); ++i) {
		xy[i*2] = res[i].x();
		xy[i*2+1] = res[i].y();
	}

	return 0;
}


int extract_convex_hull(double* xy, int point_count, double* out_xy, int* out_point_count)
{
	PolygonF poly(point_count);
	for (int i = 0; i < point_count; ++i) {
		poly[i] = PointF(xy[i*2], xy[i*2+1]);
	}

	poly = convexHull(poly);
	if (*out_point_count < (int)poly.size()) {
		*out_point_count = (int)poly.size();
		return -2;
	}

	*out_point_count = (int)poly.size();
	for (size_t i = 0; i < poly.size(); ++i) {
		out_xy[i * 2] = poly[i].x();
		out_xy[i * 2+1] = poly[i].y();
	}
	return 0;
}


int minimum_area_bbox(double* xy, int point_count, double* x_center, double* y_center, double* width, double* height, double* widthAngle, double* heightAngle)
{
	PolygonF poly(point_count);
	for (int i = 0; i < point_count; ++i) {
		poly[i] = PointF(xy[i*2], xy[i*2+1]);
	}

	OrientedRect r = minimumAreaBBox(poly);
	*x_center = r.center.x();
	*y_center = r.center.y();
	*width = r.width;
	*height = r.height;
	*widthAngle = r.widthAngle;
	*heightAngle = r.heightAngle;
	return 0;
}
