#pragma once
#include "maths.h"
#include <cassert>
#include <array>
#include <vector>
#include <bitset>

#define RTREE_MIN_ENTRIES 2
#define RTREE_MAX_ENTRIES 8

namespace RTree {
    template<typename T, size_t ChunkSize>
    class ChunkAllocator {
    public:
        T *Alloc()
        {
            if (!head) {
                head = new Chunk{};
            }
            Chunk *chunk = head;

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
            chunk->entries[slot].metadata.chunk = chunk;
            return &chunk->entries[slot].data;
        }

        void Free(T *ptr)
        {
            ChunkEntry *entry = (ChunkEntry *)ptr;
            Chunk *findChunk = entry->metadata.chunk;

            // Find the chunk containing this entry
            Chunk *chunk = head;
            Chunk *prev = 0;
            while (chunk && chunk != findChunk) {
                prev = chunk;
                chunk = chunk->next;
            }
            assert(chunk);

            // Find the slot in the chunk containing this entry
            size_t slot = 0;
            while (slot < chunk->in_use.size() && (!chunk->in_use[slot] || &chunk->entries[slot] != entry)) {
                slot++;
            }
            assert(slot < chunk->in_use.size());
            assert(&chunk->entries[slot] == entry);

            // Zero the memory
            entry = {};

            // Clear bitset flag
            chunk->in_use.reset(slot);

            // If chunk is empty, delete chunk and update linked list
            if (chunk->in_use.none()) {
                if (prev) {
                    prev->next = chunk->next;
                } else {
                    head = chunk->next;
                }
                delete chunk;
            }
        }

    private:
        struct ChunkMetadata {
            struct Chunk *chunk;
        };
        struct ChunkEntry {
            T data;
            ChunkMetadata metadata;
        };
        struct Chunk {
            ChunkEntry entries[ChunkSize];
            std::bitset<ChunkSize> in_use;
            Chunk *next;
        };
        Chunk *head{};
    };

    // "R-Trees - A Dynamic Index Structure for Spatial Searching"
    // Antonin Guttman
    //
    //  (1) Every leaf node contains between m and M index records unless it is the root
    //  (3) Every non-leaf node has between m and M children unless it is the roof
    //  (5) The root node has at least two children unless it is a leaf
    //  (6) All leaves appear on the same level
    //
    class RTree {
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
                AABB *aabbs[RTREE_MAX_ENTRIES];
                Node *children[RTREE_MAX_ENTRIES];
                Entry *entries[RTREE_MAX_ENTRIES];
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

        RTree()
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
            if (leaf->count < RTREE_MAX_ENTRIES) {
                leaf->entries[leaf->count] = entry;
                leaf->count++;
            } else {
                newLeaf = SplitLeaf(leaf, entry);
                assert(newLeaf);
            }
            AdjustTree(leaf, newLeaf);
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

            CondenseTree(*leaf);

