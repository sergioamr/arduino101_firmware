/* nano_private.h - private nanokernel definitions (IA-32) */

/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
This file contains private nanokernel structures definitions and various other
definitions for the Intel Architecture 32 bit (IA-32) processor architecture.
The header include/nanokernel.h contains the public nanokernel interface
definitions, with include/arch/nanokernel/x86/arch.h supplying the
IA-32 specific portions of the public nanokernel interface.

This file is also included by assembly language files which must #define
_ASMLANGUAGE before including this header file.  Note that nanokernel assembly
source files obtains structure offset values via "absolute symbols" in the
offsets.o module.
 */

#ifndef _NANO_PRIVATE_H
#define _NANO_PRIVATE_H

#include <toolchain.h>
#include <sections.h>
#include <asm_inline.h>

#ifndef _ASMLANGUAGE
#include <nanokernel.h>		   /* public nanokernel API */
#include <../../../kernel/nanokernel/include/nano_internal.h>
#include <stdint.h>
/*
 * This pulls in the code shared with the IDT generator that actually
 * constructs an IDT entry.
 */
#include <idtEnt.h>
#include <misc/dlist.h>
#endif

/* increase to 16 bytes (or more?) to support SSE/SSE2 instructions? */

#define STACK_ALIGN_SIZE 4

/*
 * Bitmask definitions for the tCCS->flags bit field
 *
 * The USE_FP flag bit will be set whenever a context uses any non-integer
 * capability, whether it's just the x87 FPU capability, SSE instructions, or
 * a combination of both. The USE_SSE flag bit will only be set if a context
 * uses SSE instructions.
 *
 * Note: Any change to the definitions USE_FP and USE_SSE must also be made to
 * nanokernel/x86/arch.h.
 */

#define FIBER 0
#define TASK 0x1	   /* 1 = task context, 0 = fiber context   */
#define INT_ACTIVE 0x2     /* 1 = context is executing interrupt handler */
#define EXC_ACTIVE 0x4     /* 1 = context is executing exception handler */
#define USE_FP 0x10	/* 1 = context uses floating point unit */
#define USE_SSE 0x20       /* 1 = context uses SSEx instructions */
#define PREEMPTIBLE 0x100  /* 1 = preemptible context */
#define ESSENTIAL 0x200    /* 1 = system context that must not abort */
#define NO_METRICS 0x400   /* 1 = _Swap() not to update task metrics */
#define NO_METRICS_BIT_OFFSET 0xa /* Bit position of NO_METRICS */

#define INT_OR_EXC_MASK (INT_ACTIVE | EXC_ACTIVE)

/*
 * Exception/interrupt vector definitions: vectors 20 to 31 are reserved for
 * Intel; vectors 32 to 255 are user defined interrupt vectors.
 */

#define IV_DIVIDE_ERROR 0
#define IV_DEBUG 1
#define IV_NON_MASKABLE_INTERRUPT 2
#define IV_BREAKPOINT 3
#define IV_OVERFLOW 4
#define IV_BOUND_RANGE 5
#define IV_INVALID_OPCODE 6
#define IV_DEVICE_NOT_AVAILABLE 7
#define IV_DOUBLE_FAULT 8
#define IV_COPROC_SEGMENT_OVERRUN 9
#define IV_INVALID_TSS 10
#define IV_SEGMENT_NOT_PRESENT 11
#define IV_STACK_FAULT 12
#define IV_GENERAL_PROTECTION 13
#define IV_PAGE_FAULT 14
#define IV_RESERVED 15
#define IV_X87_FPU_FP_ERROR 16
#define IV_ALIGNMENT_CHECK 17
#define IV_MACHINE_CHECK 18
#define IV_SIMD_FP 19
#define IV_INTEL_RESERVED_END 31

/*
 * Model specific register (MSR) definitions.  Use the _MsrRead() and
 * _MsrWrite() primitives to read/write the MSRs.  Only the so-called
 * "Architectural MSRs" are listed, i.e. the subset of MSRs and associated
 * bit fields which will not change on future processor generations.
 */

