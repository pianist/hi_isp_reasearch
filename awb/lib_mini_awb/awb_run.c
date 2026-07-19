/*
 * Static AWB run callback.
 *
 * This replacement deliberately ignores the image statistics and returns
 * the sensor-specific constant gains/CCM prepared by AwbInit().  The input
 * objects are nevertheless validated according to the original callback
 * ABI, which helps expose integration errors instead of hiding them.
 */

#include "isp_awb.h"

#include <stdio.h>


HI_S32 AwbRun(
    HI_S32 s32Handle,
    const ISP_AWB_INFO_S *pstAwbInfo,
    ISP_AWB_RESULT_S *pstAwbResult,
    HI_S32 s32Reserved)
{
    AWB_STUB_CTX_S *pstCtx;

    AWB_DEBUG_PRINT(
        "Calling AwbRun with params: handle=%d, pstAwbInfo=%p, "
        "pstAwbResult=%p, reserved=%d\n",
        s32Handle,
        (const void *)pstAwbInfo,
        (void *)pstAwbResult,
        s32Reserved);

    /* Remains intentionally unused when verbose tracing is compiled out. */
    (void)s32Reserved;

    if ((HI_U32)s32Handle >= AWB_MAX_HANDLES)
        return HI_FAILURE;

    if (pstAwbInfo == HI_NULL ||
        pstAwbResult == HI_NULL ||
        pstAwbInfo->pstAwbStat1 == HI_NULL ||
        pstAwbInfo->pstAwbStat2 == HI_NULL) {
        return HI_ERR_ISP_NULL_PTR;
    }

    pstCtx = &g_astAwbStubCtx[s32Handle];
    if (pstCtx->bInitialized == HI_FALSE)
        return HI_FAILURE;

    AWB_DEBUG_PRINT(
        "  input: frame=%u, stat1=%p, stat2=%p\n",
        pstAwbInfo->u32FrameCnt,
        (void *)pstAwbInfo->pstAwbStat1,
        (void *)pstAwbInfo->pstAwbStat2);

    AWB_DEBUG_PRINT(
        "  global AWB statistics: G/R=0x%03x, G/B=0x%03x, "
        "whitePoints=%u\n",
        pstAwbInfo->pstAwbStat1->u16RgRatio,
        pstAwbInfo->pstAwbStat1->u16BgRatio,
        pstAwbInfo->pstAwbStat1->u32WhitePointCount);

    /* Structure assignment copies the complete 0x34-byte ARM32 result. */
    *pstAwbResult = pstCtx->stResult;

    AWB_DEBUG_PRINT(
        "  output WB gains Q16: R=0x%08x, Gr=0x%08x, "
        "Gb=0x%08x, B=0x%08x\n",
        pstAwbResult->au32WhiteBalanceGain[0],
        pstAwbResult->au32WhiteBalanceGain[1],
        pstAwbResult->au32WhiteBalanceGain[2],
        pstAwbResult->au32WhiteBalanceGain[3]);

    AWB_DEBUG_PRINT(
        "  output CCM: "
        "[%04x %04x %04x] [%04x %04x %04x] [%04x %04x %04x]\n",
        pstAwbResult->au16ColorMatrix[0],
        pstAwbResult->au16ColorMatrix[1],
        pstAwbResult->au16ColorMatrix[2],
        pstAwbResult->au16ColorMatrix[3],
        pstAwbResult->au16ColorMatrix[4],
        pstAwbResult->au16ColorMatrix[5],
        pstAwbResult->au16ColorMatrix[6],
        pstAwbResult->au16ColorMatrix[7],
        pstAwbResult->au16ColorMatrix[8]);

    AWB_DEBUG_PRINT(
        "  output statistics config: change=%d, white=%u, black=%u, "
        "Cr=[%u..%u], Cb=[%u..%u]\n",
        pstAwbResult->stStatAttr.bChange,
        pstAwbResult->stStatAttr.u16MeteringWhiteLevelAwb,
        pstAwbResult->stStatAttr.u16MeteringBlackLevelAwb,
        pstAwbResult->stStatAttr.u16MeteringCrRefMinAwb,
        pstAwbResult->stStatAttr.u16MeteringCrRefMaxAwb,
        pstAwbResult->stStatAttr.u16MeteringCbRefMinAwb,
        pstAwbResult->stStatAttr.u16MeteringCbRefMaxAwb);

    /*
     * Statistics limits only need to be reprogrammed after initialization
     * or a sensor-default/WDR refresh.  Gains and CCM remain unchanged and
     * are returned on every frame.
     */
    pstCtx->stResult.stStatAttr.bChange = HI_FALSE;

    return HI_SUCCESS;
}
