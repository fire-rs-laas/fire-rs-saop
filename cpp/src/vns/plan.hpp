/* Copyright (c) 2017, CNRS-LAAS
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

#ifndef PLANNING_CPP_PLAN_H
#define PLANNING_CPP_PLAN_H


#include "../core/structures/trajectory.hpp"
#include "fire_data.hpp"
#include "../raster.hpp"
#include "../ext/json.hpp"
#include "../core/structures/trajectories.hpp"

using json = nlohmann::json;

using namespace std;

struct Plan;
typedef shared_ptr<Plan> PlanPtr;

struct Plan {
    TimeWindow time_window;
    Trajectories core;
    shared_ptr<FireData> firedata;
    vector<PointTimeWindow> possible_observations;
    vector<PositionTime> observed_previously;

    Plan(const Plan& plan) = default;

    Plan(vector<TrajectoryConfig> traj_confs, shared_ptr<FireData> fire_data, TimeWindow tw,
         vector<PositionTime> observed_previously={})
            : time_window(tw), core(traj_confs), firedata(fire_data), observed_previously(observed_previously)
    {
        for(auto conf : traj_confs) {
            ASSERT(conf.start_time >= time_window.start && conf.start_time <= time_window.end);
        }

        std::vector<Cell> obs_prev_cells;
        obs_prev_cells.reserve( observed_previously.size() );
        std::transform( observed_previously.begin(), observed_previously.end(),
                        std::back_inserter(obs_prev_cells),
                        [this] (const PositionTime &pt) -> Cell { return firedata->ignitions.as_cell(pt.pt); });

        for(size_t x=0; x<firedata->ignitions.x_width; x++) {
            for (size_t y = 0; y < firedata->ignitions.y_height; y++) {
                const double t = firedata->ignitions(x, y);
                if (time_window.start <= t && t <= time_window.end) {
                    Cell c{x, y};

                    // If the cell is in the observed_previously list, do not add it to possible_observations
                    if (std::find(obs_prev_cells.begin(), obs_prev_cells.end(), c)
                        == obs_prev_cells.end())
                    {
                        possible_observations.push_back(
                                PointTimeWindow{firedata->ignitions.as_position(c),
                                                {firedata->ignitions(c), firedata->traversal_end(c)}});
                    }
                }
            }
        }
    }

    json metadata() const {
        json j;
        j["duration"] = duration();
        j["utility"] = utility();
        j["num_segments"] = num_segments();
        j["trajectories"] = json::array();
        for(auto &t : core.trajectories) {
            json jt;
            jt["duration"] = t.duration();
            jt["max_duration"] = t.conf.max_flight_time;
            jt["num_segments"] = t.size();
            jt["start_time"] = t.start_time();
            jt["end_time"] = t.end_time();
            j["trajectories"].push_back(jt);
        }
        return j;
    }

    /** A plan is valid iff all trajectories are valid (match their configuration. */
    bool is_valid() const {
        return core.is_valid();
    }

    /** Sum of all trajectory durations. */
    double duration() const {
        return core.duration();
    }

    /** Cost of the plan.
     * The key idea is to sum the distance of all ignited points in the time window to their closest observation.
     **/
    double utility() const {
        vector<PositionTime> done_obs = observations();
        double global_cost = 0;
        for(PointTimeWindow possible_obs : possible_observations) {
            double min_dist = MAX_INFORMATIVE_DISTANCE;
            // find the closest observation.
            for(PositionTime obs : done_obs) {
                min_dist = min(min_dist, possible_obs.pt.dist(obs.pt));
            }
            // utility is based on the minimal distance to the observation and normalized such that
            // utility = 0 if min_dist <= REDUNDANT_OBS_DIST
            // utility = 1 if min_dist = (MAX_INFORMATIVE_DISTANCE - REDUNDANT_OBS_DIST
            // evolves linearly in between.
            const double self_cost = (max(min_dist, REDUNDANT_OBS_DIST)-REDUNDANT_OBS_DIST) / (MAX_INFORMATIVE_DISTANCE - REDUNDANT_OBS_DIST);
            global_cost += self_cost;
        }
        return global_cost;
    }

    size_t num_segments() const {
        return core.num_segments();
    }

    /** All observations in the plan. Computed by taking the visibility center of all segments.
     * Each observation is tagged with a time, corresponding to the start time of the segment.*/
    vector<PositionTime> observations() const {
        return observations(time_window);
    }

    /* Observations done within an arbitrary time window
     */
    vector<PositionTime> observations(const TimeWindow& tw) const {
        vector<PositionTime> obs = std::vector<PositionTime>(observed_previously);
        for(auto& traj : core.trajectories) {
            UAV drone = traj.conf.uav;
            for(size_t seg_id=0; seg_id<traj.size(); seg_id++) {
                const Segment3d& seg = traj[seg_id];

                double obs_time = traj.start_time(seg_id);
                double obs_end_time = traj.end_time(seg_id);
                TimeWindow seg_tw = TimeWindow{obs_time, obs_end_time};
                if (tw.contains(seg_tw)) {
                    opt<std::vector<Cell>> opt_cells = segment_trace(seg, drone.view_depth, drone.view_width,
                                                                     firedata->ignitions);
                    if (opt_cells) {
                        for (const auto &c : *opt_cells) {
                            if (firedata->ignitions(c) <= obs_time && obs_time <= firedata->traversal_end(c)) {
                                // If the cell is observable, add it to the observations list
                                obs.push_back(PositionTime{firedata->ignitions.as_position(c), traj.start_time(seg_id)});
                            }
                        }
                    }
                }
            }
        }
        return obs;
    }

    void insert_segment(size_t traj_id, const Segment3d& seg, size_t insert_loc, bool do_post_processing = true) {
        ASSERT(traj_id < core.size());
        ASSERT(insert_loc <= core[traj_id].traj.size());
        core[traj_id].insert_segment(seg, insert_loc);
        if(do_post_processing)
            post_process();
    }

    void erase_segment(size_t traj_id, size_t at_index, bool do_post_processing = true) {
        ASSERT(traj_id < core.size());
        ASSERT(at_index < core[traj_id].traj.size());
        core[traj_id].erase_segment(at_index);
        if(do_post_processing)
            post_process();
    }

    void replace_segment(size_t traj_id, size_t at_index, const Segment3d& by_segment) {
        replace_segment(traj_id, at_index, 1, std::vector<Segment3d>({by_segment}));
    }

    void replace_segment(size_t traj_id, size_t at_index, size_t n_replaced, const std::vector<Segment3d>& segments) {
        ASSERT(n_replaced > 0);
        ASSERT(traj_id < core.size());
        ASSERT(at_index + n_replaced - 1 < core[traj_id].traj.size());

        // do not post process as we will do that at the end
        for (size_t i = 0; i < n_replaced; ++i) {
            erase_segment(traj_id, at_index, false);
        }

        for (size_t i = 0; i < segments.size(); ++i) {
            insert_segment(traj_id, segments.at(i), at_index + i, false);
        }

        post_process();
    }

    void post_process() {
        project_on_fire_front();
        smooth_trajectory();
    }

    /** Make sure every segment makes an observation, i.e., that the picture will be taken when the fire in traversing the main cell.
     *
     * If this is not the case for a given segment, its is projected on the firefront.
     * */
    void project_on_fire_front() {
        for(auto &traj : core.trajectories) {
            size_t seg_id = traj.first_modifiable();
            while(seg_id <= traj.last_modifiable()) {
                const Segment3d& seg = traj[seg_id];
                const double t = traj.start_time(seg_id);
                opt<Segment3d> projected = firedata->project_on_firefront(seg, traj.conf.uav, t);
                if(projected) {
                    if(*projected != seg) {
                        // original is different than projection, replace it
                        traj.erase_segment(seg_id);
                        traj.insert_segment(*projected, seg_id);
                    }
                    seg_id++;
                } else {
                    // segment has no projection, remove it
                    traj.erase_segment(seg_id);
                }
            }
        }
    }

    /** Goes through all trajectories and erase segments causing very tight loops. */
     void smooth_trajectory() {
        for(auto &traj : core.trajectories) {
            size_t seg_id = traj.first_modifiable();
            while(seg_id < traj.last_modifiable()) {
                const Segment3d& current = traj[seg_id];
                const Segment3d& next = traj[seg_id+1];

                const double euclidian_dist_to_next = current.end.as_point().dist(next.start.as_point());
                const double dubins_dist_to_next = traj.conf.uav.travel_distance(current.end, next.start);

                if(dubins_dist_to_next / euclidian_dist_to_next > 2.)
                    // tight loop, erase next and stay on this segment to check for tight loops on the new next.
                    traj.erase_segment(seg_id+1);
                else
                    // no loop detected, go to next
                    seg_id++;
            }
        }
    }

    /* Get the cells of the Raster
     * */
    template <typename GenRaster>
    static opt<std::vector<Cell>> segment_trace(const Segment3d& segment, const GenRaster& raster) {
        std::vector<Cell> trace = {};

        /* If part of the segment is out of raster bounds, we do nothing.*/
        if (!raster.is_in(segment.start) || !raster.is_in(segment.end) ) {
            return {};
        }

        Cell cell_start = raster.as_cell(segment.start);
        Cell cell_end = raster.as_cell(segment.end);

        size_t c_x = cell_start.x;
        size_t c_y = cell_start.y;

        long dx = abs(static_cast<long>(cell_end.x) - static_cast<long>(cell_start.x));
        long sx = cell_start.x<cell_end.x ? 1 : -1;
        long dy = -abs(static_cast<long>(cell_end.y) - static_cast<long>(cell_start.y));
        long sy = cell_start.y<cell_end.y ? 1 : -1;
        long err = dx + dy;
        long e2 = 0;

        trace.push_back(Cell{c_x, c_y});
        while(c_x != cell_end.x || c_y != cell_end.y) {
            e2 = 2*err;
            if (e2 >= dy) {
                err += dy; c_x += sx;
            }
            if (e2 <= dx) {
                err += dx; c_y += sy;
            }
            trace.push_back(Cell{c_x, c_y});
        }
        return trace;
    }

    /* Get the cells of the Raster
     * */
    template <typename GenRaster>
    static opt<std::vector<Cell>> segment_trace(const Segment3d& segment, const double view_width,
                                                const double view_depth, const GenRaster& raster) {
        std::vector<Cell> trace = {};

        // computes visibility rectangle
        // the rectangle is placed right in front of the plane. Its width is given by the view width of the UAV
        // (half of it on each side) and as length equal to the length of the segment + the view depth of the UAV
        const double w = view_width; // width of rect
        const double l = /*view_depth + */segment.length; // length of rect

        // coordinates of A, B and C corners, where AB and BC are perpendicular. D is the corner opposing A
        // UAV is at the center of AB
        const double ax = segment.start.x + cos(segment.start.dir + M_PI/2) * w/2;
        const double ay = segment.start.y + sin(segment.start.dir + M_PI/2) * w/2;
        const double bx = segment.start.x - cos(segment.start.dir + M_PI/2) * w/2;
        const double by = segment.start.y - sin(segment.start.dir + M_PI/2) * w/2;
        const double cx = ax + cos(segment.start.dir) * l;
        const double cy = ay + sin(segment.start.dir) * l;
        const double dx = bx + cos(segment.start.dir) * l;
        const double dy = by + sin(segment.start.dir) * l;

        // limits of the area in which to search for visible points
        // this is a subset of the raster that strictly contains the visibility rectangle
        const double min_x = max(min(min(ax, bx), min(cx, dx)) - raster.cell_width, raster.x_offset);
        const double max_x = min(max(max(ax, bx), max(cx, dx)) + raster.cell_width, raster.x_offset + raster.x_width * raster.cell_width- raster.cell_width/2);
        const double min_y = max(min(min(ay, by), min(cy, dy)) - raster.cell_width, raster.y_offset);
        const double max_y = min(max(max(ay, by), max(cy, dy)) + raster.cell_width, raster.y_offset + raster.y_height * raster.cell_width - raster.cell_width/2);

        // coordinates of where to start the search, centered on a cell
        const size_t start_x = raster.x_coords(raster.x_index(min_x));
        const size_t start_y = raster.y_coords(raster.y_index(min_y));

        // for each point possibly in the rectangle check if it is in the visible area and mark it as pending/visible when necessary
        for(double ix=start_x; ix<=max_x; ix+=raster.cell_width) {
            for(double iy=start_y; iy<=max_y; iy+=raster.cell_width) {
                if(in_rectangle(ix, iy, ax, ay, bx, by, cx, cy)) {
                    // corresponding point in matrix coordinates
                    trace.push_back(Cell{raster.x_index(ix), raster.y_index(iy)});
                }
            }
        }

        return trace;
    }

    /** Dot product of two vectors */
    static inline double dot(double x1, double y1, double x2, double y2) {
        return x1 * x2 + y1 * y2;
    }

    /** Returns true if the point (x,y) is in the rectangle defined by its two perpendicular sides AB and AC */
    static bool in_rectangle(double x, double y, double ax, double ay, double bx, double by, double cx, double cy) {
        const double dot_ab_am = dot(bx-ax, by-ay, x-ax, y-ay);
        const double dot_ab_ab = dot(bx-ax, by-ay, bx-ax, by-ay);
        const double dot_ac_am = dot(cx-ax, cy-ay, x-ax, y-ay);
        const double dot_ac_ac = dot(cx-ax, cy-ay, cx-ax, cy-ay);
        return 0 <= dot_ab_am && dot_ab_am <= dot_ab_ab &&
               0 <= dot_ac_am && dot_ac_am <= dot_ac_ac;
    }

private:
    /** Constants used for computed the cost associated to a pair of points.
     * The cost is MAX_INDIVIDUAL_COST if the distance between two points
     * is >= MAX_INFORMATIVE_DISTANCE. It is be 0 if the distance is 0 and scales linearly between the two. */
    const double MAX_INFORMATIVE_DISTANCE = 500.;

    /** If a point is less than REDUNDANT_OBS_DIST aways from another observation, it useless to observe it.
     * This is defined such that those point are in the visible area when pictured. */
    const double REDUNDANT_OBS_DIST = 50.;
};



#endif //PLANNING_CPP_PLAN_H
