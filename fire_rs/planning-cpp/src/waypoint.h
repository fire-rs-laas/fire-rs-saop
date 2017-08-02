#ifndef PLANNING_CPP_WAYPOINT_H
#define PLANNING_CPP_WAYPOINT_H

#include <sstream>
#include "utils.h"


struct Point final {
    double x;
    double y;

    double dist(const Point& pt) const {
        return sqrt(pow(x-pt.x, 2) + pow(y-pt.y, 2));
    }

    double angle_to(const Point& pt) const {
        return atan2(y-pt.y, x-pt.x);
    }
};

struct Waypoint final {
    // order and length is important for compatibility with dubins.cpp, allowing to simply cast a Waypoint* to a double*
    double x;
    double y;
    double dir;

    constexpr Waypoint(const Waypoint &wp) : x(wp.x), y(wp.y), dir(wp.dir) {}
    constexpr Waypoint(const double x, const double y, const double dir) : x(x), y(y), dir(dir) {};

    bool operator==(const Waypoint& o) const {
        return ALMOST_EQUAL(x,o.x) && ALMOST_EQUAL(y, o.y) && ALMOST_EQUAL(dir, o.dir);
    }
    bool operator!=(const Waypoint& o) const {
        return !ALMOST_EQUAL(x,o.x) || !ALMOST_EQUAL(y, o.y) || !ALMOST_EQUAL(dir, o.dir);
    }

    std::string to_string() const {
        std::stringstream repr;
        repr << "(" << x<<", "<<y<<", "<<dir<<")";
        return repr.str();
    }

    Waypoint forward(double dist) const {
        const double new_x = x + cos(dir) * dist;
        const double new_y = y + sin(dir) * dist;
        return Waypoint(new_x, new_y, dir);
    }

    Waypoint rotate(double relative_angle) const {
        return Waypoint(x, y, dir+relative_angle);
    }

    Waypoint with_angle(double absolute_angle) const {
        return Waypoint(x, y, absolute_angle);
    }

    friend std::ostream& operator<< (std::ostream& stream, const Waypoint& w) {
        return stream << "(" << w.x << ", " << w.y << ", " << w.dir <<")";
    }

    Point as_point() const {
        return Point{x, y};
    }
};


struct Segment final {
    Waypoint start;
    Waypoint end;
    double length;

    constexpr Segment(const Waypoint& wp1) : start(wp1), end(wp1), length(0) {}
    Segment(const Waypoint& wp1, const Waypoint& wp2)
            : start(wp1), end(wp2), length(sqrt(pow(wp1.x-wp2.x, 2) + pow(wp1.y-wp2.y, 2))) {}
    Segment(const Waypoint& wp1, const double length)
            : start(wp1), end(Waypoint(wp1.x +cos(wp1.dir)*length, wp1.y + sin(wp1.dir)*length, wp1.dir)), length(length) {}

    friend std::ostream& operator<< (std::ostream& stream, const Segment& s) {
        return stream << "<" << s.start << ", " << s.length << ">";
    }

    bool operator==(const Segment& o) const {
        return start == o.start && end == o.end && ALMOST_EQUAL(length, o.length);
    }

    bool operator!=(const Segment& o) const {
        return start != o.start || end != o.end || !ALMOST_EQUAL(length, o.length);
    }
};

struct PointTime final {
    Point pt;
    double time;
};

struct TimeWindow final {
    double start;
    double end;
};

struct PointTimeWindow final {
    Point pt;
    TimeWindow tw;
};




#endif //PLANNING_CPP_WAYPOINT_H
