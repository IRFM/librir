// pybind11_wrapper.cpp
#include <pybind11/pybind11.h>
#include "geometry.h"

PYBIND11_MODULE(rir_geometry, m)
{
    m.doc() = "librir geometry module C/C++ bindings"; // Optional module docstring
    // m.def("ts_exists", &ts_exists, "A function that checks if the signal exists in base");
}