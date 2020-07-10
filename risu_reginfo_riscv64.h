/******************************************************************************
 * Copyright (c) 2020 T-Head Semiconductor Co., Ltd.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     LIU Zhiwei(T-Head) - initial implementation
 *     based on Peter Maydell's risu_arm.c
 *****************************************************************************/

#ifndef RISU_REGINFO_RISCV64_H
#define RISU_REGINFO_RISCV64_H

struct reginfo {
    uint64_t fault_address;
    uint64_t regs[32];
    uint64_t fregs[32];
    uint64_t pc;
    uint32_t flags;
    uint32_t faulting_insn;

    /* FP */
    uint32_t fcsr;
};

#endif /* RISU_REGINFO_RISCV64_H */