#define IA32_P5_MC_ADDR_MSR 0x0000
#define IA32_P5_MC_TYPE_MSR 0x0001
#define IA32_MONITOR_FILTER_SIZE_MSR 0x0006
#define IA32_TIME_STAMP_COUNTER_MSR 0x0010
#define IA32_IA32_PLATFORM_ID_MSR 0x0017
#define IA32_APIC_BASE_MSR 0x001b
#define IA32_FEATURE_CONTROL_MSR 0x003a
#define IA32_BIOS_SIGN_MSR 0x008b
#define IA32_SMM_MONITOR_CTL_MSR 0x009b
#define IA32_PMC0_MSR 0x00c1
#define IA32_PMC1_MSR 0x00c2
#define IA32_PMC2_MSR 0x00c3
#define IA32_PMC3_MSR 0x00c4
#define IA32_MPERF_MSR 0x00e7
#define IA32_APERF_MSR 0x00e8
#define IA32_MTRRCAP_MSR 0x00fe
#define IA32_SYSENTER_CS_MSR 0x0174
#define IA32_SYSENTER_ESP_MSR 0x0175
#define IA32_SYSENTER_EIP_MSR 0x0176
#define IA32_MCG_CAP_MSR 0x0179
#define IA32_MCG_STATUS_MSR 0x017a
#define IA32_MCG_CTL_MSR 0x017b
#define IA32_PERFEVTSEL0_MSR 0x0186
#define IA32_PERFEVTSEL1_MSR 0x0187
#define IA32_PERFEVTSEL2_MSR 0x0188
#define IA32_PERFEVTSEL3_MSR 0x0188
#define IA32_PERF_STATUS_MSR 0x0198
#define IA32_PERF_CTL_MSR 0x0199
#define IA32_CLOCK_MODULATION_MSR 0x019a
#define IA32_THERM_INTERRUPT_MSR 0x019b
#define IA32_THERM_STATUS_MSR 0x019c
#define IA32_MISC_ENABLE_MSR 0x01a0
#define IA32_ENERGY_PERF_BIAS_MSR 0x01b0
#define IA32_DEBUGCTL_MSR 0x01d9
#define IA32_SMRR_PHYSBASE_MSR 0x01f2
#define IA32_SMRR_PHYSMASK_MSR 0x01f3
#define IA32_PLATFORM_DCA_CAP_MSR 0x01f8
#define IA32_CPU_DCA_CAP_MSR 0x01f9
#define IA32_DCA_0_CAP_MSR 0x01fa
#define IA32_MTRR_PHYSBASE0_MSR 0x0200
#define IA32_MTRR_PHYSMASK0_MSR 0x0201
#define IA32_MTRR_PHYSBASE1_MSR 0x0202
#define IA32_MTRR_PHYSMASK1_MSR 0x0203
#define IA32_MTRR_PHYSBASE2_MSR 0x0204
#define IA32_MTRR_PHYSMASK2_MSR 0x0205
#define IA32_MTRR_PHYSBASE3_MSR 0x0206
#define IA32_MTRR_PHYSMASK3_MSR 0x0207
#define IA32_MTRR_PHYSBASE4_MSR 0x0208
#define IA32_MTRR_PHYSMASK4_MSR 0x0209
#define IA32_MTRR_PHYSBASE5_MSR 0x020a
#define IA32_MTRR_PHYSMASK5_MSR 0x020b
#define IA32_MTRR_PHYSBASE6_MSR 0x020c
#define IA32_MTRR_PHYSMASK6_MSR 0x020d
#define IA32_MTRR_PHYSBASE7_MSR 0x020e
#define IA32_MTRR_PHYSMASK7_MSR 0x020f
#define IA32_MTRR_FIX64K_00000_MSR 0x0250
#define IA32_MTRR_FIX16K_80000_MSR 0x0258
#define IA32_MTRR_FIX16K_A0000_MSR 0x0259
#define IA32_MTRR_FIX4K_C0000_MSR 0x0268
#define IA32_MTRR_FIX4K_C8000_MSR 0x0269
#define IA32_MTRR_FIX4K_D0000_MSR 0x026a
#define IA32_MTRR_FIX4K_D8000_MSR 0x026b
#define IA32_MTRR_FIX4K_E0000_MSR 0x026c
#define IA32_MTRR_FIX4K_E8000_MSR 0x026d
#define IA32_MTRR_FIX4K_F0000_MSR 0x026e
#define IA32_MTRR_FIX4K_F8000_MSR 0x026f
#define IA32_PAT_MSR 0x0277
#define IA32_MC0_CTL2_MSR 0x0280
#define IA32_MC1_CTL2_MSR 0x0281
#define IA32_MC2_CTL2_MSR 0x0282
#define IA32_MC3_CTL2_MSR 0x0283
#define IA32_MC4_CTL2_MSR 0x0284
#define IA32_MC5_CTL2_MSR 0x0285
#define IA32_MC6_CTL2_MSR 0x0286
#define IA32_MC7_CTL2_MSR 0x0287
#define IA32_MC8_CTL2_MSR 0x0288
#define IA32_MC9_CTL2_MSR 0x0289
#define IA32_MC10_CTL2_MSR 0x028a
#define IA32_MC11_CTL2_MSR 0x028b
#define IA32_MC12_CTL2_MSR 0x028c
#define IA32_MC13_CTL2_MSR 0x028d
#define IA32_MC14_CTL2_MSR 0x028e
#define IA32_MC15_CTL2_MSR 0x028f
#define IA32_MC16_CTL2_MSR 0x0290
#define IA32_MC17_CTL2_MSR 0x0291
#define IA32_MC18_CTL2_MSR 0x0292
#define IA32_MC19_CTL2_MSR 0x0293
#define IA32_MC20_CTL2_MSR 0x0294
#define IA32_MC21_CTL2_MSR 0x0295
#define IA32_MTRR_DEF_TYPE_MSR 0x02ff
#define IA32_FIXED_CTR0_MSR 0x0309
#define IA32_FIXED_CTR1_MSR 0x030a
#define IA32_FIXED_CTR2_MSR 0x030b
#define IA32_PERF_CAPABILITIES_MSR 0x0345
#define IA32_FIXED_CTR_CTL_MSR 0x038d
#define IA32_PERF_GLOBAL_STATUS_MSR 0x038e
#define IA32_PERF_GLOBAL_CTRL_MSR 0x038f
#define IA32_PERF_GLOBAL_OVF_CTRL_MSR 0x0390
#define IA32_PEBS_ENABLE_MSR 0x03f1
#define IA32_MC0_CTL_MSR 0x0400
#define IA32_MC0_STATUS 0x0401
#define IA32_MC0_ADDR_MSR 0x0402
#define IA32_MC0_MISC_MSR 0x0403
#define IA32_MC1_CTL_MSR 0x0404
#define IA32_MC1_STATUS_MSR 0x0405
#define IA32_MC1_ADDR_MSR 0x0406
#define IA32_MC1_MISC_MSR 0x0407
#define IA32_MC2_CTL_MSR 0x0408
#define IA32_MC2_STATUS_MSR 0x0409
#define IA32_MC2_ADDR_MSR 0x040a
#define IA32_MC2_MISC_MSR 0x040b
#define IA32_MC3_CTL_MSR 0x040c
#define IA32_MC3_STATUS_MSR 0x040d
#define IA32_MC3_ADDR_MSR 0x040e
#define IA32_MC3_MISC_MSR 0x040f
#define IA32_MC4_CTL_MSR 0x0410
#define IA32_MC4_STATUS_MSR 0x0411
#define IA32_MC4_ADDR_MSR 0x0412
#define IA32_MC4_MISC_MSR 0x0413
#define IA32_MC5_CTL_MSR 0x0414
#define IA32_MC5_STATUS_MSR 0x0415
#define IA32_MC5_ADDR_MSR 0x0416
#define IA32_MC5_MISC_MSR 0x0417
#define IA32_MC6_CTL_MSR 0x0418
#define IA32_MC6_STATUS_MSR 0x0419
#define IA32_MC6_ADDR_MSR 0x041a
#define IA32_MC6_MISC_MSR 0x041b
#define IA32_MC7_CTL_MSR 0x041c
#define IA32_MC7_STATUS_MSR 0x041d
#define IA32_MC7_ADDR_MSR 0x041e
#define IA32_MC7_MISC_MSR 0x041f
#define IA32_MC8_CTL_MSR 0x0420
#define IA32_MC8_STATUS_MSR 0x0421
#define IA32_MC8_ADDR_MSR 0x0422
#define IA32_MC8_MISC_MSR 0x0423
#define IA32_MC9_CTL_MSR 0x0424
#define IA32_MC9_STATUS_MSR 0x0425
#define IA32_MC9_ADDR_MSR 0x0426
#define IA32_MC9_MISC_MSR 0x0427
#define IA32_MC9_CTL_MSR 0x0424
#define IA32_MC9_STATUS_MSR 0x0425
#define IA32_MC9_ADDR_MSR 0x0426
#define IA32_MC9_MISC_MSR 0x0427
#define IA32_MC9_CTL_MSR 0x0424
#define IA32_MC9_STATUS_MSR 0x0425
#define IA32_MC9_ADDR_MSR 0x0426
#define IA32_MC9_MISC_MSR 0x0427
#define IA32_MC9_CTL_MSR 0x0424
#define IA32_MC9_STATUS_MSR 0x0425
#define IA32_MC9_ADDR_MSR 0x0426
#define IA32_MC9_MISC_MSR 0x0427
#define IA32_MC10_CTL_MSR 0x0428
#define IA32_MC10_STATUS_MSR 0x0429
#define IA32_MC10_ADDR_MSR 0x042a
#define IA32_MC10_MISC_MSR 0x042b
#define IA32_MC11_CTL_MSR 0x042c
#define IA32_MC11_STATUS_MSR 0x042d
#define IA32_MC11_ADDR_MSR 0x042e
#define IA32_MC11_MISC_MSR 0x042f
#define IA32_MC12_CTL_MSR 0x0430
#define IA32_MC12_STATUS_MSR 0x0431
#define IA32_MC12_ADDR_MSR 0x0432
#define IA32_MC12_MISC_MSR 0x0433
#define IA32_MC13_CTL_MSR 0x0434
#define IA32_MC13_STATUS_MSR 0x0435
#define IA32_MC13_ADDR_MSR 0x0436
#define IA32_MC13_MISC_MSR 0x0437
#define IA32_MC14_CTL_MSR 0x0438
#define IA32_MC14_STATUS_MSR 0x0439
#define IA32_MC14_ADDR_MSR 0x043a
#define IA32_MC14_MISC_MSR 0x043b
#define IA32_MC15_CTL_MSR 0x043c
#define IA32_MC15_STATUS_MSR 0x043d
#define IA32_MC15_ADDR_MSR 0x043e
#define IA32_MC15_MISC_MSR 0x043f
#define IA32_MC16_CTL_MSR 0x0440
#define IA32_MC16_STATUS_MSR 0x0441
#define IA32_MC16_ADDR_MSR 0x0442
#define IA32_MC16_MISC_MSR 0x0443
#define IA32_MC17_CTL_MSR 0x0444
#define IA32_MC17_STATUS_MSR 0x0445
#define IA32_MC17_ADDR_MSR 0x0446
#define IA32_MC17_MISC_MSR 0x0447
#define IA32_MC18_CTL_MSR 0x0448
#define IA32_MC18_STATUS_MSR 0x0449
#define IA32_MC18_ADDR_MSR 0x044a
#define IA32_MC18_MISC_MSR 0x044b
#define IA32_MC19_CTL_MSR 0x044c
#define IA32_MC19_STATUS_MSR 0x044d
#define IA32_MC19_ADDR_MSR 0x044e
#define IA32_MC19_MISC_MSR 0x044f
#define IA32_MC20_CTL_MSR 0x0450
#define IA32_MC20_STATUS_MSR 0x0451
#define IA32_MC20_ADDR_MSR 0x0452
#define IA32_MC20_MISC_MSR 0x0453
#define IA32_MC21_CTL_MSR 0x0454
#define IA32_MC21_STATUS_MSR 0x0455
#define IA32_MC21_ADDR_MSR 0x0456
#define IA32_MC21_MISC_MSR 0x0457
#define IA32_VMX_BASIC_MSR 0x0480
#define IA32_VMX_PINBASED_CTLS_MSR 0x0481
#define IA32_VMX_PROCBASED_CTLS_MSR 0x0482
#define IA32_VMX_EXIT_CTLS_MSR 0x0483
#define IA32_VMX_ENTRY_CTLS_MSR 0x0484
#define IA32_VMX_MISC_MSR 0x0485
#define IA32_VMX_CRO_FIXED0_MSR 0x0486
#define IA32_VMX_CRO_FIXED1_MSR 0x0487
#define IA32_VMX_CR4_FIXED0_MSR 0x0488
#define IA32_VMX_CR4_FIXED1_MSR 0x0489
#define IA32_VMX_VMCS_ENUM_MSR 0x048a
#define IA32_VMX_PROCBASED_CTLS2_MSR 0x048b
#define IA32_VMX_EPT_VPID_CAP_MSR 0x048c
#define IA32_VMX_TRUE_PINBASED_CTLS_MSR 0x048d
#define IA32_VMX_TRUE_PROCBASED_CTLS_MSR 0x048e
#define IA32_VMX_TRUE_EXIT_CTLS_MSR 0x048f
#define IA32_VMX_TRUE_ENTRY_CTLS_MSR 0x0490
#define IA32_DS_AREA_MSR 0x0600
#define IA32_EXT_XAPICID_MSR 0x0802
#define IA32_EXT_XAPIC_VERSION_MSR 0x0803
#define A32_EXT_XAPIC_TPR_MSR 0x0808
#define IA32_EXT_XAPIC_PPR_MSR 0x080a
#define IA32_EXT_XAPIC_EOI_MSR 0x080d
#define IA32_EXT_XAPIC_LDR_MSR 0x080c
#define IA32_EXT_XAPIC_SIVR_MSR 0x080f
#define IA32_EXT_XAPIC_ISR0_MSR 0x0810
#define IA32_EXT_XAPIC_ISR1_MSR 0x0811
#define A32_EXT_XAPIC_ISR2_MSR 0x0812
#define IA32_EXT_XAPIC_ISR3_MSR 0x0813
#define IA32_EXT_XAPIC_ISR4_MSR 0x0814
#define IA32_EXT_XAPIC_ISR5_MSR 0x0815
#define IA32_EXT_XAPIC_ISR6_MSR 0x0816
#define IA32_EXT_XAPIC_ISR7_MSR 0x0817
#define IA32_EXT_XAPIC_TMR0_MSR 0x0818
#define IA32_EXT_XAPIC_TMR1_MSR 0x0819
#define IA32_EXT_XAPIC_TMR2_MSR 0x081a
#define IA32_EXT_XAPIC_TMR3_MSR 0x081b
#define IA32_EXT_XAPIC_TMR4_MSR 0x081c
#define IA32_EXT_XAPIC_TMR5_MSR 0x081d
#define IA32_EXT_XAPIC_TMR6_MSR 0x081e
#define IA32_EXT_XAPIC_TMR7_MSR 0x081f
#define IA32_EXT_XAPIC_IRR0_MSR 0x0820
#define IA32_EXT_XAPIC_IRR1_MSR 0x0821
#define IA32_EXT_XAPIC_IRR2_MSR 0x0822
#define IA32_EXT_XAPIC_IRR3_MSR 0x0823
#define IA32_EXT_XAPIC_IRR4_MSR 0x0824
#define IA32_EXT_XAPIC_IRR5_MSR 0x0825
#define IA32_EXT_XAPIC_IRR6_MSR 0x0826
#define IA32_EXT_XAPIC_IRR7_MSR 0x0827
#define IA32_EXT_XAPIC_ESR_MSR 0x0828
#define IA32_EXT_XAPIC_LVT_CMCI_MSR 0x082f
#define IA32_EXT_XAPIC_ICR_MSR 0x0830
#define IA32_EXT_XAPIC_LVT_TIMER_MSR 0x0832
#define IA32_EXT_XAPIC_LVT_THERMAL_MSR 0x0833
#define IA32_EXT_XAPIC_LVT_PMI_MSR 0x0834
#define IA32_EXT_XAPIC_LVT_LINT0_MSR 0x0835
#define IA32_EXT_XAPIC_LVT_LINT1_MSR 0x0836
#define IA32_EXT_XAPIC_LVT_ERROR_MSR 0x0837
#define IA32_EXT_XAPIC_INIT_COUNT_MSR 0x0838
#define IA32_EXT_XAPIC_CUR_COUNT_MSR 0x0839
#define IA32_EXT_XAPIC_DIV_CONF_MSR 0x083e
#define IA32_EXT_XAPIC_SELF_IPI_MSR 0x083f
#define IA32_EFER_MSR 0xc0000080
#define IA32_STAR_MSR 0xc0000081
#define IA32_LSTAR_MSR 0xc0000082
#define IA32_FMASK_MSR 0xc0000084
#define IA32_FS_BASE_MSR 0xc0000100
#define IA32_GS_BASE_MSR 0xc0000101
#define IA32_KERNEL_GS_BASE_MSR 0xc0000102
#define IA32_TSC_AUX_MSR 0xc0000103

