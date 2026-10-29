/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    bst.c
 * @brief   RBTree implementation
 * @author  Tobiáš Mikan <xmikant00>
 */

#include "bst.h"

#include <string.h>

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
    if (node == NULL || node->color == BLACK) {
        return BLACK;
    }
    return RED;
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

void FlipColor(TNode* node)
{
    if (node->left == NULL || node->right == NULL) {
        return;
    }
    if (node->left->color == RED && node->right->color == RED) {
        node->color = RED;
        node->left->color = BLACK;
        node->right->color = BLACK;
    }
}

TNode* TreeInsert(RBTree* tree, void* key, void* value)
{
    if (tree == NULL) {
        return NULL;
    }
    tree->root = RecursiveInsert(tree->root, key, value, tree->cmp);
    if (tree->root != NULL) {
        tree->root->color = BLACK;
    }
    return tree->root;
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
        return node;
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

TNode* Search(RBTree* tree, void* key)
{
    if (tree == NULL || tree->root == NULL) {
        return NULL;
    }

    TNode* node = tree->root;

    while (node != NULL) {
        int comp_value = tree->cmp(key, node->key);

        if (comp_value < 0) node = node->left;
        else if (comp_value > 0) node = node->right;
        else break;
    }
    return node;
}

int RBTreeStrcmp(void* a, void* b)
{
    return strcmp((char*)a, (char*)b);
}