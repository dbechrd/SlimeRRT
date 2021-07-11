#pragma once
#include "maths.h"
#include <array>
#include <variant>
#include <vector>
#include <bitset>

#define RSTAR_MIN_ENTRIES 8
#define RSTAR_MAX_ENTRIES 16

namespace RStar {
    template<typename T, size_t ChunkSize>
    class ChunkAllocator {
    public:
        T *Alloc()
        {
            Chunk *chunk = &head;

            // Find first chunk with a free slot
            while (chunk && chunk->next && chunk->in_use.all()) {
                chunk = chunk->next;
            }

            // Last chunk is full, add new chunk
            if (chunk->in_use.all()) {
                chunk->next = new Chunk{};
                chunk = chunk->next;
            }

            // Find first free slot
            size_t slot = 0;
            while (slot < chunk->in_use.size() && chunk->in_use[slot]) {
                slot++;
            }

            // Set in_use bit and return pointer to slot
            chunk->in_use.set(slot);
            return &chunk->data[slot];
        }

        void Free(T *ptr)
        {
            // TODO: Determine which chunk ptr is in somehow..
            // TODO: Zero the memory
            // TODO: Clear bitset flag
            // TODO: If bitset.none(), delete chunk and update linked list
            assert(!"Fuck");
        }

    private:
        struct Chunk {
            T data[ChunkSize];
            std::bitset<ChunkSize> in_use;
            Chunk *next;
        };
        Chunk head{};
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
        enum NodeType {
            NodeType_Directory, // has children nodes
            NodeType_Leaf       // has values
        };

        // **MUST** start with AABB
        struct Entry {
            AABB bounds;
            void *udata;
        };

        //struct Node;
        //struct Entry {
        //    AABB bounds;
        //    union {
        //        Node *child;  // NodeType_Directory
        //        void *udata;  // NodeType_Leaf
        //    };
        //};

        // **MUST** start with AABB
        struct Node {
            AABB bounds;
            NodeType type;
            size_t count;
            union {
                AABB *aabbs[RSTAR_MAX_ENTRIES];
                Node *children[RSTAR_MAX_ENTRIES];
                Entry *entries[RSTAR_MAX_ENTRIES];
            };
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
            root = nodes.Alloc();
            root->type = NodeType_Leaf;
        }
        void Search(const AABB &aabb, std::vector<void *> &matches, CompareMode compareMode) const
        {
            SearchNode(*root, aabb, matches, compareMode);
        }
        void Insert(const AABB &aabb, void *udata)
        {
            Entry *entry = entries.Alloc();
            entry->bounds = aabb;
            entry->udata = udata;

            Node *leaf = ChooseLeaf(*entry);
            Node *newLeaf = 0;
            if (leaf->count < RSTAR_MAX_ENTRIES) {
                leaf->entries[leaf->count] = entry;
                leaf->count++;
            } else {
                newLeaf = SplitLeaf(leaf, *entry);
                assert(newLeaf);
            }
            AdjustTree(leaf, newLeaf);
            // if root split, create new root
        }
        bool Delete(const AABB &aabb, void *udata)
        {
            // TODO: Could return entry index as well.. perhaps a pointless optimization?
            Node *leaf = FindLeaf(*root, aabb, udata);
            if (!leaf) {
                return false;  // Entry not found
            }

            assert(leaf->type == NodeType_Leaf);
            assert(leaf->count > 0);

            size_t i = 0;

            // Skip over entries until we find the one to delete
            while (i < leaf->count && leaf->entries[i]->udata != udata) {
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
                root = root->children[0];
                // TODO: [!!! FIX MEMORY LEAK !!!] nodes.Free(oldRoot);
            }

            return true;
        }

        Node *root;
    private:
        ChunkAllocator<Node, 16> nodes;
        ChunkAllocator<Entry, 16> entries;

        void SearchNode(const Node &node, const AABB &aabb, std::vector<void *> &matches, CompareMode compareMode) const
        {
            for (size_t i = 0; i < node.count; i++) {
                bool passCompare = false;
                switch (compareMode) {
                    case CompareMode_Overlap: {
                        passCompare = node.aabbs[i]->intersects(aabb);
                        break;
                    }
                }
                if (passCompare) {
                    switch (node.type) {
                        case NodeType_Directory:
                            SearchNode(*node.children[i], aabb, matches, compareMode);
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
                        bestChild = node->children[i];
                    }
                }
                node = bestChild;
            }
            return node;
        }

