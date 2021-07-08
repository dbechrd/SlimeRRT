#pragma once
#include "maths.h"
#include <array>
#include <variant>
#include <vector>

#define RSTAR_MIN_ENTRIES 8
#define RSTAR_MAX_ENTRIES 16

namespace RStar {
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
        enum NodeType {
            NodeType_Directory, // has children nodes
            NodeType_Leaf       // has values
        };

        struct Node;
        struct Entry {
            AABB bounds;
            union {
                Node *child;  // NodeType_Directory
                void *udata;  // NodeType_Leaf
            };
        };

        struct Node {
            NodeType type;
            size_t count;
            Entry *entries[RSTAR_MAX_ENTRIES];
            Node *parent;
        };

        // Note: We could also support additional search modes:
        // - CompareContained     // return entries containted by the search AABB
        // - CompareContains      // return entries which contain the search AABB
        // As well as range deletion (delete all nodes which meet the comparison constraint)
        enum CompareMode {
            CompareMode_Overlap
        };

        RStarTree()
        {
            nodes.push_back({});
            root = &nodes.front();
            root->type = NodeType_Leaf;
        }
        void Search(const AABB &aabb, std::vector<void *> &matches, CompareMode compareMode) const
        {
            SearchNode(*root, aabb, matches, compareMode);
        }
        void Insert(Entry &entry)
        {
            Node *leaf = ChooseLeaf(entry);
            Node *splitLeaf = 0;
            if (leaf->count < RSTAR_MAX_ENTRIES) {
                // TODO: Find free entry (udata == 0) and only push_back if none found
                entries.push_back(entry);
                leaf->entries[leaf->count] = &entries.back();
                leaf->count++;
            } else {
                splitLeaf = SplitNode(leaf, entry);
                assert(splitLeaf);
            }
            AdjustTree(leaf, splitLeaf);
            // if root split, create new root
        }
        bool Delete(const Entry &entry)
        {
            // TODO: Could return entry index as well.. perhaps a pointless optimization?
            Node *leaf = FindLeaf(*root, entry);
            if (!leaf) {
                return false;  // Entry not found
            }

            assert(leaf->type == NodeType_Leaf);
            assert(leaf->count > 0);

            size_t i = 0;

            // Skip over entries until we find the one to delete
            while (i < leaf->count && leaf->entries[i]->udata != entry.udata) {
                i++;
            }

            // If entry to delete isn't at end, shift remaining entries to the left
            while (i < leaf->count - 1) {
                leaf->entries[i] = leaf->entries[i+1];
            }
            leaf->count--;

            // Zero last entry which is no longer in use
            leaf->entries[leaf->count] = {};
            //memset(leaf->entries[leaf->count], 0, sizeof(*leaf->entries));
            assert(leaf->entries[leaf->count]->udata == 0);

            CondenseTree(*root);

            // if root has one child, make child the new root
            if (root->count == 1) {
                Node *oldRoot = root;

                root = root->entries[0]->child;

                oldRoot = {};
                assert(oldRoot->entries[0]->udata == 0);
                //oldRoot->nodes[0] = {};
                //oldRoot->count = 0;
            }

            return true;
        }

    private:
        std::vector<Node> nodes;
        std::vector<Entry> entries;
        Node *root;

        void SearchNode(const Node &node, const AABB &aabb, std::vector<void *> &matches, CompareMode compareMode) const
        {
            for (size_t i = 0; i < node.count; i++) {
                bool passCompare = false;
                switch (compareMode) {
                    case CompareMode_Overlap: {
                        passCompare = node.entries[i]->bounds.intersects(aabb);
                        break;
                    }
                }
                if (passCompare) {
                    switch (node.type) {
                        case NodeType_Directory:
                            SearchNode(*node.entries[i]->child, aabb, matches, compareMode);
                            break;
                        case NodeType_Leaf:
                            matches.push_back(node.entries[i]->udata);
                            break;
                        default:
                            assert(!"Unexpected variant type");
                            break;
                    }
                }
            }
        }

