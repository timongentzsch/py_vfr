#include "videoreader.cpp"
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

namespace py = pybind11;

void init_videoreader(py::module &m)
{
    py::class_<VideoReader>(m, "VideoReader")
        .def(py::init<const std::string &, bool>())
        .def("getFrame", [](VideoReader &self, double timestamp) -> py::object
             {
                AVFrame *frame = self.getFrame(timestamp);
                if (!frame) {
                    return py::none();
                }

                int height = frame->height;
                int width = frame->width;
                int channels = (frame->format == AV_PIX_FMT_GRAY8) ? 1 : 3;  // Assuming only grayscale or RGB

                // Wrap the frame data in a numpy array
                py::array_t<uint8_t> result({height, width, channels}, frame->data[0]);

                return result; })
        .def("getInfo", [](VideoReader &self) -> py::dict
             {
            auto info = self.getInfo();
            py::dict result;
            for (auto &[key, value] : info.items()) {
                result[py::str(key)] = value;
            }
            return result; });
}

PYBIND11_MODULE(videoreader, m)
{
    m.doc() = "Python module for VideoReader";
    init_videoreader(m);
}