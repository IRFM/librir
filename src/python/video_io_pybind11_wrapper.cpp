// pybind11_wrapper.cpp
#include <pybind11/pybind11.h>
#include "video_io.h"

PYBIND11_MODULE(rir_video_io, m)
{
    m.doc() = "librir video_io module C/C++ bindings"; // Optional module docstring
    m.def("open_camera_file", &open_camera_file, "Open a camera handle from filename");
}