/*
 * EFLAGS value to utilize for the initial context:
 *
 *   IF (Interrupt Enable Flag) = 1
 *   IOPL bits			= 0
 *   All other "flags"          = Don't change state
 */

#define EFLAGS_INITIAL 0x00000200
#define EFLAGS_MASK 0x00003200

/* The following opcodes are used to create interrupt and exception stubs */

#define IA32_CALL_OPCODE 0xe8
#define IA32_PUSH_OPCODE 0x68
#define IA32_JMP_OPCODE 0xe9
#define IA32_ADD_OPCODE 0xc483

#ifndef _ASMLANGUAGE

#include <misc/util.h>

#ifdef DEBUG
#include <misc/printk.h>
#define PRINTK(...) printk(__VA_ARGS__)
#else
#define PRINTK(...)
#endif /* DEBUG */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The following structure defines the set of 'non-volatile' integer registers.
 * These registers must be preserved by a called C function.  These are the
 * only registers that need to be saved/restored when a cooperative context
 * switch occurs.
 */

typedef struct s_coopReg {
	unsigned long esp;

	/*
	 * The following registers are considered non-volatile, i.e.
	 *callee-save,
	 * but their values are pushed onto the stack rather than stored in the
	 *tCCS
	 * structure:
	 *
	 *  unsigned long ebp;
	 *  unsigned long ebx;
	 *  unsigned long esi;
	 *  unsigned long edi;
	 */

} tCoopReg;

