#include "drivers/modules.h"
#include "lib/libc.h"

void modules_init() {
  module_t *module;
  int err;

  for(module = &__modules_start; module != &__modules_end; module++) {
    switch(module->module_type) {
      case MODULE_TYPE_GENERAL:
        err = ((module_general_t*)module->module)->init_fn();
        break;
      case MODULE_TYPE_PCI:
        continue;
      default:
        kprintf("Modules: %s of type %d not implemented\n",
            module->module_name, module->module_type);
        continue;
    }

    if(err) {
      kprintf("Modules: %s failed initialization with error %d\n",
          module->module_name, err);
    }
  }
}
