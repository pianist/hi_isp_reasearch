/*
 * Minimal AWB algorithm initialization.
 *
 * The sensor registration callback has already copied the complete
 * AWB_SENSOR_DEFAULT_S into this handle's context.  Unlike the original
 * lib_hiawb.so, this stub does not allocate VREGs and does not initialize
 * any adaptive AWB state.  It only validates the sensor binding and builds
 * the constant result that AwbRun() will return.
 */

#include "isp_awb.h"

#include <stdio.h>
#include <string.h>


HI_VOID AwbPrepareDefaultResult(AWB_STUB_CTX_S *pstCtx)
{
    static const HI_U16 s_au16IdentityCcm[9] = {
        0x0100, 0x0000, 0x0000,
        0x0000, 0x0100, 0x0000,
        0x0000, 0x0000, 0x0100
    };

    ISP_AWB_RESULT_S *pstResult;
    HI_BOOL bHasSensorCcm;
    HI_U32 i;

    pstResult = &pstCtx->stResult;
    memset(pstResult, 0, sizeof(*pstResult));

    /* Sensor gain offsets are unsigned Q8; ISP result gains are Q16. */
    for (i = 0; i < 4; ++i) {
        HI_U16 u16Gain = pstCtx->stSensorDefault.au16GainOffset[i];

        /* A zero WB gain would black out that Bayer component. */
        if (u16Gain == 0)
            u16Gain = 0x0100;

        pstResult->au32WhiteBalanceGain[i] = (HI_U32)u16Gain << 8;
    }

    /*
     * The sensor CCM uses the API's direct/sign-magnitude Q8 encoding.
     * Use its high-temperature matrix as the fixed stub matrix.  This is
     * also the matrix selected by the original AwbCmInit() at startup.
     */
    bHasSensorCcm = HI_FALSE;
    for (i = 0; i < 9; ++i) {
        if (pstCtx->stSensorDefault.stCcm.au16HighCCM[i] != 0) {
            bHasSensorCcm = HI_TRUE;
            break;
        }
    }

    if (bHasSensorCcm) {
        memcpy(pstResult->au16ColorMatrix,
               pstCtx->stSensorDefault.stCcm.au16HighCCM,
               sizeof(pstResult->au16ColorMatrix));
    } else {
        memcpy(pstResult->au16ColorMatrix,
               s_au16IdentityCcm,
               sizeof(pstResult->au16ColorMatrix));
    }

    /* Conservative metering window used by the original first-frame init. */
    pstResult->stStatAttr.bChange                       = HI_TRUE;
    pstResult->stStatAttr.u16MeteringWhiteLevelAwb     = 940;
    pstResult->stStatAttr.u16MeteringBlackLevelAwb     = 64;
    pstResult->stStatAttr.u16MeteringCrRefMaxAwb       = 512;
    pstResult->stStatAttr.u16MeteringCrRefMinAwb       = 128;
    pstResult->stStatAttr.u16MeteringCbRefMaxAwb       = 512;
    pstResult->stStatAttr.u16MeteringCbRefMinAwb       = 128;
}


HI_S32 AwbInit(
    HI_S32 s32Handle,
    const ISP_AWB_PARAM_S *pstAwbParam)
{
    AWB_STUB_CTX_S *pstCtx;

    AWB_DEBUG_PRINT(
        "Calling AwbInit with params: handle=%d, pstAwbParam=%p\n",
        s32Handle,
        (const void *)pstAwbParam);

    /* The unsigned cast rejects negative handles as well as handles > 1. */
    if ((HI_U32)s32Handle >= AWB_MAX_HANDLES)
        return HI_FAILURE;

    if (pstAwbParam == HI_NULL)
        return HI_ERR_ISP_NULL_PTR;

    AWB_DEBUG_PRINT("  AwbInit SensorId=%d\n", pstAwbParam->SensorId);

    pstCtx = &g_astAwbStubCtx[s32Handle];

    if (pstCtx->bSensorRegistered == HI_FALSE) {
        printf("Sensor doesn't register to hisi awb(%d)!\n", s32Handle);
        return HI_FAILURE;
    }

    if (pstCtx->SensorId != pstAwbParam->SensorId) {
        printf("Awblib's sensor %d and isp's sensor %d doesn't match!\n",
               pstCtx->SensorId,
               pstAwbParam->SensorId);
        return HI_FAILURE;
    }

    /* Defaults corresponding to AwbExtRegsDefault() in the original code. */
    pstCtx->bWdrMode             = HI_FALSE;
    pstCtx->u32Iso               = 100;
    pstCtx->u32RuntimeParam8005  = 0x0100;
    pstCtx->u32Saturation        = 0x0080;
    pstCtx->enWbType             = OP_TYPE_AUTO;
    pstCtx->enAwbAlgType         = AWB_ALG_ADVANCE;

    /* Defaults written by the original AwbExtRegsDefault(). */
    memset(&pstCtx->stAdvAwbAttr, 0, sizeof(pstCtx->stAdvAwbAttr));
    pstCtx->stAdvAwbAttr.bAccuPrior                 = HI_FALSE;
    pstCtx->stAdvAwbAttr.u8Tolerance                = 6;
    pstCtx->stAdvAwbAttr.u16CurveLLimit             = 224;
    pstCtx->stAdvAwbAttr.u16CurveRLimit             = 304;
    pstCtx->stAdvAwbAttr.bGainNormEn                = HI_FALSE;
    pstCtx->stAdvAwbAttr.stInOrOut.bEnable          = HI_FALSE;
    pstCtx->stAdvAwbAttr.stInOrOut.enOpType         = OP_TYPE_AUTO;
    pstCtx->stAdvAwbAttr.stInOrOut.bOutdoorStatus   = HI_FALSE;
    pstCtx->stAdvAwbAttr.stInOrOut.u32OutThresh     = 128;
    pstCtx->stAdvAwbAttr.stInOrOut.u16LowStart      = 5000;
    pstCtx->stAdvAwbAttr.stInOrOut.u16LowStop       = 4500;
    pstCtx->stAdvAwbAttr.stInOrOut.u16HighStart     = 6500;
    pstCtx->stAdvAwbAttr.stInOrOut.u16HighStop      = 8000;
    pstCtx->stAdvAwbAttr.stInOrOut.bGreenEnhanceEn = HI_FALSE;
    pstCtx->stAdvAwbAttr.stCTLimit.bEnable          = HI_FALSE;
    pstCtx->stAdvAwbAttr.stCTLimit.enOpType         = OP_TYPE_AUTO;
    pstCtx->stAdvAwbAttr.stCTLimit.u16HighRgLimit   = 512;
    pstCtx->stAdvAwbAttr.stCTLimit.u16HighBgLimit   = 256;
    pstCtx->stAdvAwbAttr.stCTLimit.u16LowRgLimit    = 256;
    pstCtx->stAdvAwbAttr.stCTLimit.u16LowBgLimit    = 1024;

    AwbPrepareDefaultResult(pstCtx);

    /* Publish initialized state only after the result is fully prepared. */
    pstCtx->bInitialized = HI_TRUE;

    return HI_SUCCESS;
}
