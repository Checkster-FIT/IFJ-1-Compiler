/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    bst.c
 * @brief
 * @author
 */

#include "bst.h"

TNode* TreeInit()
{
    TNode* node = NULL;
    return node;
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