/*
 * Minimal AWB algorithm shutdown.
 *
 * The replacement library owns no VREG mapping and no dynamically allocated
 * algorithm state.  Sensor registration is a separate lifetime and is
 * deliberately left intact until HI_MPI_AWB_SensorUnRegCallBack().
 */

#include "isp_awb.h"

#include <stdio.h>


HI_S32 AwbExit(HI_S32 s32Handle)
{
    AWB_STUB_CTX_S *pstCtx;

    AWB_DEBUG_PRINT("Calling AwbExit with params: handle=%d\n", s32Handle);

    /* The unsigned cast rejects negative handles as well as handles > 1. */
    if ((HI_U32)s32Handle >= AWB_MAX_HANDLES)
        return HI_FAILURE;

    pstCtx = &g_astAwbStubCtx[s32Handle];

    /* Idempotent, like the original function when AWB was not initialized. */
    pstCtx->bInitialized = HI_FALSE;

    return HI_SUCCESS;
}
