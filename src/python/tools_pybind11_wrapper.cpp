// pybind11_wrapper.cpp
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>
#include "tools.h"

namespace py = pybind11;
// #ifdef WIN32
// #include <windows.h>
// #include <libloaderapi.h>

// auto sharedDllDir = AddDllDirectory(L"../libs");
// #endif

char *py_zstd_decompress(char *src)
{
    int64_t srcSize = strlen(src);
    int64_t outsize = zstd_compress_bound(srcSize);

    char *dst = (char *)malloc(outsize);
    if (outsize < 0)
        throw -1;
    int64_t retsize = zstd_decompress(src, srcSize, dst, outsize);
    return dst;
}

py::bytes py_blosc_compress_zstd(py::bytes src, int typesize, int shuffle, int level = 1)
{
    py::bytes out;

    return out;
}

PYBIND11_MODULE(rir_tools, m)
{
    m.doc() = "librir tools module C/C++ bindings"; // Optional module docstring
    m.def("get_temp_directory", &get_temp_directory, "A function to get temporary directory");
    m.def("set_print_function", &set_print_function, "Set the log function to be used by LIBRIR."
                                                     "This function must have the signature: void print_function(int,const char*), where the first argument is"
                                                     "the log level (INFO_LEVEL, WARNING_LEVEL or ERROR_LEVEL).");
    m.def("disable_print", &disable_print, "Disables logging");
    m.def("zstd_compress_bound", &zstd_compress_bound, "Zstd interface."
                                                       "Returns the highest compressed size for given input data size.");
    m.def("zstd_compress", &zstd_compress, "Zstd interface."
                                           "Compress a bytes object and return the result.");

    m.def("zstd_decompress", &py_zstd_decompress, "Zstd interface."
                                                  "Decompress a bytes object (previously compressed with zstd_compress) and return the result.");
    m.def("blosc_compress_zstd", &py_blosc_compress_zstd, "Zstd interface."
                                                          "Compress with blosc");
    // m.def("disable_print", &disable_print, "Disables logging");
    // m.def("disable_print", &disable_print, "Disables logging");
    // m.def("disable_print", &disable_print, "Disables logging");
}