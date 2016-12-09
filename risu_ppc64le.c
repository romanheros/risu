/******************************************************************************
 * Copyright (c) IBM Corp, 2016
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Jose Ricardo Ziviani - initial implementation
 *     based on Claudio Fontana's risu_aarch64.c
 *     based on Peter Maydell's risu_arm.c
 *****************************************************************************/

#include <stdio.h>
#include <ucontext.h>
#include <string.h>

#include "risu.h"
#include "risu_reginfo_ppc64le.h"

struct reginfo master_ri, apprentice_ri;
static int mem_used = 0;
static int packet_mismatch = 0;

uint8_t apprentice_memblock[MEMBLOCKLEN];

void advance_pc(void *vuc)
{
    ucontext_t *uc = (ucontext_t*)vuc;
    uc->uc_mcontext.regs->nip += 4;
}

void set_x0(void *vuc, uint64_t x0)
{
    ucontext_t *uc = vuc;
    uc->uc_mcontext.gp_regs[0] = x0;
}

static int get_risuop(uint32_t insn)
{
    uint32_t op = insn & 0xf;
    uint32_t key = insn & ~0xf;
    uint32_t risukey = 0x00005af0;
    return (key != risukey) ? -1 : op;
}

int send_register_info(write_fn write_fn, void *uc)
{
    struct reginfo ri;
    trace_header_t header;
    int op, r = 0;

    reginfo_init(&ri, uc);
    op = get_risuop(ri.faulting_insn);

    /* Write a header with PC/op to keep in sync */
    header.pc = ri.nip;
    header.risu_op = op;
    if (write_fn(&header, sizeof(header)) != 0) {
       fprintf(stderr,"%s: failed header write\n", __func__);
       return -1;
    }

    switch (op) {
    case OP_TESTEND:
       if (write_fn(&ri, sizeof(ri)) != 0) {
          fprintf(stderr,"%s: failed last write\n", __func__);
       }
       r = 1;
       break;
    case OP_SETMEMBLOCK:
        memblock = (void*)ri.gregs[0];
        break;
    case OP_GETMEMBLOCK:
        set_x0(uc, ri.gregs[0] + (uintptr_t)memblock);
        break;
    case OP_COMPAREMEM:
        return write_fn(memblock, MEMBLOCKLEN);
        break;
    case OP_COMPARE:
    default:
        return write_fn(&ri, sizeof(ri));
    }
    return r;
}

/* Read register info from the socket and compare it with that from the
 * ucontext. Return 0 for match, 1 for end-of-test, 2 for mismatch.
 * NB: called from a signal handler.
 */
int recv_and_compare_register_info(read_fn read_fn, respond_fn resp_fn, void *uc)
{
   int resp = 0;
   int op;
   trace_header_t header;

   reginfo_init(&master_ri, uc);
   op = get_risuop(master_ri.faulting_insn);

   if (read_fn(&header, sizeof(header)) != 0) {
      fprintf(stderr,"%s: failed header read\n", __func__);
      return -1;
   }

   if (header.risu_op == op ) {

      /* send OK for the header */
      resp_fn(0);

      switch (op) {
         case OP_COMPARE:
         case OP_TESTEND:
         default:
            if (read_fn(&apprentice_ri, sizeof(apprentice_ri))) {
               packet_mismatch = 1;
               resp = 2;
            } else if (!reginfo_is_eq(&master_ri, &apprentice_ri, uc)) {
               resp = 2;
            }
            else if (op == OP_TESTEND) {
               resp = 1;
            }
            resp_fn(resp);
            break;
         case OP_SETMEMBLOCK:
            memblock = (void*)master_ri.gregs[0];
            break;
         case OP_GETMEMBLOCK:
            set_x0(uc, master_ri.gregs[0] + (uintptr_t)memblock);
            break;
         case OP_COMPAREMEM:
            mem_used = 1;
            if (read_fn(apprentice_memblock, MEMBLOCKLEN)) {
               packet_mismatch = 1;
               resp = 2;
            } else if (memcmp(memblock, apprentice_memblock, MEMBLOCKLEN) != 0) {
               resp = 2;
            }
            resp_fn(resp);
            break;
      }
   } else {
      fprintf(stderr, "out of sync %lx/%lx %d/%d\n",
              master_ri.nip, header.pc,
              op, header.risu_op);
      resp = 2;
      resp_fn(resp);
   }

   return resp;
}

/* Print a useful report on the status of the last comparison
 * done in recv_and_compare_register_info(). This is called on
 * exit, so need not restrict itself to signal-safe functions.
 * Should return 0 if it was a good match (ie end of test)
 * and 1 for a mismatch.
 */
int report_match_status(void)
{
    int resp = 0;
    fprintf(stderr, "match status...\n");

    if (packet_mismatch) {
        fprintf(stderr, "packet mismatch (probably disagreement "
                "about UNDEF on load/store)\n");
        fprintf(stderr, "master reginfo:\n");
        reginfo_dump(&master_ri, 0);
    }
    if (!reginfo_is_eq(&master_ri, &apprentice_ri, NULL)) {
        fprintf(stderr, "mismatch on regs!\n");
        resp = 1;
    }
    if (mem_used && memcmp(memblock, &apprentice_memblock, MEMBLOCKLEN) != 0) {
        fprintf(stderr, "mismatch on memory!\n");
        resp = 1;
    }
    if (!resp) {
        fprintf(stderr, "match!\n");
        return 0;
    }

    fprintf(stderr, "master reginfo:\n");
    reginfo_dump(&master_ri, 1);

    fprintf(stderr, "apprentice reginfo:\n");
    reginfo_dump(&apprentice_ri, 0);

    reginfo_dump_mismatch(&master_ri, &apprentice_ri, stderr);
    return resp;
}
