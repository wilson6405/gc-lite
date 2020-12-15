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

#define var (Object *)


typedef struct {
    Object *first_object;
    Object *stack[STACK_MAX];
    int32_t total_objs;
    int32_t max_objs;
    int32_t stack_size;
} VM;

VM *vm = NULL;

void gc(void);


void
assert(int condition, const char *message)
{
    if (!condition) {
        printf("%s\n", message);
        exit(1);
    }
}

void
init_vm(void)
{
    if (vm != NULL) {
        printf("vm is exist.");
        return;
    }

    vm = (VM *)malloc(sizeof(VM));
    vm->stack_size = 0;
    vm->first_object = NULL;
    vm->total_objs = 0;
    vm->max_objs = INITIAL_GC_THRESHOLD;
}

void
delete_vm(void)
{
    vm->stack_size = 0;
    gc();
    free(vm);
}

void
push(Object *value)
{
    assert(vm->stack_size < STACK_MAX, "Stack overflow!");
    vm->stack[vm->stack_size++] = value;
}

Object *
pop(void)
{
    assert(vm->stack_size > 0, "Stack underflow!");
    return vm->stack[--vm->stack_size];
}

Object *
new_object(object_type type)
{
    if (vm->total_objs == vm->max_objs) gc();

    Object *obj = (Object *)malloc(sizeof(Object));
    obj->type = type;
    obj->marked = false;

    obj->next = vm->first_object;
    vm->first_object = obj;
    vm->total_objs++;

    return obj;
}

void
push_int(int32_t value)
{
    Object *obj = new_object(OBJ_INT);
    obj->value = value;
    push(obj);
}

Object *
push_pair(void)
{
    Object *obj = new_object(OBJ_PAIR);
    obj->tail = pop();
    obj->head = pop();

    push(obj);
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
mark_all()
{
    for (intmax_t i = 0; i < vm->stack_size; i++)
        mark(vm->stack[i]);
}

void
sweep()
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
gc(void)
{
    mark_all();
    sweep();

    vm->max_objs = vm->total_objs * 2;
}

void test1()
{
    push_int(1);
    push_int(100);

    gc();
}






// TODO: Use _Generic
int main(void)
{
    init_vm();
    test1();


    delete_vm();
    return 0;
}