/*
 * The following structure defines the set of 'volatile' integer registers.
 * These registers need not be preserved by a called C function.  Given that
 * they are not preserved across function calls, they must be save/restored
 * (along with the s_coop_reg) when a pre-emptive context switch occurs.
 */

typedef struct s_preempReg {

	/*
	 * The volatile registers 'eax', 'ecx' and 'edx' area not included in
	 *the
	 * definition of 'tPreempReg' since the interrupt stubs
	 *(_IntEnt/_IntExit)
	 * and exception stubs (_ExcEnt/_ExcEnter) use the stack to save and
	 * restore the values of these registers in order to support interrupt
	 * nesting.  The stubs do _not_ copy the saved values from the stack
	 *into
	 * the tCCS.
	 *
	 * unsigned long eax;
	 * unsigned long ecx;
	 * unsigned long edx;
	 */

} tPreempReg;

/*
 * The macro CONFIG_FP_SHARING shall be set to indicate that the
 * saving/restoring of the traditional x87 floating point (and MMX) registers
 * are supported by the nanokernel's context swapping code. The macro
 * CONFIG_SSE shall _also_ be set if saving/restoring of the XMM
 * registers is also supported in the nanokernel's context swapping code.
 */

#ifdef CONFIG_FP_SHARING

/* definition of a single x87 (floating point / MMX) register */

