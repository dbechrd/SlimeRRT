#pragma once
#include <array>
#include <variant>
#include <vector>

namespace RStar {
    template <typename T>
    struct Vector2 {
        T x;
        T y;
    };

    template <typename T>
    struct AABB {
        Vector2<T> min;
        Vector2<T> max;

        bool intersects(const AABB &other) const {
            return max.x > other.min.x &&
                   min.x < other.max.x &&
                   max.y > other.min.y &&
                   min.y > other.max.y;
        }
    };

    template <typename ValueType, size_t Max>
    struct RTreeNode {
        struct NodeEntry {
            AABB<float> bounds;
            RTreeNode *child;
        };
        struct ValueEntry {
            AABB<float> bounds;
            ValueType value;
        };

        using NodeArray  = std::array<NodeEntry, Max>;
        using ValueArray = std::array<ValueEntry, Max>;

        std::variant<
            std::array<NodeEntry, Max>,  // Inner nodes
            std::array<ValueEntry, Max>  // Leaf nodes
        > entries;

        //std::array<AABB<float>, Max> bounds;
        ////std::variant<NodeArrayType, LeafArrayType> children;
        //std::array<std::variant<InnerNode *, LeafNode *>, Max> children;
    };

    // "R-Trees - A Dynamic Index Structure for Spatial Searching"
    // Antonin Guttman
    //
    //  (1) Every leaf node contains between m and M index records unless it is the root
    //  (3) Every non-leaf node has between m and M children unless it is the roof
    //  (5) The root node has at least two children unless it is a leaf
    //  (6) All leaves appear on the same level
    //
    template <typename ValueType, size_t Max = 30, size_t Min = Max / 3>
    class RStarTree {
    public:
        using NodeType   = RTreeNode<ValueType, Max>;
        using NodeEntry  = typename NodeType::NodeEntry;
        using ValueEntry = typename NodeType::ValueEntry;
        using NodeArray  = typename NodeType::NodeArray;
        using ValueArray = typename NodeType::ValueArray;

        std::vector<ValueType> Search(const AABB<float> &aabb) const {
            std::vector<ValueType> matches{};
            Search(aabb, &root, matches);
            return matches;
        }

    private:
        void Search(const AABB<float> &aabb, const NodeType *node, std::vector<ValueType> &matches) const {
            if (std::holds_alternative<NodeType::NodeArray>(node->entries)) {
                const NodeArray &entries = std::get<NodeType::NodeArray>(node->entries);
                for (const NodeEntry &entry : entries) {
                    if (entry.bounds.intersects(aabb)) {
                        Search(aabb, entry.child, matches);
                    }
                }
            } else if (std::holds_alternative<NodeType::ValueArray>(node->entries)) {
                const ValueArray &entries = std::get<NodeType::ValueArray>(node->entries);
                for (const ValueEntry &entry : entries) {
                    if (entry.bounds.intersects(aabb)) {
                        matches.push_back(entry.value);
                    }
                }
            } else assert(!"Uh oh, unhandled variant type");
        }

        size_t min = Min;
        NodeType root;
    };
}