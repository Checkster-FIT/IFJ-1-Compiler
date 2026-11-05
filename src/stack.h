/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    stack.h
 * @brief   Declarations of ADT stack functions and data types.
 * @author  Wyslych Vojtech (xwyslyv00)
 */

#ifndef IFJ_1_COMPILER_STACK_H
#define IFJ_1_COMPILER_STACK_H

#include <stdbool.h>

#define EMPTY_STACK (-1)

typedef struct stack_el {
    void* data;
    struct stack_el* next;
} StackEl;

typedef struct {
    StackEl* top;
} Stack;

/**
 * @brief Initialises an empty stack.
 *
 * @return Pointer to initialised empty stack.
 */
Stack* init_stack();

/**
 * @brief Checks if stack is empty (has no elements).
 *
 * @param stack Pointer to checked stack.
 * @return true if empty, false if not empty.
 */
bool is_empty_stack(Stack* stack);

/**
 * @brief Tops all elements from stack and frees allocated stack space.
 *
 * @param stack Pointer to stack to dispose.
 */
void dispose_stack(Stack* stack);

/**
 * @brief Allocates memory for new stack element and pushes the element into the stack.
 * Pointer is stored into the element.
 *
 * @param stack Pointer to stack.
 * @param data Pointer to data which is to be stored.
 * @return 0 on success, 1 on memory allocation fail
 */
int push(Stack* stack, void* data);

/**
 * @brief Pops top element from stack and frees its space
 *
 * @param stack Pointer to stack from which the element will be popped.
 */
void pop(Stack* stack);

/**
 * @brief Returns pointer stored in stack top element.
 *
 * @param stack Pointer to stack.
 * @return Pointer stored in the stack top element, NULL on empty stack.
 */
void* top(Stack* stack);

#endif // IFJ_1_COMPILER_STACK_H