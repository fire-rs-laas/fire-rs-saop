#include <pybind11/pybind11.h>

// for conversions between c++ and python collections
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include "trajectory.h"
#include "local_search.h"
#include "raster.h"
#include "visibility.h"
#include "planning.h"
#include "debug.h"

namespace py = pybind11;

PYBIND11_PLUGIN(uav_planning) {
    py::module m("uav_planning", "Generating primes in c++ with python bindings using pybind11");

    srand(time(NULL));

    py::class_<Raster>(m, "Raster")
            .def(py::init<py::array_t<double, py::array::c_style | py::array::forcecast>, double, double, double>())
            .def_readonly("data", &Raster::data)
            .def_readonly("x_offset", &Raster::x_offset)
            .def_readonly("y_offset", &Raster::y_offset)
            .def_readonly("cell_width", &Raster::cell_width);

    py::class_<LRaster>(m, "LRaster")
            .def(py::init<py::array_t<long, py::array::c_style | py::array::forcecast>, double, double, double>())
            .def_readonly("data", &LRaster::data)
            .def_readonly("x_offset", &LRaster::x_offset)
            .def_readonly("y_offset", &LRaster::y_offset)
            .def_readonly("cell_width", &LRaster::cell_width);

    py::class_<Waypoint>(m, "Waypoint")
            .def(py::init<const double, const double, const double>())
            .def_readonly("x", &Waypoint::x)
            .def_readonly("y", &Waypoint::y) 
            .def_readonly("dir", &Waypoint::dir)
            .def("__repr__", &Waypoint::to_string); 

    py::class_<Segment>(m, "Segment")
            .def(py::init<const Waypoint, const double>())
            .def_readonly("start", &Segment::start)
            .def_readonly("end", &Segment::end)
            .def_readonly("length", &Segment::length);

    py::class_<UAV>(m, "UAV")
            .def(py::init<const double, const double>())
            .def_readonly("min_turn_radius", &UAV::min_turn_radius)
            .def_readonly("max_air_speed", &UAV::max_air_speed)
            .def("travel_distance", &UAV::travel_distance, py::arg("origin"), py::arg("destination"))
            .def("travel_time", &UAV::travel_time, py::arg("origin"), py::arg("destination"));

    py::class_<Trajectory>(m, "Trajectory") 
            .def(py::init<const UAV&>())
            .def_readonly("uav", &Trajectory::uav)
            .def("length", &Trajectory::length)
            .def("duration", &Trajectory::duration)
            .def("as_waypoints", &Trajectory::as_waypoints, py::arg("step_size")= -1)
            .def("with_waypoint_at_end", &Trajectory::with_waypoint_at_end)
            .def("__repr__", &Trajectory::to_string);

    py::class_<Visibility>(m, "Visibility")
            .def(py::init<Raster, double, double>())
            .def("set_time_window_of_interest", &Visibility::set_time_window_of_interest)
            .def("add_segment", &Visibility::add_segment)
            .def("remove_segment", &Visibility::remove_segment)
            .def("cost", &Visibility::cost)
            .def_readonly("visibility", &Visibility::visibility)
            .def_readonly("interest", &Visibility::interest);

    py::class_<Plan>(m, "Plan")
            .def(py::init<vector<UAV>, Visibility>())
            .def_readonly("trajectories", &Plan::trajectories);

    m.def("make_plan", [](UAV uav, vector<Segment> segments, Visibility visibility) -> Plan {
        PPlan p = make_shared<Plan>(vector<UAV> { uav }, visibility);
        PPlan ret = Insert::smart_insert(p, 0, segments);
        return *ret;
    });

    m.def("improve", [](const Trajectory& traj) {
        auto n1 = std::make_shared<dubins::AlignTwoConsecutiveNeighborhood>();
        auto n2 = std::make_shared<dubins::OrientationChangeNeighborhood>();
        auto n3 = std::make_shared<dubins::TwoOrientationChangeNeighborhood>();
        auto n4 = std::make_shared<dubins::AlignOnNextNeighborhood>();
        auto n5 = std::make_shared<dubins::AlignOnPrevNeighborhood>();
        auto ns = std::make_shared<dubins::CombinedNeighborhood<Trajectory>>(
                std::vector<std::shared_ptr<dubins::Neighborhood<Trajectory>>> { n1, n2, n3, n4, n5 });
        Trajectory t_res = dubins::first_improvement_search(traj, *ns, 5000);
        return t_res;
    });

    return m.ptr();
}