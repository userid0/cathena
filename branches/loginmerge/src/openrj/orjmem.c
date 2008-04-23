/* /////////////////////////////////////////////////////////////////////////////
 * File:    orjmem.c
 *
 * Purpose: Implementation file of the memory handling for the Open-RJ library
 *
 * Created: 15th June 2004
 * Updated: 1st January 2006
 *
 * Home:    http://openrj.org/
 *
 * Copyright 2004-2005, Matthew Wilson and Synesis Software
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the names of Matthew Wilson and Synesis Software nor the names of
 *   any contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * ////////////////////////////////////////////////////////////////////////// */


/** \file orjmem.c Implementation file of the memory handling for the Open-RJ library
 *
 */

/* /////////////////////////////////////////////////////////////////////////////
 * Version information
 */

#ifndef OPENRJ_DOCUMENTATION_SKIP_SECTION
# define OPENRJ_VER_C_ORJMEM_MAJOR      1
# define OPENRJ_VER_C_ORJMEM_MINOR      1
# define OPENRJ_VER_C_ORJMEM_REVISION   3
# define OPENRJ_VER_C_ORJMEM_EDIT       6
#endif /* !OPENRJ_DOCUMENTATION_SKIP_SECTION */

/* /////////////////////////////////////////////////////////////////////////////
 * Includes
 */

#include <openrj/openrj.h>
/* #include <openrj/openrj_assert.h> */
#include <openrj/openrj_memory.h>

#include <stdlib.h>

/* ////////////////////////////////////////////////////////////////////////// */

ORJ_CALL(void*) openrj_ator_alloc_(IORJAllocator *ator, size_t cb)
{
    return (NULL != ator) ? ator->pfnAlloc(ator, cb) : malloc(cb);
}

ORJ_CALL(void*) openrj_ator_realloc_(IORJAllocator *ator, void *pv, size_t cb)
{
    return (NULL != ator) ? ator->pfnRealloc(ator, pv, cb) : realloc(pv, cb);
}

ORJ_CALL(void) openrj_ator_free_(IORJAllocator *ator, void *pv)
{
    (NULL != ator) ? ator->pfnFree(ator, pv) : free(pv);
}

/* ////////////////////////////////////////////////////////////////////////// */
