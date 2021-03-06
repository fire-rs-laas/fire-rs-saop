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

#ifndef PLANNING_CPP_RASTER_H
#define PLANNING_CPP_RASTER_H

#include <valarray>
#include <stdexcept>
#include <unordered_set>

#include <zlib.h>

#include <string>

#include "waypoint.hpp"
#include "../ext/optional.hpp"
#include "../ext/optional.hpp"

namespace SAOP {

    struct Cell final {
        size_t x;
        size_t y;

        Cell() = default;

        Cell(const size_t x, const size_t y) : x(x), y(y) {}

        bool operator!=(Cell& pt) const { return x != pt.x || y != pt.y; }

        bool operator==(const Cell& pt) const { return x == pt.x && y == pt.y; }

        Cell operator+(const Cell& pt) const { return Cell{x + pt.x, y + pt.y}; }

        Cell operator-(const Cell& pt) const { return Cell{x - pt.x, y - pt.y}; }

        friend std::ostream& operator<<(std::ostream& stream, const Cell& pt) {
            return stream << "(" << pt.x << ", " << pt.y << ")";
        }
    };

    struct CellHash {
        size_t operator()(const Cell& x) const noexcept {
            return std::hash<size_t>()(x.x) ^ std::hash<size_t>()(x.y);
        }
    };

    template<typename T>
    struct GenRaster {
        std::vector<T> data;

        // Metadata
        size_t x_width;
        size_t y_height;
        double x_offset;
        double y_offset;
        double cell_width;

        GenRaster<T>(std::vector<T> data, size_t x_width, size_t y_height, double x_offset, double y_offset,
                     double cell_width) :
                data(data), x_width(x_width), y_height(y_height),
                x_offset(x_offset), y_offset(y_offset), cell_width(cell_width) {
            ASSERT(data.size() == x_width * y_height);
        }

        GenRaster<T>(size_t x_width, size_t y_height, double x_offset, double y_offset, double cell_width)
                : GenRaster<T>(std::vector<T>(x_width * y_height, 0), x_width, y_height, x_offset, y_offset,
                               cell_width) {}

        GenRaster<T>(const GenRaster<T>& like, T fill)
                : GenRaster<T>(std::vector<T>(like.x_width * like.y_height, fill), like.x_width, like.y_height,
                               like.x_offset,
                               like.y_offset, like.cell_width) {}

        friend std::ostream& operator<<(std::ostream& stream, const GenRaster<T>& raster) {
            stream << "GenRaster(" << typeid(T).name() << ", "
                   << "origin=(" << raster.x_offset << ", " << raster.y_offset << "), "
                   << "size=(" << raster.x_width << ", " << raster.y_height << "), "
                   << "cell_size=" << raster.cell_width
                   << ")";
            // TODO Add uav list
            return stream;
        }

        /* Reconstruct a GenRaster<T> from an compressed binary encoded version of it*/
        static GenRaster<T> decode(const std::vector<char>& encoded_raster) {
            // First two bytes are the magic code: 0xF13E big-endian

            static constexpr size_t header_size = sizeof(uint16_t) +  // magic number
                                                  sizeof(uint16_t) + // SRSID
                                                  2 * sizeof(uint64_t) +  // offset
                                                  2 * sizeof(double) +  // size
                                                  sizeof(double);  // cell size
            if (encoded_raster.size() <= header_size) {
                throw std::invalid_argument("Malformed raster");
            }

            // TODO: Store the SRSID into the GenRaster object
            uint64_t srsid;
            uint64_t x_width;
            uint64_t y_height;
            double x_offset;
            double y_offset;
            double cell_width;
            uint64_t uncomp_data_size;

            auto raster_bin_it = encoded_raster.begin();

            char magic_number[2] = {static_cast<char>(0xF1), static_cast<char>(0x3E)};

            // Check magic number
            if (!((*raster_bin_it == magic_number[0]) &&
                  (*(raster_bin_it + 1) == magic_number[1]))) {
                throw std::invalid_argument("Malformed raster");
            }
            raster_bin_it += sizeof(uint16_t);
            srsid = *reinterpret_cast<const uint64_t*>(&(*raster_bin_it));
            raster_bin_it += sizeof(uint64_t);
            x_width = *reinterpret_cast<const uint64_t*>(&(*raster_bin_it));
            raster_bin_it += sizeof(uint64_t);
            y_height = *reinterpret_cast<const uint64_t*>(&(*raster_bin_it));
            raster_bin_it += sizeof(uint64_t);
            x_offset = *reinterpret_cast<const double*>(&(*raster_bin_it));
            raster_bin_it += sizeof(double);
            y_offset = *reinterpret_cast<const double*>(&(*raster_bin_it));
            raster_bin_it += sizeof(double);
            cell_width = *reinterpret_cast<const double*>(&(*raster_bin_it));
            raster_bin_it += sizeof(double);

            uncomp_data_size = x_width * y_height * sizeof(double);

            std::vector<char> data_char = std::vector<char>(uncomp_data_size);
            size_t destlen = data_char.size();
            unsigned char* dest = reinterpret_cast<unsigned char*>(data_char.data());
            const unsigned char* src = reinterpret_cast<const unsigned char*>(&(*raster_bin_it));

            int uncomp_res = uncompress(dest, &destlen, src, encoded_raster.size() - header_size);
            if (uncomp_res != Z_OK) {
                switch (uncomp_res) {
                    case Z_MEM_ERROR:
                        throw std::invalid_argument("zlib Z_MEM_ERROR");
                    case Z_BUF_ERROR:
                        throw std::invalid_argument("zlib Z_BUF_ERROR");
                    case Z_DATA_ERROR:
                        throw std::invalid_argument("zlib Z_DATA_ERROR");
                    default:
                        throw std::invalid_argument("unknown zlib error");
                }

            }
            std::vector<T> data = std::vector<T>();
            for (auto it = data_char.begin(); it < data_char.end(); it += sizeof(T)) {
                data.emplace_back(*reinterpret_cast<const T*>(&(*it)));
            }
            return GenRaster<T>(data, x_width, y_height, x_offset, y_offset, cell_width);
//            return GenRaster<T>(2, 2, 1.0, 1.0, 5.0);
        }

