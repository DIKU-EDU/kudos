#ifndef KUDOS_DRIVERS_MODULES_H
#define KUDOS_DRIVERS_MODULES_H

#define module_define(type, name, module) \
    static module_t __module__##name \
    __attribute__ ((section ("modules_ptr"))) = {type, #name, module};

#define module_init(name, fn) \
    static module_general_t __module_general__##name \
    __attribute__ ((section ("real_modules_ptr"))) = {fn}; \
    module_define(MODULE_TYPE_GENERAL, name, &__module_general__##name)

typedef enum {
    MODULE_TYPE_GENERAL,
    MODULE_TYPE_PCI
} module_type_t;

typedef int(*module_init_t)(void);

typedef struct {
    module_init_t init_fn;
} module_general_t;

typedef struct {
    module_type_t module_type;
    char module_name[20];
    void *module;
} module_t;

extern module_t __modules_start;
extern module_t __modules_end;

void modules_init();

#endif // KUDOS_DRIVERS_MODULES_H
