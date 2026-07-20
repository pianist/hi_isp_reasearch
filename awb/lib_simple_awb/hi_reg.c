/*
 * Minimal registration layer for the replacement lib_hiawb.so.
 *
 * This module exports the same four public registration functions as the
 * original library.  The actual ISP algorithm registration is still done by
 * the closed ISP framework through HI_MPI_ISP_AwbLibRegCallBack().
 */

#include "isp_awb.h"

#include <stdio.h>
#include <string.h>

/*
 * One small state object per supported AWB handle.  hi_reg.c owns the
 * storage; AwbInit/AwbRun/AwbCtrl/AwbExit access it through the extern
 * declaration in isp_awb.h.
 */
AWB_STUB_CTX_S g_astAwbStubCtx[AWB_MAX_HANDLES];

/*
 * Validate the ABI-level library descriptor used by all public entry points.
 * Casting s32Id to HI_U32 rejects both values >= AWB_MAX_HANDLES and negative
 * values, matching the unsigned range checks seen in the original binary.
 */
static HI_S32 AwbValidateLib(const ALG_LIB_S *pstAwbLib)
{
    if (pstAwbLib == HI_NULL)
        return HI_ERR_ISP_NULL_PTR;

    if ((HI_U32)pstAwbLib->s32Id >= AWB_MAX_HANDLES)
        return HI_FAILURE;

    if (strcmp(pstAwbLib->acLibName, HI_AWB_LIB_NAME) != 0)
        return HI_FAILURE;

    return HI_SUCCESS;
}

/*
 * Register this library's four algorithm callbacks with the ISP framework.
 * The framework copies stRegister during this call, so a stack object is
 * sufficient and no pointer to it is retained by this module.
 */
HI_S32 HI_MPI_AWB_Register(ALG_LIB_S *pstAwbLib)
{
    ISP_AWB_REGISTER_S stRegister;
    HI_S32 s32Ret;

    AWB_DEBUG_PRINT(
        "Calling HI_MPI_AWB_Register with params: pstAwbLib=%p\n",
        (void *)pstAwbLib);

    s32Ret = AwbValidateLib(pstAwbLib);
    if (s32Ret != HI_SUCCESS)
        return s32Ret;

    memset(&stRegister, 0, sizeof(stRegister));

    stRegister.stAwbExpFunc.pfn_awb_init = AwbInit;
    stRegister.stAwbExpFunc.pfn_awb_run  = AwbRun;
    stRegister.stAwbExpFunc.pfn_awb_ctrl = AwbCtrl;
    stRegister.stAwbExpFunc.pfn_awb_exit = AwbExit;

    s32Ret = HI_MPI_ISP_AwbLibRegCallBack(pstAwbLib, &stRegister);
    if (s32Ret != HI_SUCCESS)
        puts("Hi_awb register failed!");

    return s32Ret;
}

/* Remove this library's callback table from the ISP framework. */
HI_S32 HI_MPI_AWB_UnRegister(ALG_LIB_S *pstAwbLib)
{
    HI_S32 s32Ret;

    AWB_DEBUG_PRINT(
        "Calling HI_MPI_AWB_UnRegister with params: pstAwbLib=%p\n",
        (void *)pstAwbLib);

    s32Ret = AwbValidateLib(pstAwbLib);
    if (s32Ret != HI_SUCCESS)
        return s32Ret;

    s32Ret = HI_MPI_ISP_AwbLibUnRegCallBack(pstAwbLib);
    if (s32Ret != HI_SUCCESS)
        puts("Hi_awb unregister failed!");

    return s32Ret;
}

/*
 * Register the sensor-specific AWB defaults provider.
 *
 * As in the original library, the sensor callback is executed immediately.
 * Its returned calibration is kept in the per-handle context and will later
 * be used by the stub AwbRun to produce constant sensor-specific gains/CCM.
 *
 * The original binary does not propagate the callback's return value.  We do
 * the same here: registration describes the callback relationship, while a
 * missing/invalid calibration is handled by AwbRun's unity fallback.
 */
HI_S32 HI_MPI_AWB_SensorRegCallBack(
    ALG_LIB_S *pstAwbLib,
    SENSOR_ID SensorId,
    AWB_SENSOR_REGISTER_S *pstRegister)
{
    AWB_STUB_CTX_S *pstCtx;
    HI_S32 s32Ret;

    AWB_DEBUG_PRINT(
        "Calling HI_MPI_AWB_SensorRegCallBack with params: "
        "pstAwbLib=%p, SensorId=%d, pstRegister=%p\n",
        (void *)pstAwbLib,
        SensorId,
        (void *)pstRegister);

    if (pstRegister == HI_NULL)
        return HI_ERR_ISP_NULL_PTR;

    s32Ret = AwbValidateLib(pstAwbLib);
    if (s32Ret != HI_SUCCESS)
        return s32Ret;

    pstCtx = &g_astAwbStubCtx[pstAwbLib->s32Id];

    /* Do not let calibration from an older sensor registration survive. */
    memset(&pstCtx->stSensorDefault, 0, sizeof(pstCtx->stSensorDefault));

    if (pstRegister->stSnsExp.pfn_cmos_get_awb_default != HI_NULL) {
        (void)pstRegister->stSnsExp.pfn_cmos_get_awb_default(
            &pstCtx->stSensorDefault);
    }

    pstCtx->stSensorRegister = *pstRegister;
    pstCtx->SensorId = SensorId;
    pstCtx->bSensorRegistered = HI_TRUE;

    return HI_SUCCESS;
}

/*
 * Detach the sensor from one AWB handle.
 *
 * SensorId is checked deliberately: silently unregistering another sensor
 * would leave the ISP framework and this library with inconsistent bindings.
 * The initialized flag is not changed here; normal shutdown order is
 * AwbExit() followed by SensorUnRegCallBack().
 */
HI_S32 HI_MPI_AWB_SensorUnRegCallBack(
    ALG_LIB_S *pstAwbLib,
    SENSOR_ID SensorId)
{
    AWB_STUB_CTX_S *pstCtx;
    HI_S32 s32Ret;

    AWB_DEBUG_PRINT(
        "Calling HI_MPI_AWB_SensorUnRegCallBack with params: "
        "pstAwbLib=%p, SensorId=%d\n",
        (void *)pstAwbLib,
        SensorId);

    s32Ret = AwbValidateLib(pstAwbLib);
    if (s32Ret != HI_SUCCESS)
        return s32Ret;

    pstCtx = &g_astAwbStubCtx[pstAwbLib->s32Id];

    if (pstCtx->bSensorRegistered == HI_FALSE)
        return HI_FAILURE;

    if (pstCtx->SensorId != SensorId)
        return HI_FAILURE;

    pstCtx->bSensorRegistered = HI_FALSE;
    pstCtx->SensorId = 0;
    memset(&pstCtx->stSensorRegister, 0, sizeof(pstCtx->stSensorRegister));
    memset(&pstCtx->stSensorDefault, 0, sizeof(pstCtx->stSensorDefault));

    return HI_SUCCESS;
}
