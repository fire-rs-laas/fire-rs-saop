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

#include <memory>
#include <queue>
#include <stack>

#include "utility.hpp"
#include "../core/trajectory.hpp"
#include "../core/fire_data.hpp"
#include "../core/raster.hpp"
#include "../core/trajectories.hpp"
#include "../core/updates/updates.hpp"
#include "../ext/json.hpp"

#include "../firemapping/ghostmapper.hpp"

namespace SAOP {

    using json = nlohmann::json;

    struct Plan;
    typedef shared_ptr<Plan> PlanPtr;

    struct Plan {
        TimeWindow time_window; /* Cells outside the range are not considered in possible observations */
        vector<PointTimeWindow> possible_observations;
        vector<PositionTime> observed_previously;

        Plan(std::vector<TrajectoryConfig> traj_confs, std::shared_ptr<FireData> fire_data, TimeWindow tw,
             std::vector<PositionTime> observed_previously = {});

        Plan(std::string name, std::vector<TrajectoryConfig> traj_confs, std::shared_ptr<FireData> fire_data,
             TimeWindow tw, std::vector<PositionTime> observed_previously = {});

        Plan(std::string name, std::vector<TrajectoryConfig> traj_confs, std::shared_ptr<FireData> fire_data,
             TimeWindow tw, GenRaster<double> utility);

        Plan(std::string name, std::vector<Trajectory> trajectories, std::shared_ptr<FireData> fire_data, TimeWindow tw,
             std::vector<PositionTime> observed_previously = {});

        Plan(std::string name, std::vector<Trajectory> trajectories, std::shared_ptr<FireData> fire_data, TimeWindow tw,
             GenRaster<double> utility);

        Plan(std::string name, Trajectories trajectories, std::shared_ptr<FireData> fire_data, TimeWindow tw,
                     std::vector<PositionTime> observed_previously, GenRaster<double> utility);

        ~Plan() = default;

        // copy constructor
        Plan(const Plan& plan) = default;

        // move constructor
        Plan(Plan&& plan) = default;

        // copy assignment operator
        Plan& operator=(const Plan& plan) = default;

        // move assignment operator
        Plan& operator=(Plan&& plan) = default;

        json metadata();

        std::string name() const { return plan_name; }

        /** A plan is valid iff all trajectories are valid (match their configuration. */
        bool is_valid() const {
            return trajs.is_valid();
        }

        const Trajectories& trajectories() const {
            return trajs;
        }

        const FireData& firedata() const {
            return *fire_data;
        }

        /* Replace plan firedata */
        void firedata(shared_ptr<FireData> fdata) {
            fire_data = std::move(fdata);
            u_map.reset(trajs);
        }

        /** Sum of all trajectory durations. */
        double duration() const {
            return trajs.duration();
        }

        /* Utility of the plan */
        double utility() const {
           return u_map.utility();
        }

        GenRaster<double> utility_map() const {
            return u_map.utility_map();
        }

        size_t num_segments() const {
            return trajs.num_segments();
        }

        /** All observations in the plan. Computed by taking the visibility center of all segments.
         * Each observation is tagged with a time, corresponding to the start time of the segment.*/
        vector<PositionTime> observations() const {
            //return observations(time_window);
            return observations_full();
        }

        /** All observations in the plan. Computed assuming we observe at any time, not only when doing a segment*/
        vector<PositionTime> observations_full() const;

        /* Observations done within an arbitrary time window
         */
        vector<PositionTime> observations(const TimeWindow& tw) const;

        /*All the positions observed by the UAV camera*/
        vector<PositionTime> view_trace(const TimeWindow& tw) const;

        /*All the positions observed by the UAV camera*/
        vector<PositionTime> view_trace() const {
            return view_trace(time_window);
        }

        void insert_segment(size_t traj_id, const Segment3d& seg, size_t insert_loc, bool do_post_processing = false);

        void erase_segment(size_t traj_id, size_t at_index, bool do_post_processing = false);

        void replace_segment(size_t traj_id, size_t at_index, const Segment3d& by_segment);

        void
        replace_segment(size_t traj_id, size_t at_index, size_t n_replaced, const std::vector<Segment3d>& segments);

        PReversibleTrajectoriesUpdate update(PReversibleTrajectoriesUpdate u, bool do_post_processing = false);

        void freeze_before(double time);

        void freeze_trajectory(std::string traj);

        void post_process() {
            project_on_fire_front();
            smooth_trajectory();
            u_map.reset(trajs);
        }

        /** Make sure every segment makes an observation, i.e., that the picture will be taken when the fire in traversing the main cell.
         *
         * If this is not the case for a given segment, its is projected on the firefront.
         * */
        void project_on_fire_front();

        /** Goes through all trajectories and erase segments causing very tight loops. */
        void smooth_trajectory();

        /* Get the cells of the Raster
         * */
        template<typename GenRaster>
        static opt<std::vector<Cell>> segment_trace(const Segment3d& segment, double view_width,
                                                    double view_depth, const GenRaster& raster) {
            return SAOP::RasterMapper::segment_trace<GenRaster>(segment, view_width, view_depth, raster);
        }

    private:
        std::string plan_name = "unnamed";
        Trajectories trajs;
        shared_ptr<FireData> fire_data;
        Utility u_map;
    };
}

#endif //PLANNING_CPP_PLAN_H
