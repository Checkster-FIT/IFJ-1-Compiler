/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    bst.c
 * @brief
 * @author
 */

#include "bst.h"

RBTree* TreeInit(int(*cmp)(void* a, void* b))
{
    RBTree* tree = (RBTree*)malloc(sizeof(RBTree));
    if (tree == NULL) {
        return NULL;
    }
    tree->root = NULL;
    tree->cmp = cmp;
    return tree;
}

TNode* NodeCreate(void* key, void* value)
{
    TNode* node = malloc(sizeof(TNode));
    if (node == NULL) {
        return NULL;
    }

    node->key = key;
    node->value = value;
    node->color = RED;

    node->left = NULL;
    node->right = NULL;

    return node;
}

NodeColor ColorCheck(TNode* node)
{
    if (node->color == RED) {
        return RED;
    }
    return BLACK;
}

TNode* RotateLeft(TNode* node)
{
    TNode* moving_node = node->right;
    node->right = moving_node->left;
    TNode* parent = node;

    node = moving_node;
    node->color = parent->color;

    node->left = parent;
    parent->color = RED;
    return node;
}

TNode* RotateRight(TNode* node)
{
    TNode* moving_node = node->left;
    node->left = moving_node->right;
    TNode* parent = node;

    node = moving_node;
    node->color = parent->color;

    node->right = parent;
    parent->color = RED;
    return node;
}

void* FlipColor(TNode* node)
{
    if (node->left->color == RED && node->right->color == RED) {
        node->color = RED;
        node->left->color = BLACK;
        node->right->color = BLACK;
    }
}

void* TreeInsert(RBTree* tree, void* key, void* value)
{
    if (tree == NULL) {
        return NULL;
    }
    tree->root = RecursiveInsert(tree->root, key, value, tree->cmp);
}

TNode* RecursiveInsert(TNode* node, void* key, void* value, int(*cmp)(void*, void*))
{
    if (node == NULL) {
        node = NodeCreate(key, value);
        return node;
    }
    int cmp_outcome = cmp(key, node->key);

    if (cmp_outcome < 0) {
        node->left = RecursiveInsert(node->left, key, value, cmp);

    } else if (cmp_outcome > 0) {
        node->right = RecursiveInsert(node->right, key, value, cmp);
    } else {
        node->value = value;
        return node; // DUPLICATE KEY WHAT TO DO???
    }

    if (ColorCheck(node->right) == RED && ColorCheck(node->left) == BLACK) {
        node = RotateLeft(node);
    }
    if (ColorCheck(node->left) == RED && ColorCheck(node->left->left) == RED) {
        node = RotateRight(node);
    }
    if (ColorCheck(node->left) == RED && ColorCheck(node->right) == RED) {
        FlipColor(node);
    }
    return node;
}