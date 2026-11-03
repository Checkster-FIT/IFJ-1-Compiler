/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    bst.h
 * @brief   Header file for RB tree implementation
 * @author  Tobiáš Mikan <xmikant00>
 */

#ifndef IFJ_1_COMPILER_BST_H
#define IFJ_1_COMPILER_BST_H

#include <stddef.h>
#include <stdlib.h>

/**
 * @brief Enum consisting of colors of the RB tree nodes
 */
typedef enum {
    BLACK = 0,
    RED = 1
} NodeColor;

/**
 * @brief Structure representing a node in a RB tree
 *
 * Each node holds pointer to the key and the value stored in the node, color of the
 * node and pointers to its left and right children.
 */
typedef struct tnode_s {
    void* key; // pointer to the key of this node
    void* value; // pointer to the value of this node
    NodeColor color; // either r or b based on color of the node

    struct tnode_s* left; // pointer to left child/subtree
    struct tnode_s* right; // pointer to right child/subtree
} TNode;

/**
 * @brief Structure representing the tree.
 */
typedef struct rbtree_s {
    TNode* root;
    int (*cmp)(void* a, void* b);
} RBTree;

/**
 * @brief Tree initialization
 *
 * @return Pointer to the new tree
 *
 * Used to start the tree
 */
RBTree* TreeInit(int (*cmp)(void*, void*));

/**
 * @brief Node creation
 *
 * @param key Key of the node
 * @param value Value of the node
 * @return Pointer to new node
 */
TNode* NodeCreate(void* key, void* value);

/**
 * @brief Checks if node is Black or Red
 *
 * @param node Node which we are checking
 * @return Color of the node
 *
 * Returns Red for Red nodes or Black for Black nodes or NULL nodes
 */
NodeColor ColorCheck(TNode* node);

/**
 * @brief Performs a left rotation on the given subtree root.
 *
 * @param node The node whose right child is promoted during the rotation.
 * @return Pointer to the new root of the rotated subtree.
 */
TNode* RotateLeft(TNode* node);

/**
 * @brief Performs a right rotation on the given subtree root.
 *
 * @param node The node whose left child is promoted during the rotation.
 * @return Pointer to the new root of the rotated subtree.
 */
TNode* RotateRight(TNode* node);

/**
 * @brief Performs a color flip on the node.
 *
 * @param node The node whose color is being changed.
 */
void FlipColor(TNode* node);

/**
 * @brief Inserts new node into tree.
 *
 * @param tree The tree being inserted into.
 * @param key Key of the data being inserted.
 * @param value Value of the data being inserted.
 *
 * @return Returns pointer to the root of the tree where the node was inserted.
 */
TNode* TreeInsert(RBTree* tree, void* key, void* value);

/**
 * @brief Recursively travel the tree and insert the node in its correct place.
 *
 * @param node The node being inserted.
 * @param key Key of the data being inserted.
 * @param value Value of the data being inserted.
 *
 * @return Pointer to the subtree root
 */
TNode* RecursiveInsert(TNode* node, void* key, void* value, int (*cmp)(void*, void*));

/**
 * @brief Recursively travel the tree and search for the node with the key.
 *
 * @param tree The tree which is being searched.
 * @param key Key of the data being searched.
 *
 * @return Node with the key, or NULL when not found
 */
TNode* Search(RBTree* tree, void* key);

/**
 * @brief String compare for this RBTree implementation.
 *
 * @param a First string to compare
 * @param b Second string to compare
 *
 * @return < 0 if first param is smaller (first alphabetically), > 0 if second param is smaller (first alphabetically) or 0 when they are equal.
 */
int RBTreeStrcmp(void* a, void* b);

#endif // IFJ_1_COMPILER_BST_H