#include <asm.h>
#include "drivers/x86_64/pci.h"
#include "kernel/stalloc.h"
#include "drivers/modules.h"
#include "lib/libc.h"

/* Class Codes */
static const char * class_code[18] = 
{ 
"Legacy", "Mass Storage Controller", "Network Controller", 
"Video Controller", "Multimedia Unit", "Memory Controller", "Bridge",
"Simple Communications Controller", "Base System Peripheral", "Input Device",
"Docking Station", "Processor", "Serial Bus Controller", "Wireless Controller",
"Intelligent I/O Controller", "Satellite Communication Controller", 
"Encryption/Decryption Controller", "Data Acquisition and Signal Processing Controller"
};

static const char * subclass[13][8] = 
{
{ "Legacy", "VGA", "Other", "Other", "Other", "Other", "Other", "Other" },
{ "SCSI", "IDE", "Floppy", "IPI", "RAID", "Other", "SATA", "Other" },
{ "Ethernet", "Token ring", "FDDI", "ATM", "ISDN", "WorldFip", "PICMG 2.14 Multi Computing", "Other" },
{ "VGA", "XGA", "3D Controller", "Other", "Other", "Other", "Other", "Other" },
{ "Video", "Audio", "Computer Telephony", "Other", "Other", "Other", "Other", "Other" },
{ "RAM", "Flash", "Other", "Other", "Other", "Other", "Other", "Other" },
{ "Host", "ISA", "EISA", "MCA", "PCI-PCI", "PCMCIA", "NuBus", "CardBus" },
{ "Serial", "Parallel", "Other", "Other", "Other", "Other", "Other", "Other" },
{ "PIC", "DMA", "PIT", "RTC", "Other", "Other", "Other", "Other" },
{ "Keyboard", "Digitizer", "Mouse", "Other", "Other", "Other", "Other", "Other" },
{ "Generic", "Other", "Other", "Other", "Other", "Other", "Other", "Other" },
{ "386", "486", "Pentium", "Other", "Other", "Other", "Other", "Other" },
{ "Firewire", "ACCESS", "SSA", "USB", "Other", "Other", "Other", "Other" }
};

/**
 * Reads a dword from a given pci register
 * 
 * @param bus The target bus
 * @param dev The target device
 * @param func The target function
 * @param reg The target port register
 *
 * @return A dword. The dword it read from a port
 */
uint32_t pci_read_dword(uint16_t bus, uint16_t dev, uint16_t func, uint32_t reg)
{
    /* Select specific func */
    _outl(0xCF8, 0x80000000 | ((uint32_t)bus << 16) | 
        ((uint32_t)dev << 11) | ((uint32_t)func << 8) |
        (reg & ~3));

    /* Read from it */
    return _inl(0xCFC + (reg & 3));
}

/**
 * Tries to locate a pci interface at given bus, device and function
 *
 * @param pcs The structure where to store information
 * @param bus The bus it needs to search
 * @param dev The device it needs to search
 * @param func The function it needs to search
 *
 * @return A boolean. 0 if pci interface not found.
 */
uint8_t get_pci_config(pci_conf_t *pcs, uint16_t bus, uint16_t dev, uint16_t func)
{
    /* Get the dword and parse vendor & device id */
    uint32_t data = pci_read_dword(bus, dev, func, 0);
    uint16_t vendor = (data & 0xFFFF);
    uint32_t i;

    if(vendor && vendor != 0xFFFF)
    {
        /* Valid device!! Read config space and fill up structure */
        for(i = 0; i < 64; i += 16)
        {
            *(uint32_t*)((uint64_t)pcs + i) = pci_read_dword(bus, dev, func, i);
            *(uint32_t*)((uint64_t)pcs + i + 4) = pci_read_dword(bus, dev, func, i + 4);
            *(uint32_t*)((uint64_t)pcs + i + 8) = pci_read_dword(bus, dev, func, i + 8);
            *(uint32_t*)((uint64_t)pcs + i + 12) = pci_read_dword(bus, dev, func, i + 12);
        }

        /* Print what kind of device! */
        if(pcs->classcode < 13 && pcs->subclass != 0x80 && pcs->device_id != 0x7a0)
        {
            kprintf("PCI: [%u:%u][%u:%u:%u] Vendor 0x%x, Device 0x%x: %s %s\n",
                pcs->classcode, pcs->subclass, bus, dev, func, 
                pcs->vendor_id, pcs->device_id, 
                subclass[pcs->classcode][pcs->subclass],
                class_code[pcs->classcode]);
        }

        return 1;
    }

    /* Didn't exist */
    return 0;
}

module_t* pci_find_module(uint8_t classcode, uint8_t subclass){
    for(module_t *module = &__modules_start;
            module != &__modules_end; module++)
    {
        if(module->module_type == MODULE_TYPE_PCI){
            pci_device_module_t *pci_module =
                (pci_device_module_t*)module->module;

            if(classcode == pci_module->classcode &&
                    subclass == pci_module->subclass){
                return module;
            }
        }
    }

    return NULL;
}

int pci_init(){
    /* Temp space */
    pci_conf_t pcs = {0};

    /* Scan PCI */
    uint16_t bus, dev, func;

    kprintf("Scanning PCI\n");
    for(bus = 0; bus < 256; bus++)
    {
        for(dev = 0; dev < 32; dev++)
        {
            for(func = 0; func < 8; func++)
            {
                uint8_t res = get_pci_config(&pcs, bus, dev, func);

                /* Was a device detected on this address? */
                if(res != 0)
                {
                    /* Does there exist a driver for the device */
                    module_t *pci_module = pci_find_module(pcs.classcode,
                            pcs.subclass);
                    if(pci_module == NULL){
                        kprintf(" - No driver found\n");
                        continue;
                    }
                    kprintf(" - Driver found\n");
                    ((pci_device_module_t*)pci_module->module)
                        ->device_handler((io_descriptor_t*)&pcs);

                    ///* Is this an IDE device? */
                    //if(pcs.classcode == 0x1 && pcs.subclass == 0x1)
                    //{
                    //    /* Yes */
                    //    device_table[number_of_devices] = 
                    //        disk_init((io_descriptor_t*)&pcs);
                    //    number_of_devices++;
                    //}
                }
            }
        }
    }

    return 0;
}

module_init(PCI_module, pci_init);
