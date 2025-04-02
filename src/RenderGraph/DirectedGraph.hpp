#pragma once

#include <memory>
#include <iostream>
#include <locale>
#include <codecvt>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <variant>

#include <cstdint>
#include <cmath>
#include <limits>

#include "nvh/nvprint.hpp"

namespace ame {
class DirectedGraph {
public:
    static constexpr uint32_t kInvalidIndex = std::numeric_limits<std::uint32_t>::max() - 1;

    class Node;
    class Edge;

    uint32_t addNode() {
        mNodes[mCurrentNodeIndex] = Node();
        return mCurrentNodeIndex++;
    }

    std::unordered_set<uint32_t> removeNode(uint32_t index) {
        if (mNodes.find(index) == mNodes.end())
        {
            LOGW("Can't remove node from DirectGraph, node ID doesn't exist");
            return {};
        }
        // Remove all edges connected to the node
        std::unordered_set<uint32_t> removedEdges;
        for (auto& edge : mEdges) {
            if (edge.second.src == index || edge.second.dst == index) {
                removedEdges.insert(edge.first);
            }
        }
        mNodes.erase(index);
        return removedEdges;
    }

    uint32_t addEdge(uint32_t src, uint32_t dst) {
        mEdges[mCurrentEdgeIndex] = Edge(src, dst);
        return mCurrentEdgeIndex++;
    }
    void removeNode(uint32_t index);
    void removeEdge(uint32_t src, uint32_t dst);
    void clear();
    bool isCyclic() const;
    
    DirectedGraph();
    ~DirectedGraph();
    
private:
    std::unordered_map<uint32_t, Node> mNodes;
    std::unordered_map<uint32_t, Edge> mEdges;
    uint32_t mCurrentNodeIndex = 0;
    uint32_t mCurrentEdgeIndex = 0;
};
} // namespace ame