typedef struct s_FpReg {
	unsigned char reg[10]; /* 80 bits: ST[0-7] */
} tFpReg;

/*
 * The following is the "normal" floating point register save area, or
 * more accurately the save area required by the 'fnsave' and 'frstor'
 * instructions.  The structure matches the layout described in the
 * "Intel® 64 and IA-32 Architectures Software Developer’s Manual
 * Volume 1: Basic Architecture": Protected Mode x87 FPU State Image in
 * Memory, 32-Bit Format.
 */

typedef struct s_FpRegSet {  /* # of bytes: name of register */
	unsigned short fcw;      /* 2  : x87 FPU control word */
	unsigned short pad1;     /* 2  : N/A */
	unsigned short fsw;      /* 2  : x87 FPU status word */
	unsigned short pad2;     /* 2  : N/A */
	unsigned short ftw;      /* 2  : x87 FPU tag word */
	unsigned short pad3;     /* 2  : N/A */
	unsigned int fpuip;      /* 4  : x87 FPU instruction pointer offset */
	unsigned short cs;       /* 2  : x87 FPU instruction pointer selector */
	unsigned short fop : 11; /* 2  : x87 FPU opcode */
	unsigned short pad4 : 5; /*    : 5 bits = 00000 */
	unsigned int fpudp;      /* 4  : x87 FPU instr operand ptr offset */
	unsigned short ds;       /* 2  : x87 FPU instr operand ptr selector */
	unsigned short pad5;     /* 2  : N/A */
	tFpReg fpReg[8];	 /* 80 : ST0 -> ST7 */
} tFpRegSet __aligned(FP_REG_SET_ALIGN);

