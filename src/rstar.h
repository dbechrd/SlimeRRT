#pragma once
#include "maths.h"
#include <array>
#include <variant>
#include <vector>

namespace RStar {
    template <typename T>
    struct RTreeNode;

    template <typename T>
    struct RTreeNodeEntry {
        AABB bounds;
        union {
            RTreeNode<T> *child;  // Inner nodes
            T value;              // Leaf nodes
        };
    };

    template <typename T>
    struct RTreeNode {
        enum {
            RTreeNodeInner,  // has children nodes
            RTreeNodeLeaf    // has values
        } type;

        std::vector<RTreeNodeEntry<T>> entries;
    };

    // "R-Trees - A Dynamic Index Structure for Spatial Searching"
    // Antonin Guttman
    //
    //  (1) Every leaf node contains between m and M index records unless it is the root
    //  (3) Every non-leaf node has between m and M children unless it is the roof
    //  (5) The root node has at least two children unless it is a leaf
    //  (6) All leaves appear on the same level
    //
    template <typename T>
    class RStarTree {
    public:
        using Node = RTreeNode<T>;

        void Search(const AABB &aabb, std::vector<T> &matches) const {
            SearchNode(aabb, &root, matches);
        }

    private:
        Node root;

        void SearchNode(const AABB &aabb, const Node *node, std::vector<T> &matches) const {
            switch (node->type) {
                case RTreeNode<T>::RTreeNodeInner:
                    for (const auto &entry : node->entries) {
                        if (entry.bounds.intersects(aabb)) {
                            SearchNode(aabb, entry.child, matches);
                        }
                    }
                    break;
                case RTreeNode<T>::RTreeNodeLeaf:
                    for (const auto &entry : node->entries) {
                        if (entry.bounds.intersects(aabb)) {
                            matches.push_back(entry.value);
                        }
                    }
                    break;
                default:
                    assert(!"Uh oh, unhandled variant type");
                    break;
            }
        }
    };
}