        template<typename U>
        static void serialize(const U& obj, std::vector<char>& buffer) {
            char const* obj_begin = reinterpret_cast<char const*>(&obj);
            std::copy(obj_begin, obj_begin + sizeof(U), std::back_inserter(buffer));
        }

        /* Encode this raster as a binary sequence. */
        std::vector<char> encoded(uint64_t epsg_code) {
            std::vector<char> binary_raster = std::vector<char>();

            // Magic number
            binary_raster.emplace_back(0xF1);
            binary_raster.emplace_back(0x3E);

            // SRSID
            uint64_t srs_id = epsg_code;
            GenRaster::serialize<uint64_t>(srs_id, binary_raster);

            // x size
            uint64_t x_size = x_width;
            GenRaster::serialize<uint64_t>(x_size, binary_raster);

            // y size
            uint64_t y_size = y_height;
            GenRaster::serialize<uint64_t>(y_size, binary_raster);

            // x offset
            double x_off = x_offset;
            GenRaster::serialize<double>(x_off, binary_raster);

            // y offset
            double y_off = y_offset;
            GenRaster::serialize<double>(y_off, binary_raster);

            // cell width
            double cell_w = cell_width;
            GenRaster::serialize<double>(cell_w, binary_raster);

            // Compress fire map data
            unsigned long compressed_bound = compressBound(data.size());
            std::vector<char> compressed_buff = std::vector<char>(compressed_bound);
            int comp_res = compress(reinterpret_cast<unsigned char*>(compressed_buff.data()), &compressed_bound,
                                    reinterpret_cast<unsigned char*>(data.data()), data.size() * sizeof(T));
            if (comp_res != Z_OK) {
                switch (comp_res) {
                    case Z_MEM_ERROR:
                        throw std::invalid_argument("zlib Z_MEM_ERROR");
                    case Z_BUF_ERROR:
                        throw std::invalid_argument("zlib Z_BUF_ERROR");
                    case Z_DATA_ERROR:
                        throw std::invalid_argument("zlib Z_DATA_ERROR");
                    default:
                        throw std::invalid_argument("unknown zlib error");
                }

            }

            std::copy(compressed_buff.begin(), compressed_buff.begin() + compressed_bound,
                      std::back_inserter(binary_raster));
            return binary_raster;
        }

        /* Encode this raster as a binary sequence without compression. */
        std::vector<char> encoded_uncompressed(uint64_t epsg_code) {
            std::vector<char> binary_raster = std::vector<char>();

            // Magic number
            binary_raster.emplace_back(0xF1);
            binary_raster.emplace_back(0x3E);

            // SRSID
            uint64_t srs_id = epsg_code;
            GenRaster::serialize<uint64_t>(srs_id, binary_raster);

            // x size
            uint64_t x_size = x_width;
            GenRaster::serialize<uint64_t>(x_size, binary_raster);

            // y size
            uint64_t y_size = y_height;
            GenRaster::serialize<uint64_t>(y_size, binary_raster);

            // x offset
            double x_off = x_offset;
            GenRaster::serialize<double>(x_off, binary_raster);

            // y offset
            double y_off = y_offset;
            GenRaster::serialize<double>(y_off, binary_raster);

            // cell width
            double cell_w = cell_width;
            GenRaster::serialize<double>(cell_w, binary_raster);

            for (const auto& p : data) {
                GenRaster::serialize<double>(p, binary_raster);
            }

            return binary_raster;
        }

