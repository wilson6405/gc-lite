#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#define STACK_MAX 256
#define INITIAL_GC_THRESHOLD 8

typedef enum {
    OBJ_INT,
    OBJ_PAIR
} object_type;

typedef struct sObject {
    uint8_t marked;
    object_type type;

    union {
        /* OBJ_INT */
        int32_t value;
        
        /* OBJ_PAIR */
        struct {
            struct sObject *head;
            struct sObject *tail;
        };
    };

    struct sObject *next;

} Object;


typedef struct {
    Object *first_object;
    Object *stack[STACK_MAX];
    int32_t total_objs;
    int32_t max_objs;
    int32_t stack_size;
} VM;

void gc(VM *); 


void
assert(int condition, const char *message)
{
    if (!condition) {
        printf("%s\n", message);
        exit(1);
    }
}

VM *
new_vm() 
{
    VM *vm = (VM *)malloc(sizeof(VM));
    vm->stack_size = 0;
    vm->first_object = NULL;
    vm->total_objs = 0;
    vm->max_objs = INITIAL_GC_THRESHOLD;

    return vm;
}

void delete_vm(VM *vm)
{
    vm->stack_size = 0;
    gc(vm);
    free(vm);
}

void 
push(VM *vm, Object *value)
{
    assert(vm->stack_size < STACK_MAX, "Stack overflow!");
    vm->stack[vm->stack_size++] = value;
}

Object *
pop(VM *vm)
{
    assert(vm->stack_size > 0, "Stack underflow!");
    return vm->stack[--vm->stack_size];
}

Object *
new_object(VM *vm, object_type type)
{
    if (vm->total_objs == vm->max_objs) gc(vm);

    Object *obj = (Object *)malloc(sizeof(Object));
    obj->type = type;
    obj->marked = false;

    obj->next = vm->first_object;
    vm->first_object = obj;
    vm->total_objs++;

    return obj;
}

void 
push_int(VM *vm, int32_t value)
{
    Object *obj = new_object(vm, OBJ_INT);
    obj->value = value;
    push(vm, obj);
}

Object *
push_pair(VM *vm)
{
    Object *obj = new_object(vm, OBJ_PAIR);
    obj->tail = pop(vm);
    obj->head = pop(vm);

    push(vm, obj);
    return obj;
}

void
mark(Object *obj)
{
    if (obj->marked) return;

    obj->marked = true;

    if (obj->type == OBJ_PAIR) {
        mark(obj->head);
        mark(obj->tail);
    }
}

void
mark_all(VM *vm)
{
    for (intmax_t i = 0; i < vm->stack_size; i++)
        mark(vm->stack[i]);
}

void
sweep(VM *vm)
{
    Object **obj = &vm->first_object;

    while (*obj) {
        if ((*obj)->marked == false) {
            printf("Object is unreachable, value: %d\n", (*obj)->value);

            Object *unreached = *obj;
            *obj = unreached->next;
            
            free(unreached);

            vm->total_objs--;
        } else {
            (*obj)->marked = false;
            obj = &(*obj)->next;
        }
    }
}

void
gc(VM *vm)
{
    mark_all(vm);
    sweep(vm);

    vm->max_objs = vm->total_objs * 2;
}

void test1()
{
    VM *vm = new_vm();
    push_int(vm, 1);
    push_int(vm, 100);
    pop(vm);

    gc(vm);
    delete_vm(vm);
}






// TODO: Initial vm as global variable

int main(void)
{
    test1();
    return 0;
}



















