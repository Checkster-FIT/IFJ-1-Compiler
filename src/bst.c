/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    bst.c
 * @brief
 * @author
 */

#include "bst.h"

TNode* TreeInit() {
    TNode* node = NULL;
    return node;
}

TNode* NodeCreate(void* key, void* value) {
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