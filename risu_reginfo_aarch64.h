/******************************************************************************
 * Copyright (c) 2013 Linaro Limited
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Claudio Fontana (Linaro) - initial implementation
 *     based on Peter Maydell's risu_arm.c
 *****************************************************************************/

#ifndef RISU_REGINFO_AARCH64_H
#define RISU_REGINFO_AARCH64_H

struct simd_reginfo {
    __uint128_t vregs[32];
    char end[0];
};

struct reginfo {
    uint64_t fault_address;
    uint64_t regs[31];
    uint64_t sp;
    uint64_t pc;
    uint32_t flags;
    uint32_t faulting_insn;

    /* FP/SIMD */
    uint32_t fpsr;
    uint32_t fpcr;

    union {
        struct simd_reginfo simd;
    };
};

#endif /* RISU_REGINFO_AARCH64_H */
