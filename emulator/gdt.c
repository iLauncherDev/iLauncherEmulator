#include "gdt.h"

void gdt_set_entry(uint8_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    gdt_entry_t *gdt = (gdt_entry_t *)(&vm_memory[((gdtr_t *)&vm_memory[cpu_read_reg(cpu_reg_gdtr)])->base]);
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;
    gdt[num].granularity |= gran & 0xF0;
    gdt[num].access = access;
}

void gdt_setup()
{
    cpu_write_reg(cpu_reg_gdtr, vm_memory_size + bios_size);
    cpu_write_reg(cpu_reg_gdtr_next, cpu_read_reg(cpu_reg_gdtr));
    gdtr_t *gdtr = (gdtr_t *)&vm_memory[cpu_read_reg(cpu_reg_gdtr)];
    gdtr->limit = sizeof(gdt_entry_t) * 5 - 1;
    gdtr->base = cpu_read_reg(cpu_reg_gdtr) + sizeof(gdtr_t);
    gdt_set_entry(0, 0, 0, 0, 0);
    gdt_set_entry(1, 0x000e0000, 0xffffffff, 0x9A, 0x0F);
    gdt_set_entry(2, 0x00000000, 0xffffffff, 0x92, 0x0F);
    cpu_write_reg(cpu_reg_gs, 0x10);
    cpu_write_reg(cpu_reg_fs, 0x10);
    cpu_write_reg(cpu_reg_es, 0x10);
    cpu_write_reg(cpu_reg_ds, 0x10);
    cpu_write_reg(cpu_reg_cs, 0x08);
    cpu_write_reg(cpu_reg_ss, 0x10);
}