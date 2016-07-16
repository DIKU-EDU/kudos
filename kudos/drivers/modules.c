#include "drivers/modules.h"
#include "lib/libc.h"

void modules_init(){
    for(module_t *module = &__modules_start;
            module != &__modules_end; module++)
    {
        int err = module->init_fn();
        if(err){
            kprintf("Modules: %s failed initialization with error %d\n",
                    module->module_name, err);
        }
    }
}
