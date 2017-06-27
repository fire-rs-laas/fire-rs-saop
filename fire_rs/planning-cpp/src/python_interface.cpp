#include <pybind11/pybind11.h>

#include <pybind11/stl.h> // for conversions between c++ and python collections
#include <pybind11/numpy.h> // support for numpy arrays
#include "trajectory.h"
#include "local_search.h"
#include "raster.h"
#include "visibility.h"
#include "planning.h"

namespace py = pybind11;

/** Converts a nupy array to a vector */
template<class T>
std::vector<T> as_vector(py::array_t<T, py::array::c_style | py::array::forcecast> array) {
    std::vector<T> data(array.size());
    for(size_t x=0; x<array.shape(0); x++) {
        for(size_t y=0; y<array.shape(1); y++) {
            data[x + y*array.shape(0)] = *(array.data(x, y));
        }
    }
    return data;
}

/** Converts a vector to a 2D numpy array. */
template<class T>
py::array_t<T> as_nparray(std::vector<T> vec, size_t x_width, size_t y_height) {
    ASSERT(vec.size() == x_width * y_height)
    py::array_t<T, py::array::c_style | py::array::forcecast> array(std::vector<size_t> {x_width, y_height});
    for(size_t x=0; x<x_width; x++) {
        for (size_t y = 0; y < y_height; y++) {
            *(array.mutable_data(x, y)) = vec[x + y*x_width];
        }
    }
    return array;
}

PYBIND11_PLUGIN(uav_planning) {
    py::module m("uav_planning", "Python module for AUV trajectory planning");

    srand(time(0));

    py::class_<Raster>(m, "Raster")
            .def("__init__", [](Raster& self, py::array_t<double, py::array::c_style | py::array::forcecast> arr, double x_offset, double y_offset, double cell_width)  {
                // create a new object and sibstitute to the given self
                new (&self) Raster(as_vector<double>(arr), arr.shape(0), arr.shape(1), x_offset, y_offset, cell_width);
            })
            .def("as_numpy", [](Raster& self) {
                return as_nparray<double>(self.data, self.x_width, self.y_height);
            })
            .def_readonly("x_offset", &Raster::x_offset)
            .def_readonly("y_offset", &Raster::y_offset)
            .def_readonly("cell_width", &Raster::cell_width);

    py::class_<LRaster>(m, "LRaster")
            .def("__init__", [](LRaster& self, py::array_t<long, py::array::c_style | py::array::forcecast> arr, double x_offset, double y_offset, double cell_width) {
                new (&self) LRaster(as_vector<long>(arr), arr.shape(0), arr.shape(1), x_offset, y_offset, cell_width);
            })
            .def("as_numpy", [](LRaster& self) {
                return as_nparray<long>(self.data, self.x_width, self.y_height);
            })
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
            .def(py::init<const TrajectoryConfig&>())
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

    py::class_<TrajectoryConfig>(m, "TrajectoryConfig")
            .def(py::init<UAV, Waypoint, Waypoint, double, double>())
            .def_readonly("uav", &TrajectoryConfig::uav)
            .def_readonly("max_flight_time", &TrajectoryConfig::max_flight_time)
            .def_static("build", [](UAV uav, double start_time, double max_flight_time) -> TrajectoryConfig {
                return TrajectoryConfig(uav, start_time, max_flight_time);
            }, "Constructor", py::arg("uav"), py::arg("start_time") = 0, py::arg("max_flight_time") = std::numeric_limits<double>::max());

    py::class_<Plan>(m, "Plan")
            .def_readonly("trajectories", &Plan::trajectories);

    py::class_<SearchResult>(m, "SearchResult")
            .def("initial_plan", &SearchResult::initial)
            .def("final_plan", &SearchResult::final)
            .def_readonly("intermediate_plans", &SearchResult::intermediate_plans);

    m.def("make_plan_vns", [](UAV uav, Raster ignitions, double min_time, double max_time,
                              double max_flight_time, size_t save_every) -> SearchResult {
        auto fire_data = make_shared<FireData>(ignitions);
        TrajectoryConfig conf(uav, min_time, max_flight_time);
        Plan p(vector<TrajectoryConfig> { conf }, fire_data, TimeWindow{min_time, max_time});

        DefaultVnsSearch vns;

        auto res = vns.search(p, 0, save_every);
        return res;
    }, py::arg("uav"), py::arg("ignitions"), py::arg("min_time"), py::arg("max_time"), py::arg("max_flight_time"), py::arg("save_every") = 0);

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