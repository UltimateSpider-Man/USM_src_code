#pragma once

// Surface continuity for explicitly posed, unrigged custom imports. Geometry
// and corner attributes remain owned by the importer; only skin weights change.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace modmesh { namespace donorsurface {

template<class Corner> struct Surface {
    int64_t model = 0;
    std::vector<Corner> *corners = nullptr;
};
struct Stats {
    size_t positions = 0, edges = 0, recoveredFamilies = 0;
    unsigned iterations = 0;
    double maximumGroupError = 0;
};
namespace detail {
struct Weight { int bone; double value; };
struct Key {
    int64_t model;
    uint32_t p[3];
    bool operator==(const Key &b) const {
        return model == b.model && p[0] == b.p[0] && p[1] == b.p[1] && p[2] == b.p[2];
    }
};
struct KeyHash {
    size_t operator()(const Key &k) const {
        return size_t(uint64_t(k.model) ^ (uint64_t(k.model) >> 32))
             ^ size_t(k.p[0])*73856093u ^ size_t(k.p[1])*19349663u ^ size_t(k.p[2])*83492791u;
    }
};
inline void accumulate(std::vector<Weight> &weights, int bone, double value) {
    for (auto &w : weights) if (w.bone == bone) { w.value += value; return; }
    weights.push_back({bone, value});
}
inline bool fail(std::string *why, const char *message) {
    if (why) *why = message;
    return false;
}
}

