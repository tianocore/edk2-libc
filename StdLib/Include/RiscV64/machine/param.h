/*      $NetBSD: param.h,v 1.13 2010/05/06 19:10:26 joerg Exp $ */

/*
 * Copyright (c) 1994,1995 Mark Brinicombe.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the RiscBSD team.
 * 4. The name "RiscBSD" nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY RISCBSD ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL RISCBSD OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _RISCV64_PARAM_H_
#define _RISCV64_PARAM_H_

#undef _MACHINE
#undef MACHINE
#undef _MACHINE_ARCH
#undef MACHINE_ARCH
#define _MACHINE        riscv64
#define MACHINE         "riscv64"
#define _MACHINE_ARCH   riscv64
#define MACHINE_ARCH    "riscv64"

#define MID_MACHINE     MID_RISCV

/*
 * Round p (pointer or byte index) up to a correctly-aligned value
 * for all data types (int, long, ...).   The result is u_int and
 * must be cast to any desired pointer type.
 *
 * ALIGNED_POINTER is a boolean macro that checks whether an address
 * is valid to fetch data elements of type t from on this architecture.
 * This does not reflect the optimal alignment, just the possibility
 * (within reasonable limits).
 *
 */
#define ALIGNBYTES              (sizeof(int) - 1)
#define ALIGN(p)                (((u_int)(p) + ALIGNBYTES) &~ ALIGNBYTES)
#define ALIGNED_POINTER(p,t)    ((((u_long)(p)) & (sizeof(t)-1)) == 0)
/* RISC-V specific macro to align a stack pointer (downwards). */
#define STACKALIGNBYTES		(16UL - 1)
#define STACKALIGN(p)           ((u_int)(p) &~ STACKALIGNBYTES)

#define DEV_BSHIFT      9               /* log2(DEV_BSIZE) */
#define DEV_BSIZE       (1 << DEV_BSHIFT)
#define BLKDEV_IOSIZE   2048

#ifndef MAXPHYS
#define MAXPHYS         65536           /* max I/O transfer size */
#endif

/*
 * Constants related to network buffer management.
 * MCLBYTES must be no larger than NBPG (the software page size), and,
 * on machines that exchange pages of input or output buffers with mbuf
 * clusters (MAPPED_MBUFS), MCLBYTES must also be an integral multiple
 * of the hardware page size.
 */
#define MSIZE           256             /* size of an mbuf */

#ifndef MCLSHIFT
#define MCLSHIFT        11              /* convert bytes to m_buf clusters */
                                        /* 2K cluster can hold Ether frame */
#endif  /* MCLSHIFT */

#define MCLBYTES        (1 << MCLSHIFT) /* size of a m_buf cluster */

#ifndef NMBCLUSTERS_MAX
#define NMBCLUSTERS_MAX (0x2000000 / MCLBYTES)  /* Limit to 64MB for clusters */
#endif

#endif /* _RISCV64_PARAM_H_ */
