#pragma once
#include "maths.h"
#include "raylib/raylib.h"
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
            *entry = {};
            assert(entry->metadata.chunk == 0);

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
            struct Chunk *chunk{};
        };
        struct ChunkEntry {
            T data{};
            ChunkMetadata metadata{};
        };
        struct Chunk {
            ChunkEntry entries[ChunkSize]{};
            std::bitset<ChunkSize> in_use{};
            Chunk *next{};
        };
        Chunk *head{};
    };

    enum class NodeType {
        Directory, // has children nodes
        Leaf       // has values
    };

    // Note: We could also support additional search modes:
    // - CompareContained     // return entries contained by the search AABB
    // - CompareContains      // return entries which contain the search AABB
    // As well as range deletion (delete all nodes which meet the comparison constraint)
    enum class CompareMode {
        Overlap
    };

    // "R-Trees - A Dynamic Index Structure for Spatial Searching"
    // Antonin Guttman
    //
    //  (1) Every leaf node contains between m and M index records unless it is the root
    //  (3) Every non-leaf node has between m and M children unless it is the roof
    //  (5) The root node has at least two children unless it is a leaf
    //  (6) All leaves appear on the same level
    //
    template<typename T>
    class RTree {
    public:
        // **MUST** start with AABB
        template<typename T>
        struct Entry {
            AABB bounds {};
            T    udata  {};
        };

        //struct Node;
        //struct Entry {
        //    AABB bounds;
        //    union {
        //        Node *child;  // NodeType::Directory
        //        void *udata;  // NodeType::Leaf
        //    };
        //};

        // **MUST** start with AABB
        template<typename T>
        struct Node {
            AABB     bounds {};
            NodeType type   {};
            size_t   count  {};
            union {
                AABB *aabbs[RTREE_MAX_ENTRIES];
                Node *children[RTREE_MAX_ENTRIES];
                Entry<T> *entries[RTREE_MAX_ENTRIES];
            };
            Node<T> *parent{};
        };

        RTree()
        {
            root = nodes.Alloc();
            root->type = NodeType::Leaf;
        }

        template<typename T>
        void Search(const AABB &aabb, std::vector<T> &matches, CompareMode compareMode) const
        {
            SearchNode(*root, aabb, matches, compareMode);
        }

        template<typename T>
        void Insert(const AABB &aabb, T udata)
        {
            Entry<T> *entry = entries.Alloc();
            entry->bounds = aabb;
            entry->udata = udata;

            Node<T> *leaf = ChooseLeaf(*entry);
            Node<T> *newLeaf = 0;
            if (leaf->count < RTREE_MAX_ENTRIES) {
                leaf->entries[leaf->count] = entry;
                leaf->count++;
            } else {
                newLeaf = SplitLeaf(leaf, entry);
                assert(newLeaf);
            }
            AdjustTree(leaf, newLeaf);
        }

        template<typename T>
        bool Delete(const AABB &aabb, T udata)
        {
            // TODO: Could return entry index as well.. perhaps a pointless optimization?
            Node<T> *leaf = FindLeaf(*root, aabb, udata);
            if (!leaf) {
                return false;  // Entry not found
            }

            assert(leaf->type == NodeType::Leaf);
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
            *leaf->entries[leaf->count] = {};
            //memset(leaf->entries[leaf->count], 0, sizeof(*leaf->entries));
            assert(leaf->entries[leaf->count]->udata == 0);

            CondenseTree(*leaf);

            // if root has one child, make child the new root
            if (root->count == 1) {
                Node<T> *oldRoot = root;
                root = root->children[0];
                if (oldRoot->type == NodeType::Leaf) {
                    for (size_t j = 0; j < oldRoot->count; j++) {
                        entries.Free(oldRoot->entries[j]);
                    }
                }
                nodes.Free(oldRoot);
            }

            return true;
        }

        void Draw() const {
            DrawNode(*root, 0);
        }

        Node<T> *root{};
    private:
        ChunkAllocator<Node<T>, 16>  nodes   {};
        ChunkAllocator<Entry<T>, 16> entries {};

        template<typename T>
        void SearchNode(const Node<T> &node, const AABB &aabb, std::vector<T> &matches, CompareMode compareMode) const
        {
            for (size_t i = 0; i < node.count; i++) {
                bool passCompare = false;
                switch (compareMode) {
                    case CompareMode::Overlap: {
                        passCompare = node.aabbs[i]->intersects(aabb);
                        break;
                    }
                }
                if (passCompare) {
                    switch (node.type) {
                        case NodeType::Directory:
                            SearchNode(*node.children[i], aabb, matches, compareMode);
                            break;
                        case NodeType::Leaf:
                            matches.push_back(node.entries[i]->udata);
                            break;
                        default:
                            assert(!"Unexpected variant itemClass");
                            break;
                    }
                }
            }
        }

        Node<T> *ChooseLeaf(const Entry<T> &entry)
        {
            assert(root);
            Node<T> *node = root;
            while (node->type == NodeType::Directory) {
                // Find child whose bounds need least enlargement to include aabb
                Node<T> *bestChild = 0;
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
        Node<T> *SplitLeaf(Node<T> *node, Entry<T> *entry)
        {
            assert(node->type == NodeType::Leaf);
            assert(node->count == RTREE_MAX_ENTRIES);

            Entry<T> *remainingEntries[RTREE_MAX_ENTRIES + 1] = {};
            memcpy(remainingEntries, node->entries, sizeof(node->entries));
            remainingEntries[RTREE_MAX_ENTRIES] = entry;

            memset(node->entries, 0, sizeof(node->entries));
            node->count = 0;

            Node<T> *nodeB = nodes.Alloc();
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
                Node<T> *addToNode;

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

        Node<T> *SplitDirectory(Node<T> *node, Node<T> *child)
        {
            assert(node->type == NodeType::Directory);
            assert(node->count == RTREE_MAX_ENTRIES);

            Node<T> *remainingChildren[RTREE_MAX_ENTRIES + 1] = {};
            memcpy(remainingChildren, node->children, sizeof(node->children));
            remainingChildren[RTREE_MAX_ENTRIES] = child;

            memset(node->children, 0, sizeof(node->children));
            node->count = 0;

            Node<T> *nodeB = nodes.Alloc();
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
                Node<T> *addToNode;

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

        void RecalcNodeBounds(Node<T> *node)
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

        void AdjustTree(Node<T> *node, Node<T> *splitNode)
        {
            // TODO: Remove 1980 single-letter variable names (replicating paper to start)
            Node<T> *N = node;
            Node<T> *NN = splitNode;

            RecalcNodeBounds(N);
            if (NN) {
                RecalcNodeBounds(NN);
            }

            while (N->parent) {
                Node<T> *P = N->parent;
                RecalcNodeBounds(P);

                if (NN) {
                    Node<T> *PP = NN->parent;
                    Node<T> *PPP = 0;

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
                Node<T> *newRoot = nodes.Alloc();
                newRoot->type = NodeType::Directory;
                newRoot->count = 2;
                newRoot->children[0] = N;
                newRoot->children[0]->parent = newRoot;
                newRoot->children[1] = NN;
                newRoot->children[1]->parent = newRoot;
                root = newRoot;
                RecalcNodeBounds(root);
            }

        }

        template<typename T>
        Node<T> *FindLeaf(Node<T> &node, const AABB &aabb, T udata)
        {
            switch (node.type) {
                case NodeType::Directory: {
                    for (size_t i = 0; i < node.count; i++) {
                        if (aabb.intersects(node.entries[i]->bounds)) {
                            Node<T> *result = FindLeaf(*node.children[i], aabb, udata);
                            if (result) {
                                return result;
                            }
                        }
                    }
                    break;
                }
                case NodeType::Leaf: {
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

        void CondenseTree(Node<T> &node)
        {
            // TODO: "Re-insert orphaned entries from eliminated leaf nodes"
            UNUSED(node);
        }

        void DrawNode(const Node<T> &node, int level) const {
            thread_local Color colors[] = {
                Color{ 255,   0,   0, 255 },
                Color{   0, 255,   0, 255 },
                Color{   0,   0, 255, 255 },
                PINK,
                LIME,
                SKYBLUE
            };
            assert(level < ARRAY_SIZE(colors));
            switch (node.type) {
                case NodeType::Directory: {
                    for (size_t i = 0; i < node.count; i++) {
                        DrawNode(*node.children[i], level + 1);
                    }
                    break;
                }
                case NodeType::Leaf: {
                    for (size_t i = 0; i < node.count; i++) {
                        Entry<T> *entry = node.entries[i];
                        DrawRectangleLinesEx(entry->bounds.toRect(), level + 4, WHITE);
                    }
                    break;
                }
            }
            DrawRectangleLinesEx(node.bounds.toRect(), level + 3, colors[level]);
        }
    };
}