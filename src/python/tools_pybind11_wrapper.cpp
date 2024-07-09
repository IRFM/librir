// pybind11_wrapper.cpp
#include <pybind11/pybind11.h>
#include "tools.h"
// #ifdef WIN32
// #include <windows.h>
// #include <libloaderapi.h>

// auto sharedDllDir = AddDllDirectory(L"../libs");
// #endif
PYBIND11_MODULE(rir_tools, m)
{
    m.doc() = "librir tools module C/C++ bindings"; // Optional module docstring
    m.def("get_temp_directory", &get_temp_directory, "A function to get temporary directory");
}