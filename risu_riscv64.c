/******************************************************************************
 * Copyright (c) 2020 T-Head Semiconductor Co., Ltd.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     LIU Zhiwei(Linaro) - initial implementation
 *     based on Peter Maydell's risu_arm.c
 *****************************************************************************/

#include "risu.h"

void advance_pc(void *vuc)
{
    ucontext_t *uc = vuc;
    uc->uc_mcontext.__gregs[0] += 4;
}

void set_ucontext_paramreg(void *vuc, uint64_t value)
{
    ucontext_t *uc = vuc;
    uc->uc_mcontext.__gregs[10] = value;
}

uint64_t get_reginfo_paramreg(struct reginfo *ri)
{
    return ri->regs[10];
}

int get_risuop(struct reginfo *ri)
{
    /* Return the risuop we have been asked to do
     * (or -1 if this was a SIGILL for a non-risuop insn)
     */
    uint32_t insn = ri->faulting_insn;
    uint32_t op = (insn & 0xf00) >> 8;
    uint32_t key = insn & ~0xf00;
    uint32_t risukey = 0x0000006b;
    return (key != risukey) ? -1 : op;
}

uintptr_t get_pc(struct reginfo *ri)
{
   return ri->pc;
}