// Grid is the native, already posed donor cloud. Its entries still carry
// validated native bone indices. Separate per-family grids provide real skin
// influences when local boundary smoothing introduces a family absent at a
// source vertex. Confident surface regions remain fixed during smoothing.
template<class Corner, class Grid>
bool smooth(const std::vector<Surface<Corner>> &surfaces,
            const std::vector<uint8_t> &family, const Grid &native,
            Stats *result = nullptr, std::string *why = nullptr)
{
    using namespace detail;
    struct Node {
        float p[3];
        std::vector<Weight> original;
        std::array<double, 3> total{{0,0,0}};
        float bone[4] = {-1,-1,-1,-1}, weight[4] = {0,0,0,0};
    };
    if (family.empty() || family.size() > 1024 || native.donors.empty())
        return fail(why, "missing validated native surface donors");
    for (auto f : family) if (f > 2) return fail(why, "invalid native pose family");
    for (const auto &donor : native.donors) {
        for (float p : donor.p) if (!std::isfinite(p)) return fail(why, "non-finite native donor position");
        for (unsigned lane = 0; lane < 4; ++lane) {
            const double w = donor.bw[lane], b = donor.bi[lane];
            if (!std::isfinite(w) || w < 0) return fail(why, "invalid native donor weight");
            if (w > 0 && (!std::isfinite(b) || b < 0 || b >= family.size() || std::floor(b) != b))
                return fail(why, "invalid native donor bone");
        }
    }
    size_t cornerCount = 0;
    for (const auto &surface : surfaces) {
        if (!surface.corners || surface.corners->size() % 3)
            return fail(why, "surface is not complete triangles");
        cornerCount += surface.corners->size();
    }
    if (!cornerCount) return fail(why, "empty source surface");
    std::unordered_map<Key, uint32_t, KeyHash> lookup;
    lookup.reserve(cornerCount/3);
    std::vector<Node> nodes;
    std::vector<uint32_t> cornerNodes;
    cornerNodes.reserve(cornerCount);
    std::vector<uint64_t> links;
    links.reserve(cornerCount);
    float minimum[3] = {1e30f,1e30f,1e30f}, maximum[3] = {-1e30f,-1e30f,-1e30f};
    for (const auto &surface : surfaces) {
        uint32_t triangle[3];
        size_t ordinal = 0;
        for (const auto &c : *surface.corners) {
            const float p[3] = {c.px,c.py,c.pz};
            Key key{}; key.model = surface.model;
            for (unsigned d = 0; d < 3; ++d) {
                if (!std::isfinite(p[d])) return fail(why, "non-finite source position");
                const float canonical = p[d] == 0 ? 0.f : p[d];
                std::memcpy(&key.p[d], &canonical, sizeof(float));
                minimum[d] = std::min(minimum[d], p[d]);
                maximum[d] = std::max(maximum[d], p[d]);
            }
            auto inserted = lookup.emplace(key, uint32_t(nodes.size()));
            if (inserted.second) {
                if (nodes.size() >= std::numeric_limits<uint32_t>::max())
                    return fail(why, "source surface exceeds index capacity");
                Node node{};
                std::copy(p, p+3, node.p);
                nodes.push_back(std::move(node));
            }
            const uint32_t index = inserted.first->second;
            auto &node = nodes[index];
            double sum = 0;
            for (unsigned lane = 0; lane < 4; ++lane) {
                const double w = c.bw[lane], b = c.bi[lane];
                if (!std::isfinite(w) || w < 0) return fail(why, "invalid transferred weight");
                if (w == 0) continue;
                if (!std::isfinite(b) || b < 0 || b >= family.size() || std::floor(b) != b)
                    return fail(why, "invalid transferred native bone");
                sum += w;
            }
            if (!(sum > 0)) return fail(why, "weightless source surface vertex");
            for (unsigned lane = 0; lane < 4; ++lane) if (c.bw[lane] > 0) {
                const int bone = int(c.bi[lane]);
                const double w = double(c.bw[lane])/sum;
                accumulate(node.original, bone, w);
                node.total[family[size_t(bone)]] += w;
            }
            cornerNodes.push_back(index);
            triangle[ordinal++ % 3] = index;
            if (ordinal % 3 == 0) for (unsigned edge = 0; edge < 3; ++edge) {
                uint32_t a = triangle[edge], b = triangle[(edge+1)%3];
                if (a == b) continue;
                if (a > b) std::swap(a,b);
                links.push_back((uint64_t(a)<<32)|b);
            }
        }
    }
    // No corners are welded or reordered. Only the auxiliary adjacency graph
    // shares exact positions, scoped to one original FBX model.
    lookup.clear(); lookup.rehash(0);
    std::sort(links.begin(), links.end());
    links.erase(std::unique(links.begin(), links.end()), links.end());
    struct Edge { uint32_t a,b; double weight, length; };
    std::vector<Edge> edges; edges.reserve(links.size());
    std::vector<double> degree(nodes.size());
    double extent = 0;
    for (unsigned d = 0; d < 3; ++d)
        extent = std::max(extent, double(maximum[d])-minimum[d]);
    const double minimumEdge = std::max(extent*.0003, 1e-7);
    for (uint64_t link : links) {
        const uint32_t a = uint32_t(link>>32), b = uint32_t(link);
        double squared = 0;
        for (unsigned d = 0; d < 3; ++d) {
            const double delta = double(nodes[a].p[d])-nodes[b].p[d]; squared += delta*delta;
        }
        const double length = std::sqrt(squared);
        const double weight = 1/std::max(length, minimumEdge);
        edges.push_back({a,b,weight,length}); degree[a] += weight; degree[b] += weight;
    }
    links.clear(); links.shrink_to_fit();
    using Totals = std::array<double,3>;
    std::vector<Totals> current(nodes.size()), next(nodes.size());
    for (size_t i = 0; i < nodes.size(); ++i) {
        const double sum = nodes[i].total[0]+nodes[i].total[1]+nodes[i].total[2];
        for (unsigned f = 0; f < 3; ++f) current[i][f] = nodes[i].total[f]/sum;
    }
    // Repeated unanchored averaging spreads arm ownership through the entire
    // connected torso and legs. Even a small tail then reserves an arm lane
    // and discards one of the four substantial native leg influences. Solve
    // only near the originally transferred family boundaries, with unchanged
    // source weights as fixed boundary conditions. Distance follows original
    // triangle edges, so spatially nearby hands and thighs stay separate.
    const double boundaryWidth = std::max(extent*.025, 1e-7);
    std::vector<std::vector<std::pair<uint32_t,double>>> adjacent(nodes.size());
    std::vector<double> distance(nodes.size(), std::numeric_limits<double>::infinity());
    using Visit = std::pair<double,uint32_t>;
    std::priority_queue<Visit,std::vector<Visit>,std::greater<Visit>> pending;
    auto seed = [&](uint32_t vertex) {
        if (distance[vertex] == 0) return;
        distance[vertex] = 0;
        pending.push({0,vertex});
    };
    std::vector<unsigned> dominant(nodes.size());
    for (size_t i = 0; i < nodes.size(); ++i) {
        unsigned active = 0;
        for (unsigned f = 0; f < 3; ++f) {
            if (current[i][f] > current[i][dominant[i]]) dominant[i] = f;
            if (current[i][f] > 1e-6) ++active;
        }
        if (active > 1) seed(uint32_t(i));
    }
    for (const auto &edge : edges) {
        adjacent[edge.a].push_back({edge.b,edge.length});
        adjacent[edge.b].push_back({edge.a,edge.length});
        if (dominant[edge.a] != dominant[edge.b]) { seed(edge.a); seed(edge.b); }
    }
    while (!pending.empty()) {
        const auto visit = pending.top(); pending.pop();
        if (visit.first != distance[visit.second]) continue;
        for (const auto &neighbor : adjacent[visit.second]) {
            const double candidate = visit.first+neighbor.second;
            if (candidate <= boundaryWidth && candidate < distance[neighbor.first]) {
                distance[neighbor.first] = candidate;
                pending.push({candidate,neighbor.first});
            }
        }
    }
    adjacent.clear(); adjacent.shrink_to_fit();
    constexpr unsigned iterations = 512;
    for (unsigned iteration = 0; iteration < iterations; ++iteration) {
        std::fill(next.begin(), next.end(), Totals{{0,0,0}});
        for (const auto &edge : edges) for (unsigned f = 0; f < 3; ++f) {
            next[edge.a][f] += edge.weight*current[edge.b][f];
            next[edge.b][f] += edge.weight*current[edge.a][f];
        }
        for (size_t i = 0; i < nodes.size(); ++i) for (unsigned f = 0; f < 3; ++f)
            next[i][f] = distance[i] <= boundaryWidth && degree[i] > 0
                ? .5*(current[i][f]+next[i][f]/degree[i]) : current[i][f];
        current.swap(next);
    }
    Grid familyDonors[3];
    bool familyReady[3] = {false,false,false};
    auto prepareFamily = [&](unsigned f) {
        if (familyReady[f]) return;
        familyReady[f] = true;
        for (const auto &source : native.donors) {
            auto donor = source;
            double sum = 0;
            for (unsigned lane = 0; lane < 4; ++lane) {
                const float bone = donor.bi[lane], weight = donor.bw[lane];
                if (weight > 0 && std::isfinite(bone) && bone >= 0
                    && bone < family.size() && std::floor(bone) == bone && family[size_t(bone)] == f)
                    sum += weight;
                else { donor.bi[lane] = -1; donor.bw[lane] = 0; }
            }
            if (!(sum > 0)) continue;
            for (auto &w : donor.bw) w = float(w/sum);
            familyDonors[f].donors.push_back(donor);
        }
        familyDonors[f].build();
    };
    Stats stats; stats.positions = nodes.size(); stats.edges = edges.size(); stats.iterations = iterations;
    for (size_t i = 0; i < nodes.size(); ++i) {
        auto &node = nodes[i];
        std::vector<Weight> candidates[3];
        for (const auto &weight : node.original) candidates[family[size_t(weight.bone)]].push_back(weight);
        float desired[3];
        const double originalSum = current[i][0]+current[i][1]+current[i][2];
        Totals retainedFamily{};
        double desiredSum = 0;
        for (unsigned f = 0; f < 3; ++f) {
            const double normalized = current[i][f]/originalSum;
            // Diffusion has minute tails over an entire connected body.
            // They must not reserve bone lanes or expand draw palettes at
            // the expense of substantial native leg/finger influences.
            retainedFamily[f] = normalized <= 1e-6 ? 0 : normalized;
            desiredSum += retainedFamily[f];
        }
        unsigned largest = 0;
        for (unsigned f = 0; f < 3; ++f) {
            desired[f] = float(retainedFamily[f]/desiredSum);
            if (desired[f] > desired[largest]) largest = f;
        }
        desired[largest] = 1.f-desired[(largest+1)%3]-desired[(largest+2)%3];
        for (unsigned f = 0; f < 3; ++f) if (desired[f] > 0 && candidates[f].empty()) {
            prepareFamily(f);
            // A missing group is supplied by actual native bones in that
            // group, never a guessed skeleton index or the unrelated root.
            const auto *donor = familyDonors[f].nearest(node.p);
            if (!donor) return fail(why, "no native donor in a required pose family");
            for (unsigned lane = 0; lane < 4; ++lane) if (donor->bw[lane] > 0)
                accumulate(candidates[f], int(donor->bi[lane]), donor->bw[lane]);
            ++stats.recoveredFamilies;
        }
        unsigned kept[3] = {0,0,0}, used = 0;
        double candidateSum[3] = {0,0,0};
        for (unsigned f = 0; f < 3; ++f) {
            auto &list = candidates[f];
            std::sort(list.begin(), list.end(), [](const Weight &a, const Weight &b) {
                return a.value != b.value ? a.value > b.value : a.bone < b.bone;
            });
            for (const auto &w : list) candidateSum[f] += w.value;
            if (desired[f] > 0) { kept[f] = 1; ++used; }
        }
        while (used < 4) {
            int best = -1; double score = -1;
            for (unsigned f = 0; f < 3; ++f) if (desired[f] > 0 && kept[f] < candidates[f].size()) {
                const double value = desired[f]*candidates[f][kept[f]].value/candidateSum[f];
                if (value > score) { score = value; best = int(f); }
            }
            if (best < 0) break;
            ++kept[best]; ++used;
        }
        unsigned lane = 0;
        for (unsigned f = 0; f < 3; ++f) if (kept[f]) {
            double retained = 0; for (unsigned k = 0; k < kept[f]; ++k) retained += candidates[f][k].value;
            float written = 0;
            for (unsigned k = 0; k < kept[f]; ++k, ++lane) {
                node.bone[lane] = float(candidates[f][k].bone);
                node.weight[lane] = k+1 == kept[f] ? std::max(0.f, desired[f]-written)
                    : float(desired[f]*candidates[f][k].value/retained);
                written += node.weight[lane];
            }
            stats.maximumGroupError = std::max(stats.maximumGroupError, std::abs(double(written)-desired[f]));
        }
        double finalSum = 0;
        for (unsigned k = 0; k < 4; ++k) {
            if (!std::isfinite(node.weight[k]) || node.weight[k] < 0)
                return fail(why, "non-finite compressed surface weight");
            finalSum += node.weight[k];
        }
        if (std::abs(finalSum-1) > 1e-6) return fail(why, "compressed surface weights are not normalized");
    }
    // Commit only after every vertex has valid actual native influences.
    size_t corner = 0;
    for (const auto &surface : surfaces) for (auto &c : *surface.corners) {
        const auto &node = nodes[cornerNodes[corner++]];
        std::copy(node.bone, node.bone+4, c.bi);
        std::copy(node.weight, node.weight+4, c.bw);
    }
    if (result) *result = stats;
    return true;
}

}} // namespace modmesh::donorsurface
