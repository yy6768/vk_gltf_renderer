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

#include <unordered_set>

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
        for (auto& edgeId: mNodes[index].mOutgoingEdges) {
            findEdgeToRemove<true>(mNodes[mEdges[edgeId].mSrc].mIncomingEdges, index, removedEdges);
        }
        for (auto& edgeId: mNodes[index].mIncomingEdges) {
            findEdgeToRemove<false>(mNodes[mEdges[edgeId].mDst].mOutgoingEdges, index, removedEdges);
        }
        mNodes.erase(index);
        return removedEdges;
    }

    uint32_t addEdge(uint32_t src, uint32_t dst) {
        mEdges[mCurrentEdgeIndex] = Edge(src, dst);
        return mCurrentEdgeIndex++;
    }
    void removeEdge(uint32_t src, uint32_t dst);
    void clear();
    bool isCyclic() const;
    
    DirectedGraph();
    ~DirectedGraph();

    class Node {
    public:
        Node() = default;
        uint32_t index;
        uint32_t getOutgoingEdgeCount() const { return (uint32_t)mOutgoingEdges.size(); }
        uint32_t getIncomingEdgeCount() const { return (uint32_t)mIncomingEdges.size(); }

        uint32_t getIncomingEdge(uint32_t i) const { return mIncomingEdges[i]; }
        uint32_t getOutgoingEdge(uint32_t i) const { return mOutgoingEdges[i]; }

    private:
        friend class DirectedGraph;
        std::vector<uint32_t> mIncomingEdges;
        std::vector<uint32_t> mOutgoingEdges;
    };

    class Edge {
    public:
        Edge() = default;
        uint32_t getSrc() const { return mSrc; }
        uint32_t getDst() const { return mDst; }
    private:
        friend class DirectedGraph;
        Edge(uint32_t src, uint32_t dst) : mSrc(src), mDst(dst) {}
        uint32_t mSrc = kInvalidIndex;
        uint32_t mDst = kInvalidIndex;
    };
    
    
private:
    std::unordered_map<uint32_t, Node> mNodes;
    std::unordered_map<uint32_t, Edge> mEdges;
    uint32_t mCurrentNodeIndex = 0;
    uint32_t mCurrentEdgeIndex = 0;

    /**
     * @brief Given a vector of edges, find the edges that connect to the given node index.
     * 
     * @tparam removeSrc If true, find the edges that connect to the given node index as the source node.
     * @param edges The vector of edges to search.
     * @param nodeIndex The node index to search for.
     * @param removeEdge The set of edges to remove.
     */
    template <bool removeSrc>
    void findEdgeToRemove(std::vector<uint32_t>& edges, uint32_t nodeIndex, std::unordered_set<uint32_t>& removeEdge) {
        for (size_t i = 0; i < edges.size(); ++i) {
            uint32_t edgeId = edges[i];
            const auto& edge = mEdges[edgeId];
            auto& otherNode = removeSrc ? edge.mSrc : edge.mDst;
            if (otherNode == nodeIndex) {
                removeEdge.insert(edgeId);
            }
        }
    }

    template <bool removeInput>
    void removeEdgeFromNode(uint32_t edgeId, Node& node) {
        auto vec = removeInput ? node.mIncomingEdges : node.mOutgoingEdges;
        for (auto e = vec.begin(); e != vec.end(); ++e) {
            if (*e == edgeId) {
                vec.erase(e);
                return;
            }
        }
        LOGE("Can't remove edge from node, edge ID doesn't exist");
        abort();
    }
};
} // namespace ame