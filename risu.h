/*******************************************************************************
 * Copyright (c) 2010 Linaro Limited
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Peter Maydell (Linaro) - initial implementation
 ******************************************************************************/

#ifndef RISU_H
#define RISU_H

#include <inttypes.h>
#include <stdint.h>
#include <ucontext.h>
#include <stdio.h>

/* GCC computed include to pull in the correct risu_reginfo_*.h for
 * the architecture.
 */
#define REGINFO_HEADER2(X) #X
#define REGINFO_HEADER1(ARCHNAME) REGINFO_HEADER2(risu_reginfo_ ## ARCHNAME.h)
#define REGINFO_HEADER(ARCH) REGINFO_HEADER1(ARCH)

#include REGINFO_HEADER(ARCH)

/* Socket related routines */
int master_connect(int port);
int apprentice_connect(const char *hostname, int port);
int send_data_pkt(int sock, void *pkt, int pktlen);
int recv_data_pkt(int sock, void *pkt, int pktlen);
void send_response_byte(int sock, int resp);

extern uintptr_t image_start_address;
extern void *memblock;

extern int test_fp_exc;

/* Ops code under test can request from risu: */
#define OP_COMPARE 0
#define OP_TESTEND 1
#define OP_SETMEMBLOCK 2
#define OP_GETMEMBLOCK 3
#define OP_COMPAREMEM 4

/* The memory block should be this long */
#define MEMBLOCKLEN 8192

struct reginfo;

/* Functions operating on reginfo */

/* Send the register information from the struct ucontext down the socket.
 * Return the response code from the master.
 * NB: called from a signal handler.
 */
int send_register_info(int sock, void *uc);

/* Read register info from the socket and compare it with that from the
 * ucontext. Return 0 for match, 1 for end-of-test, 2 for mismatch.
 * NB: called from a signal handler.
 */
int recv_and_compare_register_info(int sock, void *uc);

/* Print a useful report on the status of the last comparison
 * done in recv_and_compare_register_info(). This is called on
 * exit, so need not restrict itself to signal-safe functions.
 * Should return 0 if it was a good match (ie end of test)
 * and 1 for a mismatch.
 */
int report_match_status(void);

/* Interface provided by CPU-specific code: */

/* Move the PC past this faulting insn by adjusting ucontext
 */
void advance_pc(void *uc);

/* Set the parameter register in a ucontext_t to the specified value.
 * (32-bit targets can ignore high 32 bits.)
 * vuc is a ucontext_t* cast to void*.
 */
void set_ucontext_paramreg(void *vuc, uint64_t value);

/* Return the value of the parameter register from a reginfo. */
uint64_t get_reginfo_paramreg(struct reginfo *ri);

/* Return the risu operation number we have been asked to do,
 * or -1 if this was a SIGILL for a non-risuop insn.
 */
int get_risuop(struct reginfo *ri);

/* initialize structure from a ucontext */
void reginfo_init(struct reginfo *ri, ucontext_t *uc);

/* return 1 if structs are equal, 0 otherwise. */
int reginfo_is_eq(struct reginfo *r1, struct reginfo *r2);

/* print reginfo state to a stream, returns 1 on success, 0 on failure */
int reginfo_dump(struct reginfo *ri, FILE * f);

/* reginfo_dump_mismatch: print mismatch details to a stream, ret nonzero=ok */
int reginfo_dump_mismatch(struct reginfo *m, struct reginfo *a, FILE *f);

#endif /* RISU_H */