        void reset() {
            for (size_t i = 0; i < x_width * y_height; i++)
                data[i] = 0;
        }

        bool is_like(const GenRaster<T>& other) const {
            return x_width == other.x_width && y_height == other.y_height && x_offset == other.x_offset &&
                   y_offset == other.y_offset && cell_width == other.cell_width;
        }

        inline virtual T operator()(Cell cell) const {
            return (*this)(cell.x, cell.y);
        }

        inline virtual T operator()(size_t x, size_t y) const {
            ASSERT(x >= 0 && x < x_width);
            ASSERT(y >= 0 && y <= y_height);
            return data[x + y * x_width];
        }

        inline void set(size_t x, size_t y, T value) {
            data[x + y * x_width] = value;
        }

        inline void set(const Cell& c, T value) {
            set(c.x, c.y, value);
        }

        bool is_in(Cell cell) const {
            return cell.x < x_width && cell.y < y_height;
        }

        Position as_position(Cell cell) const {
            ASSERT(is_in(cell));
            return Position{x_coords(cell.x), y_coords(cell.y)};
        }

        double x_coords(size_t x_index) const {
            return x_offset + cell_width * x_index;
        }

        double y_coords(size_t y_index) const {
            return y_offset + cell_width * y_index;
        }

        bool is_in(const Waypoint& wp) const {
            return is_x_in(wp.x) && is_y_in(wp.y);
        }

        bool is_in(const Waypoint3d& wp) const {
            return is_x_in(wp.x) && is_y_in(wp.y);
        }

        bool is_in(const Position& pos) const {
            return is_x_in(pos.x) && is_y_in(pos.y);
        }

        bool is_in(const Position3d& pos) const {
            return is_x_in(pos.x) && is_y_in(pos.y);
        }

        bool is_x_in(double x_coord) const {
            return x_offset - cell_width / 2 <= x_coord && x_coord <= x_offset + cell_width * x_width - cell_width / 2;
        }

        bool is_y_in(double y_coord) const {
            return y_offset - cell_width / 2 <= y_coord && y_coord <= y_offset + cell_width * y_height - cell_width / 2;
        }

        Cell as_cell(const Waypoint& wp) const {
            ASSERT(is_in(wp));
            return Cell{x_index(wp.x), y_index(wp.y)};
        }

        Cell as_cell(const Waypoint3d& wp) const {
            ASSERT(is_in(wp));
            return Cell{x_index(wp.x), y_index(wp.y)};
        }

        Cell as_cell(const Position& pos) const {
            ASSERT(is_in(pos));
            return Cell{x_index(pos.x), y_index(pos.y)};
        }

        Cell as_cell(const Position3d& pos) const {
            ASSERT(is_in(pos));
            return Cell{x_index(pos.x), y_index(pos.y)};
        }

        size_t x_index(double x_coord) const {
            ASSERT(is_x_in(x_coord));
            return (size_t) lround((x_coord - x_offset) / cell_width);
        }

        size_t y_index(double y_coord) const {
            ASSERT(is_y_in(y_coord));
            return (size_t) lround((y_coord - y_offset) / cell_width);
        }

