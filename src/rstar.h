#pragma once
#include "maths.h"
#include <array>
#include <variant>
#include <vector>

#define RSTAR_MIN_ENTRIES 8
#define RSTAR_MAX_ENTRIES 16

namespace RStar {

    struct RTreeNode {
        enum {
            RTreeNodeInner,  // has children nodes
            RTreeNodeLeaf    // has values
        } type;

        struct {
            void *data;  // Pointer to child node, or user data if leaf node
            AABB bounds;
        } entries[RSTAR_MAX_ENTRIES];

        size_t entryCount;
    };

    // "R-Trees - A Dynamic Index Structure for Spatial Searching"
    // Antonin Guttman
    //
    //  (1) Every leaf node contains between m and M index records unless it is the root
    //  (3) Every non-leaf node has between m and M children unless it is the roof
    //  (5) The root node has at least two children unless it is a leaf
    //  (6) All leaves appear on the same level
    //
    class RStarTree {
    public:
        void Search(const AABB &aabb, std::vector<void *> &matches) const
        {
            SearchNode(aabb, root, matches);
        }
        void Insert(const AABB &aabb, void *value)
        {
            RTreeNode *leaf = ChooseLeaf(aabb);
            RTreeNode *splitLeaf = 0;
            if (leaf->entryCount == RSTAR_MAX_ENTRIES) {
                splitLeaf = SplitNode(leaf);
                assert(splitLeaf);
            }
            AdjustTree(leaf, splitLeaf);
            // if root split, create new root
        }

    private:
        std::vector<RTreeNode> nodes;
        RTreeNode *root;

        void SearchNode(const AABB &aabb, const RTreeNode *node, std::vector<void *> &matches) const
        {
            switch (node->type) {
                case RTreeNode::RTreeNodeInner:
                    for (size_t i = 0; i < node->entryCount; i++) {
                        if (node->entries[i].bounds.intersects(aabb)) {
                            SearchNode(aabb, (RTreeNode *)node->entries[i].data, matches);
                        }
                    }
                    break;
                case RTreeNode::RTreeNodeLeaf:
                    for (size_t i = 0; i < node->entryCount; i++) {
                        if (node->entries[i].bounds.intersects(aabb)) {
                            matches.push_back(node->entries[i].data);
                        }
                    }
                    break;
                default:
                    assert(!"Unexpected variant type");
                    break;
            }
        }

        RTreeNode *ChooseLeaf(const AABB &aabb)
        {
            RTreeNode *node = root;
            while (node->type == RTreeNode::RTreeNodeInner) {
                // Find child whose bounds need least enlargement to include aabb
                RTreeNode *bestChild = 0;
                float minEnlargementArea = FLT_MAX;
                for (size_t i = 0; i < node->entryCount; i++) {
                    // TODO: Calculate enlargement and break ties
                    float enlargmentArea = 1.0f;
                    if (bestChild == nullptr || enlargmentArea < minEnlargementArea) {
                        bestChild = (RTreeNode *)node->entries->data;
                    }
                }
                node = bestChild;
            }
            return node;
        }

        // node will be split into two nodes. the first new node will overwrite node in-place; the
        // second new node will be created and a pointer to it will be returned
        RTreeNode *SplitNode(const RTreeNode *node)
        {
            assert(node->entryCount == RSTAR_MAX_ENTRIES);
            return 0;
        }

        void AdjustTree(const RTreeNode *node, const RTreeNode *splitNode)
        {

        }
    };
}