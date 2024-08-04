#include <stdio.h>
#include <stdlib.h>

#define INITIAL_GC_THRESHOLD 8 // number of objects to kick off first GC
#define STACK_MAX 256

typedef enum
{
    OBJ_INT,
    OBJ_PAIR
} ObjectType;

typedef struct sObject
{

    struct sObject *next; // next object in the list
    unsigned char marked;
    ObjectType type;

    union
    {
        int value;

        struct
        {
            struct sObject *head;
            struct sObject *tail;
        };
    };
} Object;

typedef struct
{

    // keep track to decide when to mark and sweep
    int numObjects;
    int maxObjects;

    Object *firstObject; // head
    Object *stack[STACK_MAX];
    int stackSize;
} VM;

VM *newVM()
{
    VM *vm = malloc(sizeof(VM));
    vm->stackSize = 0;
    vm->firstObject = NULL;
    vm->numObjects = 0;
    vm->maxObjects = INITIAL_GC_THRESHOLD;
    return vm;
}

void push(VM *vm, Object *value)
{
    assert(vm->stackSize < STACK_MAX, "Stack overflow!");
    vm->stack[vm->stackSize++] = value;
}

Object *pop(VM *vm)
{
    assert(vm->stackSize > 0, "Stack underflow!");
    return vm->stack[--vm->stackSize];
}

void mark(Object *object)
{

    if (object->marked)
        return; // avoid cycles
    object->marked = 1;

    if (object->type == OBJ_PAIR)
    {
        mark(object->head);
        mark(object->tail);
    }
}

void markAll(VM *vm)
{
    for (int i = 0; i < vm->stackSize; i++)
    {
        mark(vm->stack[i]);
    }
}

void sweep(VM *vm)
{
    Object **object = &vm->firstObject;
    while (*object)
    {
        if (!(*object)->marked)
        {
            // Not reached -> free it
            Object *unreached = *object;

            *object = unreached->next;
            free(unreached);
            vm->numObjects--;
        }
        else
        {
            (*object)->marked = 0;
            object = &(*object)->next;
        }
    };
}

void gc(VM *vm)
{
    int numObjects = vm->numObjects;

    markAll(vm);
    sweep(vm);

    vm->maxObjects = vm->numObjects * 2;
}

Object *newObject(VM *vm, ObjectType type)
{
    if (vm->numObjects == vm->maxObjects)
        gc(vm);

    Object *object = malloc(sizeof(Object));
    object->type = type;
    object->marked = 0;

    object->next = vm->firstObject;
    vm->firstObject = object;

    vm->numObjects++;
    return object;
}

void pushInt(VM *vm, int i)
{
    Object *object = newObject(vm, OBJ_INT);
    object->value = i;
    push(vm, object);
}

Object *pushPair(VM *vm)
{
    Object *object = newObject(vm, OBJ_PAIR);
    object->tail = pop(vm);
    object->head = pop(vm);
    push(vm, object);
    return object;
}