#ifdef CONFIG_SSE

/* definition of a single x87 (floating point / MMX) register */

typedef struct s_FpRegEx {
	unsigned char reg[10];  /* 80 bits: ST[0-7] or MM[0-7] */
	unsigned char rsrvd[6]; /* 48 bits: reserved */
} tFpRegEx;

/* definition of a single XMM register */

typedef struct s_XmmReg {
	unsigned char reg[16]; /* 128 bits: XMM[0-7] */
} tXmmReg;

/*
 * The following is the "extended" floating point register save area, or
 * more accurately the save area required by the 'fxsave' and 'fxrstor'
 * instructions.  The structure matches the layout described in the
 * "Intel 64 and IA-32 Architectures Software Developer's Manual
 * Volume 2A: Instruction Set Reference, A-M", except for the bytes from offset
 * 464 to 511 since these "are available to software use. The processor does
 * not write to bytes 464:511 of an FXSAVE area".
 *
 * This structure must be aligned on a 16 byte boundary when the instructions
 * fxsave/fxrstor are used to write/read the data to/from the structure.
 */

typedef struct s_FpRegSetEx /* # of bytes: name of register */
{
	unsigned short fcw;     /* 2  : x87 FPU control word */
	unsigned short fsw;     /* 2  : x87 FPU status word */
	unsigned char ftw;      /* 1  : x87 FPU abridged tag word */
	unsigned char rsrvd0;   /* 1  : reserved */
	unsigned short fop;     /* 2  : x87 FPU opcode */
	unsigned int fpuip;     /* 4  : x87 FPU instruction pointer offset */
	unsigned short cs;      /* 2  : x87 FPU instruction pointer selector */
	unsigned short rsrvd1;  /* 2  : reserved */
	unsigned int fpudp;     /* 4  : x87 FPU instr operand ptr offset */
	unsigned short ds;      /* 2  : x87 FPU instr operand ptr selector */
	unsigned short rsrvd2;  /* 2  : reserved */
	unsigned int mxcsr;     /* 4  : MXCSR register state */
	unsigned int mxcsrMask; /* 4  : MXCSR register mask */
	tFpRegEx fpReg[8];      /* 128 : x87 FPU/MMX registers */
	tXmmReg xmmReg[8];      /* 128 : XMM registers */
	unsigned char rsrvd3[176]; /* 176 : reserved */
} tFpRegSetEx __aligned(FP_REG_SET_ALIGN);

#else /* CONFIG_SSE == 0 */

typedef struct s_FpRegSetEx {
} tFpRegSetEx;

#endif /* CONFIG_SSE == 0 */

#else /* CONFIG_FP_SHARING == 0 */

/* empty floating point register definition */

typedef struct s_FpRegSet {
} tFpRegSet;

typedef struct s_FpRegSetEx {
} tFpRegSetEx;

#endif /* CONFIG_FP_SHARING == 0 */

/*
 * The following structure defines the set of 'non-volatile' x87 FPU/MMX/SSE
 * registers. These registers must be preserved by a called C function.
 * These are the only registers that need to be saved/restored when a
 * cooperative context switch occurs.
 */

typedef struct s_coopFloatReg {

	/*
	 * This structure intentionally left blank, i.e. the ST[0] -> ST[7] and
	 * XMM0 -> XMM7 registers are all 'volatile'.
	 */

} tCoopFloatReg;

