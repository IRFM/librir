#include "DrawPolygon.h"

namespace rir
{

	size_t polygonArea(const Point* pts, size_t size, const Rect& r)
	{
		struct Fun {
			void operator()(signed_integral, signed_integral) const {}
		};
		Fun f;
		return fillPolygonFunctor(pts, size, f, r);
	}

}