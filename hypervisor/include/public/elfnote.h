/******************************************************************************
 * elfnote.h
 *
 * Definitions used for the acrn ELF notes.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Copyright (c) 2006, Ian Campbell, acrnSource Ltd.
 */

#ifndef __ACRN_PUBLIC_ELFNOTE_H__
#define __ACRN_PUBLIC_ELFNOTE_H__

/*
 * `incontents 200 elfnotes ELF notes
 *
 * The notes should live in a PT_NOTE segment and have "acrn" in the
 * name field.
 *
 * Numeric types are either 4 or 8 bytes depending on the content of
 * the desc field.
 *
 * LEGACY indicated the fields in the legacy __acrn_guest string which
 * this a note type replaces.
 *
 * String values (for non-legacy) are NULL terminated ASCII, also known
 * as ASCIZ type.
 */

/*
 * NAME=VALUE pair (string).
 */
#define ACRN_ELFNOTE_INFO           0

/*
 * The virtual address of the entry point (numeric).
 *
 * LEGACY: VIRT_ENTRY
 */
#define ACRN_ELFNOTE_ENTRY          1

/* The virtual address of the hypercall transfer page (numeric).
 *
 * LEGACY: HYPERCALL_PAGE. (n.b. legacy value is a physical page
 * number not a virtual address)
 */
#define ACRN_ELFNOTE_HYPERCALL_PAGE 2

/* The virtual address where the kernel image should be mapped (numeric).
 *
 * Defaults to 0.
 *
 * LEGACY: VIRT_BASE
 */
#define ACRN_ELFNOTE_VIRT_BASE      3

/*
 * The offset of the ELF paddr field from the actual required
 * pseudo-physical address (numeric).
 *
 * This is used to maintain backwards compatibility with older kernels
 * which wrote __PAGE_OFFSET into that field. This field defaults to 0
 * if not present.
 *
 * LEGACY: ELF_PADDR_OFFSET. (n.b. legacy default is VIRT_BASE)
 */
#define ACRN_ELFNOTE_PADDR_OFFSET   4

/*
 * The version of acrn that we work with (string).
 *
 * LEGACY: ACRN_VER
 */
#define ACRN_ELFNOTE_ACRN_VERSION    5

/*
 * The name of the guest operating system (string).
 *
 * LEGACY: GUEST_OS
 */
#define ACRN_ELFNOTE_GUEST_OS       6

/*
 * The version of the guest operating system (string).
 *
 * LEGACY: GUEST_VER
 */
#define ACRN_ELFNOTE_GUEST_VERSION  7

/*
 * The loader type (string).
 *
 * LEGACY: LOADER
 */
#define ACRN_ELFNOTE_LOADER         8

/*
 * The kernel supports PAE (x86/32 only, string = "yes", "no" or
 * "bimodal").
 *
 * For compatibility with acrn 3.0.3 and earlier the "bimodal" setting
 * may be given as "yes,bimodal" which will cause older acrn to treat
 * this kernel as PAE.
 *
 * LEGACY: PAE (n.b. The legacy interface included a provision to
 * indicate 'extended-cr3' support allowing L3 page tables to be
 * placed above 4G. It is assumed that any kernel new enough to use
 * these ELF notes will include this and therefore "yes" here is
 * equivalent to "yes[entended-cr3]" in the __acrn_guest interface.
 */
#define ACRN_ELFNOTE_PAE_MODE       9

/*
 * The features supported/required by this kernel (string).
 *
 * The string must consist of a list of feature names (as given in
 * features.h, without the "ACRNFEAT_" prefix) separated by '|'
 * characters. If a feature is required for the kernel to function
 * then the feature name must be preceded by a '!' character.
 *
 * LEGACY: FEATURES
 */
#define ACRN_ELFNOTE_FEATURES      10

/*
 * The kernel requires the symbol table to be loaded (string = "yes" or "no")
 * LEGACY: BSD_SYMTAB (n.b. The legacy treated the presence or absence
 * of this string as a boolean flag rather than requiring "yes" or
 * "no".
 */
#define ACRN_ELFNOTE_BSD_SYMTAB    11

/*
 * The lowest address the hypervisor hole can begin at (numeric).
 *
 * This must not be set higher than HYPERVISOR_VIRT_START. Its presence
 * also indicates to the hypervisor that the kernel can deal with the
 * hole starting at a higher address.
 */
#define ACRN_ELFNOTE_HV_START_LOW  12