        Node *ChooseLeaf(const Entry &entry)
        {
            Node *node = root;
            while (node->type == NodeType_Directory) {
                // Find child whose bounds need least enlargement to include aabb
                Node *bestChild = 0;
                float minEnlargementArea = FLT_MAX;
                for (size_t i = 0; i < node->count; i++) {
                    // TODO: Calculate enlargement and break ties
                    float enlargmentArea = 1.0f;
                    if (bestChild == nullptr || enlargmentArea < minEnlargementArea) {
                        bestChild = node->entries[i]->child;
                    }
                }
                node = bestChild;
            }
            return node;
        }

        // node will be split into two nodes. the first new node will overwrite node in-place; the
        // second new node will be created and a pointer to it will be returned
        Node *SplitNode(Node *node, Entry &entry)
        {
            assert(node->count == RSTAR_MAX_ENTRIES);

            nodes.push_back({});
            Node &splitNode = nodes.back();
            splitNode.type = node->type;
            splitNode.parent = node->parent;

            // TODO: Implement an actual split algorithm. Currently, we are just moving the second
            // half of the entries to a new node arbitrarily.
            size_t half = node->count / 2;
            for (size_t i = half; i < node->count; i++) {
                splitNode.entries[i] = node->entries[i];
                node->entries[i] = {};
            }
            splitNode.count = half;
            node->count = half;

            // TODO: Put the new entry someone intelligent as well. For now, just add to first node's
            // entry list aribtrarily.
            node->entries[node->count] = &entry;
            node->count++;

            return &splitNode;
        }

        void AdjustTree(Node *node, Node *splitNode)
        {
            // TODO: Remove 1980 single-letter variable names (replicating paper to start)
            Node *N = node;
            Node *NN = splitNode;

            while (N->parent) {
                Node *P = N->parent;
                Node *PP = 0;

                Entry *EN = 0;
                for (size_t i = 0; i < P->count; i++) {
                    if (P->entries[i]->child == N) {
                        EN = P->entries[i];
                        break;
                    }
                }
                assert(EN);

                EN->bounds = N->entries[0]->bounds;
                // TODO: AABB.growToContain(other)
                for (size_t i = 1; i < N->count; i++) {
                    EN->bounds.min.x = MIN(EN->bounds.min.x, N->entries[i]->bounds.min.x);
                    EN->bounds.min.y = MIN(EN->bounds.min.y, N->entries[i]->bounds.min.y);
                    EN->bounds.max.x = MAX(EN->bounds.max.x, N->entries[i]->bounds.max.x);
                    EN->bounds.max.y = MAX(EN->bounds.max.y, N->entries[i]->bounds.max.y);
                }

                if (NN) {
                    entries.push_back({});
                    Entry &ENN = entries.back();
                    ENN.child = NN;
                    ENN.bounds = NN->entries[0]->bounds;
                    // TODO: AABB.growToContain(other)
                    for (size_t i = 1; i < N->count; i++) {
                        ENN.bounds.min.x = MIN(ENN.bounds.min.x, NN->entries[i]->bounds.min.x);
                        ENN.bounds.min.y = MIN(ENN.bounds.min.y, NN->entries[i]->bounds.min.y);
                        ENN.bounds.max.x = MAX(ENN.bounds.max.x, NN->entries[i]->bounds.max.x);
                        ENN.bounds.max.y = MAX(ENN.bounds.max.y, NN->entries[i]->bounds.max.y);
                    }

                    if (P->count < RSTAR_MAX_ENTRIES) {
                        P->entries[P->count] = &ENN;
                        P->count++;
                    } else {
                        PP = SplitNode(P, ENN);
                    }
                }

                // TODO: Adjust covering rectangle of N.. wut?

                N = P;
                NN = PP;
            }

            // TODO: "Re-insert orphaned entries from eliminated leaf nodes, higher in the tree..." wut?
        }

        // TODO: Rename to FindEntry
        Node *FindLeaf(Node &node, const Entry &entry)
        {
            switch (node.type) {
                case NodeType_Directory: {
                    for (size_t i = 0; i < node.count; i++) {
                        if (entry.bounds.intersects(node.entries[i]->bounds)) {
                            Node *result = FindLeaf(*node.entries[i]->child, entry);
                            if (result) {
                                return result;
                            }
                        }
                    }
                    break;
                }
                case NodeType_Leaf: {
                    for (size_t i = 0; i < node.count; i++) {
                        if (node.entries[i]->udata == entry.udata) {
                            return &node;
                        }
                    }
                    break;
                }
            }
        }

        void CondenseTree(Node &node)
        {

        }
    };
}