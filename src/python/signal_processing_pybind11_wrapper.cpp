// pybind11_wrapper.cpp
#include <pybind11/pybind11.h>
#include "signal_processing.h"

PYBIND11_MODULE(rir_signal_processing, m)
{
    m.doc() = "librir signal_processing module C/C++ bindings"; // Optional module docstring
    // m.def("ts_exists", &ts_exists, "A function that checks if the signal exists in base");
}