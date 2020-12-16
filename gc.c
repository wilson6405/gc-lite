#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define STACK_MAX 256
#define INITIAL_GC_THRESHOLD 8

typedef enum {
    OBJ_NUMBER,
    OBJ_STRING,
    OBJ_REF,
    OBJ_PAIR
} object_type;

typedef struct sObject {
    uint8_t marked;
    ssize_t ref_count;
    object_type type;

    union {
        double number;
        char *string;
        struct sObject *ref_obj;

        /* OBJ_PAIR */
        struct {
            struct sObject *head;
            struct sObject *tail;
        };
    };

    struct sObject *next;
} Object;

#define var Object *

#define OBJECT(X) _Generic((X), \
    int: new_number_object, \
    float: new_number_object, \
    double: new_number_object, \
    char *: new_string_object, \
    Object *: new_ref_object \
)(X)

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
    if (vm == NULL) {
        printf("vm is not exist.");
        return;
    }

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

static Object *
new_object(object_type type)
{
    if (vm->total_objs == vm->max_objs) gc();

    Object *obj = (Object *)calloc(1, sizeof(Object));
    obj->type = type;
    obj->ref_count = 1;
    obj->marked = false;

    obj->next = vm->first_object;
    vm->first_object = obj;
    vm->total_objs++;

    push(obj);
    return obj;
}

static Object *
new_number_object(double number)
{
    Object *obj = new_object(OBJ_NUMBER);
    obj->number = number;

    return obj;
}

static Object *
new_string_object(char *string)
{
    Object *obj = new_object(OBJ_STRING);
    int len = strlen(string) + 1;
    char *str = malloc(len);
    strncpy(str, string, len);

    obj->string = str;

    return obj;
}

static Object *
new_ref_object(Object *ref_obj)
{
    Object *obj = new_object(OBJ_REF);
    obj->ref_obj = ref_obj;
    obj->ref_obj->ref_count++;

    return obj;
}

void
delete_object(Object *obj)
{
    if (obj == NULL) return;

    object_type type = obj->type;

    switch(type) {
    case OBJ_NUMBER:
        free(obj);
        break;

    case OBJ_STRING:
        free(obj->string);
        free(obj);
        break;

    case OBJ_REF:
        if (obj->ref_obj)
            obj->ref_obj->ref_count--;
        free(obj);
        break;
    default:
        printf("Unknown object type.");
    }

    obj = NULL;
}

Object *
push_pair(void)
{
    Object *obj = new_object(OBJ_PAIR);
    obj->tail = pop();
    obj->head = pop();

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
            Object *unreached = *obj;
            *obj = unreached->next;

            delete_object(unreached);

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

void
test1()
{
    var int_obj = OBJECT(100);
    var str_obj = OBJECT("Wilson");
    var double_obj = OBJECT(99.99);
    
    gc();
}

void
perf_test(void)
{
    var o = NULL;

    for (int i = 0; i < 1000; i++) {
        for (int j = 0; j < 20; j++) {
            o = OBJECT(i);
        }

        for (int k = 0; k < 20; k++) {
            pop();
        }
    }

    gc();
}

int main(void)
{
    init_vm();

    // test1();
    perf_test();

    delete_vm();
    return 0;
}
