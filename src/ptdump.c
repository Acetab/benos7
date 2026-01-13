// src/ptdump.c
#include "printk.h"
#include "asm/csr.h"
#include "asm/pgtable.h"
#include "asm/pgtable_hwdef.h"
#include "asm/pgtable_types.h"
#include "mm.h"

static void print_flags(unsigned long val)
{
    printk("%c", (val & _PAGE_PRESENT)  ? 'V' : '-');
    printk("%c", (val & _PAGE_READ)     ? 'R' : '-');
    printk("%c", (val & _PAGE_WRITE)    ? 'W' : '-');
    printk("%c", (val & _PAGE_EXEC)     ? 'X' : '-');
    printk("%c", (val & _PAGE_USER)     ? 'U' : '-');
    printk("%c", (val & _PAGE_GLOBAL)   ? 'G' : '-');
    printk("%c", (val & _PAGE_ACCESSED) ? 'A' : '-');
    printk("%c", (val & _PAGE_DIRTY)    ? 'D' : '-');
}

static inline unsigned long pte_pfn(unsigned long pteval)
{
    // PFN 在 [XLEN-1:10]
    return (pteval >> _PAGE_PFN_SHIFT);
}

static void dump_level(const char *tag, int level, unsigned long index,
                       unsigned long entry_val)
{
    unsigned long pfn = pte_pfn(entry_val);
    printk("[%s][L%d] idx=%03ld PFN=0x%lx flags=", tag, level, index, pfn);
    print_flags(entry_val);
    printk(" raw=0x%lx\n", entry_val);
}

static void walk_pte_table(pte_t *pte_base)
{
    for (unsigned long i = 0; i < PTRS_PER_PTE; i++) {
        unsigned long v = pte_val(pte_base[i]);
        if (!v) continue;

        dump_level("PTE", 0, i, v);
    }
}

static void walk_pmd_table(pmd_t *pmd_base)
{
    for (unsigned long i = 0; i < PTRS_PER_PMD; i++) {
        unsigned long v = pmd_val(pmd_base[i]);
        if (!v) continue;

        dump_level("PMD", 1, i, v);

        // 叶子：表示 2MB 大页映射（R/W/X 任意非0）
        if (v & _PAGE_LEAF) {
            continue;
        }

        // 非叶子：指向下一级 PTE 表
        unsigned long next_pa = (pte_pfn(v) << PTE_SHIFT);
        pte_t *pte_base = (pte_t *)next_pa;   // 若有 pa2va()，这里改成 (pte_t*)pa2va(next_pa)
        walk_pte_table(pte_base);
    }
}

static void walk_pgd_table(pgd_t *pgd_base)
{
    for (unsigned long i = 0; i < PTRS_PER_PGD; i++) {
        unsigned long v = pgd_val(pgd_base[i]);
        if (!v) continue;

        dump_level("PGD", 2, i, v);

        // 叶子：表示 1GB 大页映射
        if (v & _PAGE_LEAF) {
            continue;
        }

        // 非叶子：指向下一级 PMD 表
        unsigned long next_pa = (pte_pfn(v) << PTE_SHIFT);
        pmd_t *pmd_base = (pmd_t *)next_pa;   // 若有 pa2va()，这里改成 (pmd_t*)pa2va(next_pa)
        walk_pmd_table(pmd_base);
    }
}

void pt_dump_from_satp(void)
{
    unsigned long satp = read_csr(satp);
    // satp[43:0] PPN（Sv39）
    unsigned long root_ppn = satp & ((1UL << 44) - 1);
    unsigned long root_pa = root_ppn << PAGE_SHIFT;

    printk("=== PageTable Dump (Sv39) ===\n");
    printk("satp=0x%lx root_ppn=0x%lx root_pa=0x%lx\n", satp, root_ppn, root_pa);

    pgd_t *pgd = (pgd_t *)root_pa;  // 若有 pa2va()，这里改成 (pgd_t*)pa2va(root_pa)
    walk_pgd_table(pgd);

    printk("=== PageTable Dump End ===\n");
}