        /* Neighbor cells of cell taking in account raster limits*/
        std::vector<Cell> neighbor_cells(Cell cell) const {
            std::vector<Cell> neighbors = {};
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    // exclude any neighbor that is of the grid or is never ignited.
                    if (dx == 0 && dy == 0) continue;
                    if (cell.x == 0 && dx < 0) continue;
                    if (cell.x + dx >= x_width) continue;
                    if (cell.y == 0 && dy < 0) continue;
                    if (cell.y + dy >= y_height) continue;

                    neighbors.emplace_back(Cell{cell.x + dx, cell.y + dy});
                }
            }
            return neighbors;
        }

        typename std::vector<T>::iterator begin() {
            return data.begin();
        }

        typename std::vector<T>::iterator end() {
            return data.end();
        }

        typename std::vector<T>::const_iterator begin() const {
            return data.begin();
        }

        typename std::vector<T>::const_iterator end() const {
            return data.end();
        }
    };

    typedef GenRaster<double> DRaster;
    typedef GenRaster<long> LRaster;

    template<typename T>
    struct LocalRaster {
    public:

        std::vector<T> data;

        LocalRaster<T>(std::shared_ptr<GenRaster<T>> parent, std::vector<T> data, size_t x_width, size_t y_height,
                       Cell offset) : _parent(std::move(parent)), data(data), width(x_width), height(y_height),
                                      _offset(offset) {
            ASSERT(offset.x + x_width <= parent->x_width);
            ASSERT(offset.y + y_height <= parent->y_height);
            ASSERT(data.size() == x_width * y_height);
        }

        LocalRaster<T>(std::shared_ptr<GenRaster<T>> parent, size_t x_width, size_t y_height, Cell offset) :
                _parent(std::move(parent)), data(std::vector<T>(x_width * y_height, 0)), width(x_width),
                height(y_height), _offset(offset) {
            ASSERT(offset.x + x_width <= parent->x_width);
            ASSERT(offset.y + y_height <= parent->y_height);
        }

        /* Convert a Raster in a RasterUpdate*/
        LocalRaster<T>(GenRaster<T>&& raster, std::shared_ptr<GenRaster<T>> parent, size_t x_width, size_t y_height,
                       Cell offset) :
                _parent(std::move(parent)), data(std::move(raster.data)), width(x_width),
                height(y_height), _offset(offset) {
            ASSERT(offset.x + x_width <= parent->x_width);
            ASSERT(offset.y + y_height <= parent->y_height);
        }

        std::shared_ptr<GenRaster<T>> parent() const {
            return _parent;
        }

        Cell offset() const {
            return _offset;
        }

        void reset() {
            for (size_t i = 0; i < width * height; i++)
                data[i] = 0;
        }

        /* Get the data associated to a Cell in the ChildRaster refrence frame */
        inline virtual T operator()(Cell cell) const {
            return operator()(cell.x, cell.y);
        }

        /* Get the data associated to a Cell in the ChildRaster refrence frame */
        inline virtual T operator()(size_t x, size_t y) const {
            ASSERT(x >= 0 && x < width);
            ASSERT(y >= 0 && y <= height);
            return data[x + y * width];
        }

        void set(Cell cell, T value) {
            data[cell.x + cell.y * width] = value;
        }

        bool is_child_cell_in(Cell cell) const {
            return cell.x < width && cell.y < height;
        }

        bool is_parent_cell_in(Cell cell) const {
            return cell.x < width && cell.y < height;
        }

        Cell parent_cell(const Cell& child_raster_cell) const {
            return child_raster_cell + _offset;
        }

        Cell child_cell(const Cell& parent_raster_cell) const {
            return parent_raster_cell - _offset;
        }

        Position as_position(Cell cell) const {
            return _parent->as_position(parent_cell(cell));
        }

        bool is_in(const Waypoint& wp) const {
            return is_parent_cell_in(_parent->as_cell(wp));
        }

        bool is_in(const Waypoint3d& wp) const {
            return is_parent_cell_in(_parent->as_cell(wp));
        }

        bool is_in(const Position& pos) const {
            return is_parent_cell_in(_parent->as_cell(pos));
        }

        bool is_in(const Position3d& pos) const {
            return is_parent_cell_in(_parent->as_cell(pos));
        }

        Cell as_cell(const Waypoint& wp) const {
            ASSERT(is_in(wp));
            return child_cell(_parent->as_cell(wp));
        }

        Cell as_cell(const Waypoint3d& wp) const {
            ASSERT(is_in(wp));
            return child_cell(_parent->as_cell(wp));
        }

        Cell as_cell(const Position& pos) const {
            ASSERT(is_in(pos));
            return child_cell(_parent->as_cell(pos));
        }

        Cell as_cell(const Position3d& pos) const {
            ASSERT(is_in(pos));
            return child_cell(_parent->as_cell(pos));
        }

        bool applied() const {
            return _applied;
        }

        void apply_update() {
            ASSERT(!applied());

            auto parent_it = _parent->data.begin() + (_offset.x + _offset.y * _parent->x_width);
            auto update_it = data.begin();

            while (update_it <= data.end()) {
                std::copy(update_it, update_it + width, parent_it);
                std::advance(parent_it, _parent->x_width);
                std::advance(update_it, width);
            }
        }

    private:
        std::shared_ptr<GenRaster<T>> _parent;

        size_t width;
        size_t height;
        Cell _offset;

        bool _applied = false; // wether this update has been applyed or not
    };

    typedef LocalRaster<double> DLocalRaster;
    typedef LocalRaster<long> LLocalRaster;

    class RasterMapper {
    public:
        /* Get the cells resulting of mapping a maneuver into a GenRaster*/
        template<typename GenRaster>
        static opt<std::vector<Cell>> segment_trace(const Segment3d& segment, const double view_width,
                                                    const double view_depth, const GenRaster& raster) {
            // FIXME: Remove this horrible opt<vector<Cell>> creation!!!
//            std::vector<Cell> trace = {};

            std::unordered_set<Cell, CellHash> trace_set = {};

            // computes visibility rectangle
            // the rectangle is placed right in front of the plane. Its width is given by the view width of the UAV
            // (half of it on each side) and as length equal to the length of the segment + the view depth of the UAV
            const double w = view_width; // width of rect
            const double l = segment.length; // length of rect

            // coordinates of A, B and C corners, where AB and BC are perpendicular. D is the corner opposing A
            // UAV is at the center of AB
            const double ssx = segment.start.x - cos(segment.start.dir) * view_depth / 2;
            const double ssy = segment.start.y - sin(segment.start.dir) * view_depth / 2;

            const double ax = ssx + cos(segment.start.dir + M_PI_2) * w / 2;
            const double ay = ssy + sin(segment.start.dir + M_PI_2) * w / 2;
            const double bx = ssx - cos(segment.start.dir + M_PI_2) * w / 2;
            const double by = ssy - sin(segment.start.dir + M_PI_2) * w / 2;
            const double cx = ax + cos(segment.start.dir) * (l + view_depth);
            const double cy = ay + sin(segment.start.dir) * (l + view_depth);
            const double dx = bx + cos(segment.start.dir) * (l + view_depth);
            const double dy = by + sin(segment.start.dir) * (l + view_depth);

            // limits of the area in which to search for visible points
            // this is a subset of the raster that strictly contains the visibility rectangle
            const double min_x = std::min(std::max(std::min(std::min(ax, bx), std::min(cx, dx)) - raster.cell_width,
                                                   raster.x_offset + raster.cell_width / 2),
                                          raster.x_offset + raster.x_width * raster.cell_width - raster.cell_width / 2);
            const double max_x = std::max(std::min(std::max(std::max(ax, bx), std::max(cx, dx)) + raster.cell_width,
                                                   raster.x_offset + raster.x_width * raster.cell_width -
                                                   raster.cell_width / 2), raster.x_offset + raster.cell_width / 2);
            const double min_y = std::min(std::max(std::min(std::min(ay, by), std::min(cy, dy)) - raster.cell_width,
                                                   raster.y_offset + raster.cell_width / 2),
                                          raster.y_offset + raster.y_height * raster.cell_width -
                                          raster.cell_width / 2);
            const double max_y = std::max(std::min(std::max(std::max(ay, by), std::max(cy, dy)) + raster.cell_width,
                                                   raster.y_offset + raster.y_height * raster.cell_width -
                                                   raster.cell_width / 2), raster.y_offset + raster.cell_width / 2);

            // for each point possibly in the rectangle check if it is in the visible area and mark it as pending/visible when necessary
            for (double ix = min_x; ix <= max_x; ix += raster.cell_width / 2) {
                for (double iy = min_y; iy <= max_y; iy += raster.cell_width / 2) {
                    if (in_rectangle(ix, iy, ax, ay, bx, by, cx, cy)) {
                        // corresponding point in matrix coordinates
                        if (raster.is_x_in(ix) && raster.is_y_in(iy)) {
                            trace_set.insert(Cell{raster.x_index(ix), raster.y_index(iy)});
                        }
                    }
                }
            }

            std::vector<Cell> trace = std::vector<Cell>(trace_set.begin(), trace_set.end());

            return trace;
        }

    private:
        /** Dot product of two vectors */
        static inline double dot(double x1, double y1, double x2, double y2) {
            return x1 * x2 + y1 * y2;
        }

        /** Returns true if the point (x,y) is in the rectangle defined by its two perpendicular sides AB and AC */
        static bool in_rectangle(double x, double y, double ax, double ay, double bx, double by, double cx, double cy) {
            const double dot_ab_am = dot(bx - ax, by - ay, x - ax, y - ay);
            const double dot_ab_ab = dot(bx - ax, by - ay, bx - ax, by - ay);
            const double dot_ac_am = dot(cx - ax, cy - ay, x - ax, y - ay);
            const double dot_ac_ac = dot(cx - ax, cy - ay, cx - ax, cy - ay);
            return (0 <= dot_ab_am) && (dot_ab_am <= dot_ab_ab) &&
                   (0 <= dot_ac_am) && (dot_ac_am <= dot_ac_ac);
        }
    };
}
#endif //PLANNING_CPP_RASTER_H
