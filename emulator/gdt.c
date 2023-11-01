#include "gdt.h"

void gdt_set_entry(uint8_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    gdt_entry_t *entry = (gdt_entry_t *)(&vm_memory[((gdtr_t *)&vm_memory[cpu_read_reg(cpu_reg_gdtr)])->base]);
    entry[num].base_low = (base & 0xFFFF);
    entry[num].base_middle = (base >> 16) & 0xFF;
    entry[num].base_high = (base >> 24) & 0xFF;
    entry[num].limit_low = (limit & 0xFFFF);
    entry[num].granularity = (limit >> 16) & 0x0F;
    entry[num].granularity |= gran & 0xF0;
    entry[num].access = access;
}

global_uint64_t gdt_read_seg_offset(uint16_t seg)
{
    return cpu_read_reg(seg) << 4;
}

void gdt_setup()
{
    cpu_write_reg(cpu_reg_gs, 0x00);
    cpu_write_reg(cpu_reg_fs, 0x00);
    cpu_write_reg(cpu_reg_es, 0x00);
    cpu_write_reg(cpu_reg_ds, 0x00);
    cpu_write_reg(cpu_reg_cs, (0xfffff - limit(bios_size, (256 * 1024) - 1)) >> 4);
    cpu_write_reg(cpu_reg_ss, 0x00);
}