        // node will be split into two nodes. the first new node will overwrite node in-place; the
        // second new node will be created and a pointer to it will be returned
        Node *SplitLeaf(Node *node, Entry &entry)
        {
            assert(node->count == RSTAR_MAX_ENTRIES);

            Node &newNode = *nodes.Alloc();
            newNode.type = node->type;
            newNode.parent = node->parent;

            float avg_x = 0.0f;
            for (size_t i = 0; i < node->count; i++) {
                avg_x += node->aabbs[i]->min.x;
            }
            avg_x += entry.bounds.min.x;
            avg_x /= node->count + 1.0f;

#if 0
            // TODO: Implement an actual split algorithm. Currently, we are just moving the second
            // half of the entries to a new node arbitrarily.
            size_t half = node->count / 2;
            for (size_t i = 0; i < half; i++) {
                newNode.entries[i] = node->entries[half + i];
                node->entries[half + i] = {};
            }
            newNode.count = half;
            node->count = half;
#else
            size_t entry_count = node->count;
            node->count = 0;
            for (size_t i = 0; i < entry_count; i++) {
                if (node->aabbs[i]->min.x < avg_x) {
                    node->entries[node->count] = node->entries[i];
                    if (i > node->count) node->entries[i] = {};
                    node->count++;
                } else {
                    newNode.entries[newNode.count] = node->entries[i];
                    node->entries[i] = {};
                    newNode.count++;
                }
            }
#endif

#if 0
            // TODO: Put the new entry somewhere intelligent as well. For now, just add to first node's
            // entry list arbitrarily.
            node->entries[node->count] = &entry;
            node->count++;
#else
            if (entry.bounds.min.x < avg_x && node->count < RSTAR_MAX_ENTRIES) {
                node->entries[node->count] = &entry;
                node->count++;
            } else {
                newNode.entries[newNode.count] = &entry;
                newNode.count++;
            }
#endif

            return &newNode;
        }

        Node *SplitDirectory(Node *node, Node *child)
        {
            assert(node->count == RSTAR_MAX_ENTRIES);

            Node &newNode = *nodes.Alloc();
            newNode.type = node->type;
            newNode.parent = node->parent;

            // TODO: Implement an actual split algorithm. Currently, we are just moving the second
            // half of the entries to a new node arbitrarily.
            size_t half = node->count / 2;
            for (size_t i = 0; i < half; i++) {
                newNode.children[i] = node->children[half + i];
                node->children[half + i] = {};
            }
            newNode.count = half;
            node->count = half;

            // TODO: Put the new entry somewhere intelligent as well. For now, just add to first node's
            // entry list arbitrarily.
            node->children[node->count] = child;
            node->count++;

            return &newNode;
        }

        void AdjustTree(Node *node, Node *splitNode)
        {
            // TODO: Remove 1980 single-letter variable names (replicating paper to start)
            Node *N = node;
            Node *NN = splitNode;

            // Recalculate bounds
            N->bounds = *N->aabbs[0];
            for (size_t i = 1; i < N->count; i++) {
                N->bounds.growToContain(*N->aabbs[i]);
            }

            if (NN) {
                NN->bounds = *NN->aabbs[0];
                for (size_t i = 1; i < NN->count; i++) {
                    NN->bounds.growToContain(*NN->aabbs[i]);
                }
            }

            while (N->parent) {
                Node *P = N->parent;

                // Recalculate parent bounds
                P->bounds = *N->aabbs[0];
                for (size_t i = 1; i < N->count; i++) {
                    P->bounds.growToContain(*N->aabbs[i]);
                }

                if (NN) {
                    Node *PP = NN->parent;

                    bool split = false;
                    if (PP->count < RSTAR_MAX_ENTRIES) {
                        PP->children[P->count] = NN;
                        PP->count++;
                    } else {
                        PP = SplitDirectory(PP, NN);
                        split = true;
                    }

                    // Recalculate split node parent's bounds
                    PP->bounds = *NN->aabbs[0];
                    for (size_t i = 1; i < NN->count; i++) {
                        PP->bounds.growToContain(*NN->aabbs[i]);
                    }

                    NN = split ? PP : 0;
                }
                N = P;
            }

            // We're at root now, if there's an NN, then the root node was split. Make new root.
            if (NN) {
                assert(N && !N->parent);
                assert(NN && !NN->parent);
                Node *newRoot = nodes.Alloc();
                newRoot->type = NodeType_Directory;
                newRoot->count = 2;
                newRoot->children[0] = N;
                newRoot->children[0]->parent = newRoot;
                newRoot->children[1] = NN;
                newRoot->children[1]->parent = newRoot;
                root = newRoot;
            }
        }

        // TODO: Rename to FindEntry
        Node *FindLeaf(Node &node, const AABB &aabb, void *udata)
        {
            switch (node.type) {
                case NodeType_Directory: {
                    for (size_t i = 0; i < node.count; i++) {
                        if (aabb.intersects(node.entries[i]->bounds)) {
                            Node *result = FindLeaf(*node.children[i], aabb, udata);
                            if (result) {
                                return result;
                            }
                        }
                    }
                    break;
                }
                case NodeType_Leaf: {
                    for (size_t i = 0; i < node.count; i++) {
                        if (node.entries[i]->udata == udata) {
                            return &node;
                        }
                    }
                    break;
                }
            }
        }

        void CondenseTree(Node &node)
        {
            // TODO: "Re-insert orphaned entries from eliminated leaf nodes"
        }
    };
}