/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    stack.c
 * @brief   Implementation of ADT stack.
 * @author  Wyslych Vojtech (xwyslyv00)
 */

#include "stack.h"
#include "error.h"

#include <stdbool.h>
#include <stdlib.h>

Stack* init_stack()
{
    Stack* stack = malloc(sizeof(Stack));
    if (!stack)
        return NULL;

    stack->top = NULL;
    stack->size = 0;

    return stack;
}

bool is_empty_stack(const Stack* stack)
{
    return stack->top == NULL;
}

void dispose_stack(Stack* stack)
{
    while (stack->top != NULL) {
        pop(stack);
    }

    stack->size = 0;
    free(stack);
}

int push(Stack* stack, void* data)
{
    StackEl* stack_el = malloc(sizeof(StackEl));
    if (!stack_el)
        return 1;

    stack_el->data = data;
    stack_el->next = stack->top;
    stack->top = stack_el;
    stack->size++;
    return 0;
}

int push_value(Stack* stack, const int value)
{
    StackEl* stack_el = malloc(sizeof(StackEl));
    if (!stack_el)
        return 1;

    stack_el->value = value;
    stack_el->next = stack->top;
    stack->top = stack_el;
    stack->size++;
    return 0;
}

void pop(Stack* stack)
{
    if (!stack->top)
        return;

    StackEl* stack_el = stack->top;
    stack->top = stack_el->next;
    stack->size--;
    free(stack_el);
}

void* top(const Stack* stack)
{
    if (stack->top == NULL)
        return NULL;

    return stack->top->data;
}

int top_value(const Stack* stack)
{
    if (stack->top == NULL)
        return -1;

    return stack->top->value;
}

int stack_size(const Stack* stack)
{
    return stack->size;
}