/*
 * The following structure defines the set of 'volatile' x87 FPU/MMX/SSE
 * registers.  Thes registers need not be preserved by a called C function.
 * Given that they are not preserved across function calls, they must be
 * save/restored (along with s_coopFloatReg) when a pre-emptive context
 * switch occurs.
 */

typedef struct s_preempFloatReg {
	union {
		tFpRegSet fpRegs; /* contexts with USE_FP utilize this format */
		tFpRegSetEx fpRegsEx; /* contexts with USE_SSE utilize this
					 format */
	} floatRegsUnion;
} tPreempFloatReg;

/*
 * The context control stucture definition.  It contains the
 * various fields to manage a _single_ context. The CCS will be aligned
 * to the appropriate architecture specific boundary via the
 * _NewContext() call.
 */

struct ccs {
	/*
	 * Link to next context in singly-linked context list (such as
	 * prioritized
	 * list of runnable fibers, or list of fibers waiting on a nanokernel
	 * FIFO).
	 */

	struct ccs *link;

	/*
	 * See the above flag definitions above for valid bit settings.  This
	 * field must remain near the start of the tCCS structure, specifically
	 * before any #ifdef'ed fields since the host tools currently use a
	 * fixed
	 * offset to read the 'flags' field.
	 */

	int flags;

	/*
	 * Storage space for integer registers.  These must also remain near
	 * the start of the tCCS structure for the same reason mention for
	 * 'flags'.
	 */

	tCoopReg coopReg;     /* non-volatile integer register storage */
	tPreempReg preempReg; /* volatile integer register storage */

#if defined(CONFIG_CONTEXT_MONITOR)
	struct ccs *next_context; /* next item in list of ALL fiber+tasks */
#endif
#ifdef CONFIG_GDB_INFO
	void *esfPtr; /* pointer to exception stack frame saved by */
		      /* outermost exception wrapper */
#endif		      /* CONFIG_GDB_INFO */
	int prio;     /* context priority used to sort linked list */
#if (defined(CONFIG_FP_SHARING) || defined(CONFIG_GDB_INFO))
	/*
	 * Nested exception count to maintain setting of EXC_ACTIVE flag across
	 * outermost exception.  EXC_ACTIVE is used by _Swap() lazy FP
	 * save/restore
	 * and by debug tools.
	 */
	unsigned excNestCount; /* nested exception count */
#endif /* CONFIG_FP_SHARING || CONFIG_GDB_INFO */

#ifdef CONFIG_CONTEXT_CUSTOM_DATA
	void *custom_data;     /* available for custom use */
#endif

#ifdef CONFIG_NANO_TIMEOUTS
	struct _nano_timeout nano_timeout;
#endif

	/*
	 * The location of all floating point related structures/fields MUST be
	 * located at the end of the tCCS structure.  This way only the
	 *fibers/tasks
	 * that actually utilize non-integer capabilities need to account for
	 * the increased memory required for storing FP state when sizing
	 *stacks.
	 *
	 * Given that stacks "grow down" on IA-32, and the tCCS structure is
	 *located
	 * at the start of a context's "workspace" memory, the stacks of
	 *fibers/tasks
	 * that do not utilize floating point instruction can effectively
	 *consume
	 * the memory occupied by the 'tCoopFloatReg' and 'tPreempFloatReg'
	 * structures without ill effect.
	 */

	tCoopFloatReg coopFloatReg; /* non-volatile float register storage */
	tPreempFloatReg preempFloatReg; /* volatile float register storage */
};

/*
 * The nanokernel structure definition.  It contains various fields to
 * manage _all_ the contexts in the nanokernel (system level).
 */

typedef struct s_NANO {
	tCCS *fiber;   /* singly linked list of runnable fiber contexts */
	tCCS *task;    /* pointer to runnable task context */
	tCCS *current; /* currently scheduled context (fiber or task) */
#if defined(CONFIG_CONTEXT_MONITOR)
	tCCS *contexts; /* singly linked list of ALL fiber+tasks */
#endif
	unsigned nested;  /* nested interrupt count */
	char *common_isp; /* interrupt stack pointer base */

#ifdef CONFIG_ADVANCED_POWER_MANAGEMENT
	int32_t idle; /* Number of ticks for kernel idling */
#endif		      /* CONFIG_ADVANCED_POWER_MANAGEMENT */


#ifdef CONFIG_FP_SHARING
	/*
	 * A 'current_sse' field does not exist in addition to the 'current_fp'
	 * field since it's not possible to divide the IA-32 non-integer
	 * registers
	 * into 2 distinct blocks owned by differing contexts.  In other words,
	 * given that the 'fxnsave/fxrstor' instructions save/restore both the
	 * X87 FPU and XMM registers, it's not possible for a context to only
	 * "own" the XMM registers.
	 */

	tCCS *current_fp; /* context (fiber or task) that owns the FP regs */
#endif			  /* CONFIG_FP_SHARING */
#ifdef CONFIG_NANO_TIMEOUTS
	sys_dlist_t timeout_q;
#endif
} tNANO;