            // if root has one child, make child the new root
            if (root->count == 1) {
                Node *oldRoot = root;
                root = root->children[0];
                if (oldRoot->type == NodeType_Leaf) {
                    for (size_t j = 0; j < oldRoot->count; j++) {
                        entries.Free(oldRoot->entries[j]);
                    }
                }
                nodes.Free(oldRoot);
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
                    float enlargmentArea = node->children[i]->bounds.calcAreaIncrease(entry.bounds);
                    if (enlargmentArea < minEnlargementArea) {
                        minEnlargementArea = enlargmentArea;
                        bestChild = node->children[i];
                    }
                }
                node = bestChild;
            }
            return node;
        }

        // node will be split into two nodes. the first new node will overwrite node in-place; the
        // second new node will be created and a pointer to it will be returned
        Node *SplitLeaf(Node *node, Entry *entry)
        {
            assert(node->type == NodeType_Leaf);
            assert(node->count == RTREE_MAX_ENTRIES);

            Entry *remainingEntries[RTREE_MAX_ENTRIES + 1] = {};
            memcpy(remainingEntries, node->entries, sizeof(node->entries));
            remainingEntries[RTREE_MAX_ENTRIES] = entry;

            memset(node->entries, 0, sizeof(node->entries));
            node->count = 0;

            Node *nodeB = nodes.Alloc();
            nodeB->type = node->type;
            nodeB->parent = node->parent;

            {
                // PickSeeds - Find the two entries that would create the most wasted space if put into the same group
                size_t E1 = ARRAY_SIZE(remainingEntries);
                size_t E2 = ARRAY_SIZE(remainingEntries);

                float maxWastedSpace = 0.0f;
                for (size_t i = 0; i < ARRAY_SIZE(remainingEntries); i++) {
                    for (size_t j = i + 1; j < ARRAY_SIZE(remainingEntries); j++) {
                        AABB *R1 = &remainingEntries[i]->bounds;
                        AABB *R2 = &remainingEntries[j]->bounds;
                        float wastedSpace = AABB::wastedSpace(*R1, *R2);
                        if (wastedSpace > maxWastedSpace) {
                            maxWastedSpace = wastedSpace;
                            E1 = i;
                            E2 = j;
                        }
                    }
                }

                assert(E1 < ARRAY_SIZE(remainingEntries));
                assert(E2 < ARRAY_SIZE(remainingEntries));

                node->entries[node->count] = remainingEntries[E1];
                nodeB->entries[nodeB->count] = remainingEntries[E2];
                node->count++;
                nodeB->count++;
                node->bounds = remainingEntries[E1]->bounds;
                nodeB->bounds = remainingEntries[E2]->bounds;
                remainingEntries[E1] = 0;
                remainingEntries[E2] = 0;
            }

            {
                // PickNext
                size_t ENext = ARRAY_SIZE(remainingEntries);
                Node *addToNode;

                for (;;) {
                    addToNode = 0;

                    float maxIncreaseDelta = 0.0f;
                    for (size_t i = 0; i < ARRAY_SIZE(remainingEntries); i++) {
                        if (!remainingEntries[i]) {
                            continue;
                        }
                        float increase1 = node->bounds.calcAreaIncrease(remainingEntries[i]->bounds);
                        float increase2 = nodeB->bounds.calcAreaIncrease(remainingEntries[i]->bounds);
                        float increaseDelta = fabsf(increase1 - increase2);
                        if (increaseDelta > maxIncreaseDelta) {
                            maxIncreaseDelta = increaseDelta;
                            ENext = i;
                            addToNode = (increase1 <= increase2) ? node : nodeB;
                        }
                    }

                    if (!addToNode) {
                        assert(node->count + nodeB->count == ARRAY_SIZE(remainingEntries));
                        break;
                    }

                    assert(ENext < ARRAY_SIZE(remainingEntries));
                    addToNode->entries[addToNode->count] = remainingEntries[ENext];
                    addToNode->count++;
                    addToNode->bounds.growToContain(remainingEntries[ENext]->bounds);
                    remainingEntries[ENext] = 0;

                    if (node->count == (RTREE_MAX_ENTRIES - RTREE_MIN_ENTRIES + 1) ||
                        nodeB->count == (RTREE_MAX_ENTRIES - RTREE_MIN_ENTRIES + 1))
                    {
                        break;
                    }
                }

                if (node->count + nodeB->count < ARRAY_SIZE(remainingEntries)) {
                    // Add remaining entries to least occupied node to meet minimum fill requirement
                    // TODO: Smarter distribution of entries (R*-Tree recommends forced re-insertion)
                    addToNode = (node->count < nodeB->count) ? node : nodeB;
                    for (size_t i = 0; i < ARRAY_SIZE(remainingEntries); i++) {
                        if (!remainingEntries[i]) {
                            continue;
                        }
                        addToNode->entries[addToNode->count] = remainingEntries[i];
                        addToNode->count++;
                        addToNode->bounds.growToContain(remainingEntries[i]->bounds);
                        remainingEntries[i] = 0;
                    }
                }

                assert(node->count >= RTREE_MIN_ENTRIES);
                assert(nodeB->count >= RTREE_MIN_ENTRIES);
            }

            return nodeB;
        }

        Node *SplitDirectory(Node *node, Node *child)
        {
            assert(node->type == NodeType_Directory);
            assert(node->count == RTREE_MAX_ENTRIES);

            Node *remainingChildren[RTREE_MAX_ENTRIES + 1] = {};
            memcpy(remainingChildren, node->children, sizeof(node->children));
            remainingChildren[RTREE_MAX_ENTRIES] = child;

            memset(node->children, 0, sizeof(node->children));
            node->count = 0;

            Node *nodeB = nodes.Alloc();
            nodeB->type = node->type;
            nodeB->parent = node->parent;

            {
                // PickSeeds - Find the two children that would create the most wasted space if put into the same group
                size_t E1 = ARRAY_SIZE(remainingChildren);
                size_t E2 = ARRAY_SIZE(remainingChildren);

                float maxWastedSpace = 0.0f;
                for (size_t i = 0; i < ARRAY_SIZE(remainingChildren); i++) {
                    for (size_t j = i + 1; j < ARRAY_SIZE(remainingChildren); j++) {
                        AABB *R1 = &remainingChildren[i]->bounds;
                        AABB *R2 = &remainingChildren[j]->bounds;
                        float wastedSpace = AABB::wastedSpace(*R1, *R2);
                        if (wastedSpace > maxWastedSpace) {
                            maxWastedSpace = wastedSpace;
                            E1 = i;
                            E2 = j;
                        }
                    }
                }

                assert(E1 < ARRAY_SIZE(remainingChildren));
                assert(E2 < ARRAY_SIZE(remainingChildren));

                node->children[node->count] = remainingChildren[E1];
                nodeB->children[nodeB->count] = remainingChildren[E2];
                node->count++;
                nodeB->count++;
                node->bounds = remainingChildren[E1]->bounds;
                nodeB->bounds = remainingChildren[E2]->bounds;
                remainingChildren[E1] = 0;
                remainingChildren[E2] = 0;
            }

            {
                // PickNext
                size_t ENext = ARRAY_SIZE(remainingChildren);
                Node *addToNode;

                for (;;) {
                    addToNode = 0;

                    float maxIncreaseDelta = 0.0f;
                    for (size_t i = 0; i < ARRAY_SIZE(remainingChildren); i++) {
                        if (!remainingChildren[i]) {
                            continue;
                        }
                        float increase1 = node->bounds.calcAreaIncrease(remainingChildren[i]->bounds);
                        float increase2 = nodeB->bounds.calcAreaIncrease(remainingChildren[i]->bounds);
                        float increaseDelta = fabsf(increase1 - increase2);
                        if (increaseDelta > maxIncreaseDelta) {
                            maxIncreaseDelta = increaseDelta;
                            ENext = i;
                            addToNode = (increase1 <= increase2) ? node : nodeB;
                        }
                    }

                    if (!addToNode) {
                        assert(node->count + nodeB->count == ARRAY_SIZE(remainingChildren));
                        break;
                    }

                    assert(ENext < ARRAY_SIZE(remainingChildren));
                    addToNode->children[addToNode->count] = remainingChildren[ENext];
                    addToNode->count++;
                    addToNode->bounds.growToContain(remainingChildren[ENext]->bounds);
                    remainingChildren[ENext] = 0;

                    if (node->count > (RTREE_MAX_ENTRIES - RTREE_MIN_ENTRIES + 1) ||
                        nodeB->count > (RTREE_MAX_ENTRIES - RTREE_MIN_ENTRIES + 1))
                    {
                        break;
                    }
                }

                if (node->count + nodeB->count < ARRAY_SIZE(remainingChildren)) {
                    // Add remaining children to least occupied node to meet minimum fill requirement
                    // TODO: Smarter distribution of children (R*-Tree recommends forced re-insertion)
                    addToNode = (node->count < nodeB->count) ? node : nodeB;
                    for (size_t i = 0; i < ARRAY_SIZE(remainingChildren); i++) {
                        if (!remainingChildren[i]) {
                            continue;
                        }
                        addToNode->children[addToNode->count] = remainingChildren[i];
                        addToNode->count++;
                        addToNode->bounds.growToContain(remainingChildren[i]->bounds);
                        remainingChildren[i] = 0;
                    }
                }
            }

            return nodeB;
        }

        void RecalcNodeBounds(Node *node)
        {
            assert(node);
            assert(node->count);

            AABB temp = node->bounds;
            node->bounds = *node->aabbs[0];
            for (size_t i = 1; i < node->count; i++) {
                node->bounds.growToContain(*node->aabbs[i]);
            }

            assert(temp.min.x != 6.2349234f);
        }

        void AdjustTree(Node *node, Node *splitNode)
        {
            // TODO: Remove 1980 single-letter variable names (replicating paper to start)
            Node *N = node;
            Node *NN = splitNode;

            RecalcNodeBounds(N);
            if (NN) {
                RecalcNodeBounds(NN);
            }

            while (N->parent) {
                Node *P = N->parent;
                RecalcNodeBounds(P);

                if (NN) {
                    Node *PP = NN->parent;
                    Node *PPP = 0;

                    if (PP->count < RTREE_MAX_ENTRIES) {
                        PP->children[P->count] = NN;
                        PP->count++;
                    } else {
                        PPP = SplitDirectory(PP, NN);
                        RecalcNodeBounds(PPP);
                    }

                    // Recalculate split node parent's bounds
                    RecalcNodeBounds(PP);

                    NN = PPP;
                }
                N = P;
            }

            RecalcNodeBounds(N);

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
                RecalcNodeBounds(root);
            }

        }

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
            return NULL;
        }

        void CondenseTree(Node &node)
        {
            // TODO: "Re-insert orphaned entries from eliminated leaf nodes"
            UNUSED(node);
        }
    };
}