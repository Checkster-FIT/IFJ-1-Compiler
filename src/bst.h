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
    struct tnode_s* right; // pointer to left child/subtree
} TNode;

/**
 * @brief Tree initialization
 *
 * @return Null pointer
 *
 * Used to start the tree
 */
TNode* TreeInit();

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
void* FlipColor(TNode* node);

#endif // IFJ_1_COMPILER_BST_H