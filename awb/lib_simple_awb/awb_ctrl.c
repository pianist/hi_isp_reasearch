/*
 * Minimal AWB control callback.
 *
 * The closed ISP framework uses this entry point both for public AWB
 * controls and for private runtime notifications (WDR mode, ISO and the
 * value carried by command 8005).  The stub stores values that may be useful
 * for diagnostics, but it deliberately performs no adaptive calculation.
 */

#include "isp_awb.h"

#include <stdio.h>
#include <string.h>


HI_S32 AwbCtrl(
    HI_S32 s32Handle,
    HI_U32 u32Cmd,
    HI_VOID *pValue)
{
    AWB_STUB_CTX_S *pstCtx;
    HI_U32 u32Value;

/*
    AWB_DEBUG_PRINT(
        "Calling ioctl %u with params: handle=%d, pValue=%p\n",
        u32Cmd,
        s32Handle,
        pValue);*/

    if ((HI_U32)s32Handle >= AWB_MAX_HANDLES)
        return HI_FAILURE;

    pstCtx = &g_astAwbStubCtx[s32Handle];
    if (pstCtx->bInitialized == HI_FALSE)
        return HI_FAILURE;

    /* The original AwbCtrlCmd rejects NULL for every command. */
    if (pValue == HI_NULL)
        return HI_ERR_ISP_NULL_PTR;

    switch (u32Cmd) {
    case AWB_SATURATION_SET:
        u32Value = *(const HI_U32 *)pValue;
        pstCtx->u32Saturation = u32Value & 0xFFu;
/*        AWB_DEBUG_PRINT(
            "  AWB_SATURATION_SET: saturation=%u (stored=%u)\n",
            u32Value,
            pstCtx->u32Saturation);*/
        return HI_SUCCESS;

    case AWB_SATURATION_GET:
        *(HI_U32 *)pValue = pstCtx->u32Saturation;
/*        AWB_DEBUG_PRINT(
            "  AWB_SATURATION_GET: saturation=%u\n",
            pstCtx->u32Saturation);*/
        return HI_SUCCESS;

    case AWB_DEBUG_ATTR_SET:
        /* Debug-buffer mapping is intentionally unsupported by the stub. */
/*        AWB_DEBUG_PRINT(
            "  AWB_DEBUG_ATTR_SET: ignored, firstWord=0x%08x\n",
            *(const HI_U32 *)pValue);*/
        return HI_SUCCESS;

    case AWB_DEBUG_ATTR_GET:
        /* Original AwbDbgGet writes enable, physical address and depth. */
        ((HI_U32 *)pValue)[0] = 0;
        ((HI_U32 *)pValue)[1] = 0;
        ((HI_U32 *)pValue)[2] = 0;
/*        AWB_DEBUG_PRINT(
            "  AWB_DEBUG_ATTR_GET: enable=0, phyAddr=0, depth=0\n");*/
        return HI_SUCCESS;

    case ISP_WDR_MODE_SET:
        u32Value = *(const HI_U32 *)pValue;
        pstCtx->bWdrMode = (u32Value != 0) ? HI_TRUE : HI_FALSE;
/*        AWB_DEBUG_PRINT(
            "  ISP_WDR_MODE_SET: mode=%u, bWdrMode=%d\n",
            u32Value,
            pstCtx->bWdrMode);*/

        /*
         * Sensor drivers commonly return mode-dependent defaults.  Refresh
         * them on a WDR notification, just as the original AwbSetWdrMode()
         * does.  A failed refresh leaves the last valid defaults intact.
         */
        if (pstCtx->stSensorRegister.stSnsExp.pfn_cmos_get_awb_default !=
            HI_NULL) {
            AWB_SENSOR_DEFAULT_S stNewDefault;
            HI_S32 s32Ret;

            memset(&stNewDefault, 0, sizeof(stNewDefault));
            s32Ret = pstCtx->stSensorRegister.stSnsExp
                         .pfn_cmos_get_awb_default(&stNewDefault);

            AWB_DEBUG_PRINT(
                "  sensor get_awb_default returned %d\n",
                s32Ret);

            if (s32Ret == HI_SUCCESS)
                pstCtx->stSensorDefault = stNewDefault;
        }

        AwbPrepareDefaultResult(pstCtx);
        return HI_SUCCESS;

    case ISP_PROC_WRITE:
        /* No private algorithm state is available for a proc dump. */
//        AWB_DEBUG_PRINT("  ISP_PROC_WRITE: ignored by static AWB stub\n");
        return HI_SUCCESS;

    case ISP_AWB_ISO_SET:
        u32Value = *(const HI_U32 *)pValue;
        pstCtx->u32Iso = u32Value;
//        AWB_DEBUG_PRINT("  ISP_AWB_ISO_SET: ISO=%u\n", u32Value);
        return HI_SUCCESS;

    case ISP_AWB_RUNTIME_PARAM_SET:
        u32Value = *(const HI_U32 *)pValue;
        pstCtx->u32RuntimeParam8005 = u32Value;
/*        AWB_DEBUG_PRINT(
            "  ISP_AWB_RUNTIME_PARAM_SET: value=%u (0x%08x)\n",
            u32Value,
            u32Value);*/
        return HI_SUCCESS;

    default:
        /* The original AwbCtrlCmd also returns success for unknown commands. */
        AWB_DEBUG_PRINT("  unknown AWB ioctl %u: ignored\n", u32Cmd);
        return HI_SUCCESS;
    }
}