/* stack alignment related macros: STACK_ALIGN_SIZE is defined above */

#define STACK_ROUND_UP(x) ROUND_UP(x, STACK_ALIGN_SIZE)
#define STACK_ROUND_DOWN(x) ROUND_DOWN(x, STACK_ALIGN_SIZE)

/* variable declarations */

/*
 * There is only a single instance of the s_NANO structure, given that there
 * is only a single nanokernel in the system: _nanokernel
 */

extern tNANO _nanokernel;

/* inline function definitions */

/**
 *
 * @brief Performs architecture-specific initialization
 *
 * This routine performs architecture-specific initialization of the nanokernel.
 * Trivial stuff is done inline; more complex initialization is done via
 * function calls.
 *
 * @return N/A
 *
 * \NOMANUAL
 */

static inline void nanoArchInit(void)
{
	extern void *__isr___SpuriousIntHandler;
	extern void *_dummy_spurious_interrupt;
	extern void _ExcEnt(void);
	extern void *_dummy_exception_vector_stub;
	extern char _interrupt_stack[CONFIG_ISR_STACK_SIZE];

	_nanokernel.nested = 0;

#ifdef CONFIG_NO_ISRS
	_nanokernel.common_isp = (char *)NULL;
#else  /* notdef CONFIG_NO_ISRS */
	_nanokernel.common_isp = (char *)STACK_ROUND_DOWN(
		&_interrupt_stack[CONFIG_ISR_STACK_SIZE - 1]);
#endif /* notdef CONFIG_NO_ISRS */

	/*
	 * Forces the inclusion of the spurious interrupt handlers. If a
	 * reference
	 * isn't made then intconnect.o is never pulled in by the linker.
	 */

	_dummy_spurious_interrupt = &__isr___SpuriousIntHandler;

	/*
	 * Forces the inclusion of the exception vector stub code. If a
	 * reference
	 * isn't made then excstubs.o is never pulled in by the linker.
	 */

	_dummy_exception_vector_stub = &_ExcEnt;


}

/**
 *
 * @brief Set the return value for the specified fiber (inline)
 *
 * The register used to store the return value from a function call invocation is
 * set to <value>.  It is assumed that the specified <fiber> is pending, and
 * thus the fibers context is stored in its tCCS structure.
 *
 * @return N/A
 *
 * \NOMANUAL
 */

static inline void fiberRtnValueSet(
	tCCS *fiber,       /* pointer to fiber */
	unsigned int value /* value to set as return value */
	)
{
	/* write into 'eax' slot created in _Swap() entry */

	*(unsigned int *)(fiber->coopReg.esp) = value;
}

/* definitions to support nanoCpuExcConnect() */

#define _EXC_STUB_SIZE 0x14

/*
 * Performance optimization
 *
 * Macro PERF_OPT is defined if project is compiled with option other than
 * size optimization ("-Os" for GCC, "-XO -Xsize-opt" for Diab). If the
 * last of the compiler options is the size optimization, PERF_OPT is not
 * defined and the project is optimized for size, hence the stub should be
 * aligned to 1 and not 16.
 */
#ifdef PERF_OPT
#define _EXC_STUB_ALIGN 16
#else
#define _EXC_STUB_ALIGN 1
#endif

typedef unsigned char __aligned(_EXC_STUB_ALIGN) NANO_EXC_STUB[_EXC_STUB_SIZE];

/*
 * Macro to declare a dynamic exception stub. Using the macro places the stub
 * in the .intStubSection which is located in the image according to the kernel
 * configuration.
 */
#define NANO_CPU_EXC_STUB_DECL(s) _NODATA_SECTION(.intStubSect) NANO_EXC_STUB(s)

/* function prototypes */

extern void nano_cpu_atomic_idle(unsigned int imask);

extern void nanoCpuExcConnect(unsigned int vector,
			      void (*routine)(NANO_ESF * pEsf),
			      NANO_EXC_STUB pExcStubMem);

extern void _IntVecSet(unsigned int vector,
		       void (*routine)(void *),
		       unsigned int dpl);

extern void _MsrWrite(unsigned int msr, uint64_t msrData);
extern uint64_t _MsrRead(unsigned int msr);

/*
 * _IntLibInit() is called from the non-arch specific nanokernel function,
 * _nano_init(). The IA-32 nanokernel does not require any special
 * initialization of the interrupt subsystem. However, we still need to
 * provide an _IntLibInit() of some sort to prevent build errors.
 */
static inline void _IntLibInit(void)
{
}

#include <stddef.h> /* For size_t */

#ifdef __cplusplus
}
#endif

#define _IS_IN_ISR() (_nanokernel.nested != 0)

#endif /* _ASMLANGUAGE */

#endif /* _NANO_PRIVATE_H */