/*
 * List of maddr_t-sized mask/value pairs describing how to recognize
 * (non-present) L1 page table entries carrying valid MFNs (numeric).
 */
#define ACRN_ELFNOTE_L1_MFN_VALID  13

/*
 * Whether or not the guest supports cooperative suspend cancellation.
 * This is a numeric value.
 *
 * Default is 0
 */
#define ACRN_ELFNOTE_SUSPEND_CANCEL 14

/*
 * The (non-default) location the initial phys-to-machine map should be
 * placed at by the hypervisor (Dom0) or the tools (DomU).
 * The kernel must be prepared for this mapping to be established using
 * large pages, despite such otherwise not being available to guests. Note
 * that these large pages may be misaligned in PFN space (they'll obviously
 * be aligned in MFN and virtual address spaces).
 * The kernel must also be able to handle the page table pages used for
 * this mapping not being accessible through the initial mapping.
 * (Only x86-64 supports this at present.)
 */
#define ACRN_ELFNOTE_INIT_P2M      15

/*
 * Whether or not the guest can deal with being passed an initrd not
 * mapped through its initial page tables.
 */
#define ACRN_ELFNOTE_MOD_START_PFN 16

/*
 * The features supported by this kernel (numeric).
 *
 * Other than ACRN_ELFNOTE_FEATURES on pre-4.2 acrn, this note allows a
 * kernel to specify support for features that older hypervisors don't
 * know about. The set of features 4.2 and newer hypervisors will
 * consider supported by the kernel is the combination of the sets
 * specified through this and the string note.
 *
 * LEGACY: FEATURES
 */
#define ACRN_ELFNOTE_SUPPORTED_FEATURES 17

/*
 * Physical entry point into the kernel.
 *
 * 32bit entry point into the kernel. When requested to launch the
 * guest kernel in a HVM container, acrn will use this entry point to
 * launch the guest in 32bit protected mode with paging disabled.
 * Ignored otherwise.
 */
#define ACRN_ELFNOTE_PHYS32_ENTRY 18

/*
 * The number of the highest elfnote defined.
 */
#define ACRN_ELFNOTE_MAX ACRN_ELFNOTE_PHYS32_ENTRY

/*
 * System information exported through crash notes.
 *
 * The kexec / kdump code will create one ACRN_ELFNOTE_CRASH_INFO
 * note in case of a system crash. This note will contain various
 * information about the system, see acrn/include/acrn/elfcore.h.
 */
#define ACRN_ELFNOTE_CRASH_INFO 0x1000001

/*
 * System registers exported through crash notes.
 *
 * The kexec / kdump code will create one ACRN_ELFNOTE_CRASH_REGS
 * note per cpu in case of a system crash. This note is architecture
 * specific and will contain registers not saved in the "CORE" note.
 * See acrn/include/acrn/elfcore.h for more information.
 */
#define ACRN_ELFNOTE_CRASH_REGS 0x1000002


/*
 * acrn dump-core none note.
 * xm dump-core code will create one ACRN_ELFNOTE_DUMPCORE_NONE
 * in its dump file to indicate that the file is acrn dump-core
 * file. This note doesn't have any other information.
 * See tools/libxc/xc_core.h for more information.
 */
#define ACRN_ELFNOTE_DUMPCORE_NONE               0x2000000

/*
 * acrn dump-core header note.
 * xm dump-core code will create one ACRN_ELFNOTE_DUMPCORE_HEADER
 * in its dump file.
 * See tools/libxc/xc_core.h for more information.
 */
#define ACRN_ELFNOTE_DUMPCORE_HEADER             0x2000001

/*
 * acrn dump-core acrn version note.
 * xm dump-core code will create one ACRN_ELFNOTE_DUMPCORE_ACRN_VERSION
 * in its dump file. It contains the acrn version obtained via the
 * ACRNVER hypercall.
 * See tools/libxc/xc_core.h for more information.
 */
#define ACRN_ELFNOTE_DUMPCORE_ACRN_VERSION        0x2000002

/*
 * acrn dump-core format version note.
 * xm dump-core code will create one ACRN_ELFNOTE_DUMPCORE_FORMAT_VERSION
 * in its dump file. It contains a format version identifier.
 * See tools/libxc/xc_core.h for more information.
 */
#define ACRN_ELFNOTE_DUMPCORE_FORMAT_VERSION     0x2000003

#endif /* __ACRN_PUBLIC_ELFNOTE_H__ */

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
