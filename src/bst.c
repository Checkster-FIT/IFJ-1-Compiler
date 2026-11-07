/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    bst.c
 * @brief   Red-Black tree abstract data type implementation
 * @author  Tobiáš Mikan <xmikant00>
 */

#include "bst.h"
#include <string.h>

RBTree* tree_init(int (*cmp)(const void* a, const void* b))
{
    RBTree* tree = (RBTree*)malloc(sizeof(RBTree));
    if (tree == NULL) {
        return NULL;
    }
    tree->root = NULL;
    tree->cmp = cmp;
    return tree;
}

TNode* node_create(const void* key, void* value)
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

NodeColor color_check(TNode* node)
{
    if (node == NULL || node->color == BLACK) {
        return BLACK;
    }
    return RED;
}

TNode* rotate_left(TNode* node)
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

TNode* rotate_right(TNode* node)
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

void flip_color(TNode* node)
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

TNode* tree_insert(RBTree* tree, const void* key, void* value)
{
    if (tree == NULL) {
        return NULL;
    }
    TNode* null_check = recursive_insert(tree->root, key, value, tree->cmp);
    if (null_check == NULL) {
        return NULL;
    }
    tree->root = null_check;

    tree->root->color = BLACK;
    return tree->root;
}

TNode* recursive_insert(TNode* node, const void* key, void* value, int (*cmp)(const void*, const void*))
{
    if (node == NULL) {
        node = node_create(key, value);
        return node;
    }

    const int cmp_outcome = cmp(key, node->key);

    if (cmp_outcome < 0) {
        node->left = recursive_insert(node->left, key, value, cmp);

    } else if (cmp_outcome > 0) {
        node->right = recursive_insert(node->right, key, value, cmp);
    } else {
        node->value = value;
        return node;
    }

    if (color_check(node->right) == RED && color_check(node->left) == BLACK) {
        node = rotate_left(node);
    }
    if (color_check(node->left) == RED && color_check(node->left->left) == RED) {
        node = rotate_right(node);
    }
    if (color_check(node->left) == RED && color_check(node->right) == RED) {
        flip_color(node);
    }
    return node;
}

TNode* search(const RBTree* tree, const void* key)
{
    if (tree == NULL || tree->root == NULL) {
        return NULL;
    }

    TNode* node = tree->root;

    while (node != NULL) {
        const int comp_value = tree->cmp(key, node->key);

        if (comp_value < 0)
            node = node->left;
        else if (comp_value > 0)
            node = node->right;
        else
            break;
    }
    return node;
}

int rb_tree_strcmp(const void* a, const void* b)
{
    return strcmp((char*)a, (char*)b);
}

void tree_delete(TNode* node)
{
    if (node != NULL) {
        tree_delete(node->left);
        tree_delete(node->right);
        free(node);
    }
}