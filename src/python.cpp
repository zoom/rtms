#include <pybind11/pybind11.h>


int add(int i, int j) {
    return i + j;
}

PYBIND11_MODULE(rtms_sdk, m) {
    m.doc() = "Zoom RTMS SDK Python Bindings"; // optional module docstring

    m.def("add", &add, "A function that adds two numbers");
}