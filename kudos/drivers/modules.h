#define module_init(name, fn) \
    static module_t __module__##name \
    __attribute__ ((section ("modules_ptr"))) = {"name", fn}

typedef int(*module_init_t)(void);

typedef struct {
    char module_name[20];
    module_init_t init_fn;
} module_t;

extern module_t __modules_start;
extern module_t __modules_end;

void modules_init();
