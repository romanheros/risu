/*****************************************************************************
 * Copyright (c) 2020 T-Head Semiconductor Co., Ltd.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     LIU Zhiwei (T-Head) - initial implementation
 *     based on test_arm.s by Peter Maydell
 *****************************************************************************/

/* Initialise the gp regs */
li x1, 1
#li x2, 2  # stack pointer
#li x3, 3  # global pointer
#li x4, 4  # thread pointer
li x5, 5
li x6, 6
li x7, 7
li x8, 8
li x9, 9
li x10, 10
li x11, 11
li x12, 12
li x13, 13
li x14, 14
li x15, 15
li x16, 16
li x17, 17
li x18, 18
li x19, 19
li x20, 20
li x21, 21
li x22, 22
li x23, 23
li x24, 24
li x25, 25
li x26, 26
li x27, 27
li x28, 28
li x29, 29
li x30, 30
li x31, 30

/* Initialise the fp regs */
fcvt.d.lu f0, x0
fcvt.d.lu f1, x1
fcvt.d.lu f2, x2
fcvt.d.lu f3, x3
fcvt.d.lu f4, x4
fcvt.d.lu f5, x5
fcvt.d.lu f6, x6
fcvt.d.lu f7, x7
fcvt.d.lu f8, x8
fcvt.d.lu f9, x9
fcvt.d.lu f10, x10
fcvt.d.lu f11, x11
fcvt.d.lu f12, x12
fcvt.d.lu f13, x13
fcvt.d.lu f14, x14
fcvt.d.lu f15, x15
fcvt.d.lu f16, x16
fcvt.d.lu f17, x17
fcvt.d.lu f18, x18
fcvt.d.lu f19, x19
fcvt.d.lu f20, x20
fcvt.d.lu f21, x21
fcvt.d.lu f22, x22
fcvt.d.lu f23, x23
fcvt.d.lu f24, x24
fcvt.d.lu f25, x25
fcvt.d.lu f26, x26
fcvt.d.lu f27, x27
fcvt.d.lu f28, x28
fcvt.d.lu f29, x29
fcvt.d.lu f30, x30
fcvt.d.lu f31, x31

/* do compare.
 * The manual says instr with bits (6:0) == 1 1 0 1 0 1 1 are UNALLOCATED
 */
.int 0x0000006b
/* exit test */
.int 0x0000016b
