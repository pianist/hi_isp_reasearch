/*
 * Minimal AWB run callback.
 *
 * Global statistics produce dynamic WB gains while CCM remains fixed.  A
 * small first-order low-pass filter prevents frame-to-frame ratio noise and
 * the resulting feedback loop from becoming visible as color flicker.
 */

#include "isp_awb.h"

#include <stdio.h>
#include <stdlib.h>

static HI_U32 AwbFilterGain(HI_U32 u32Previous, HI_U32 u32Target)
{
    HI_U32 u32Delta;

    /* IIR alpha = 1/16, implemented without signed arithmetic. */
    if (u32Target >= u32Previous) {
        u32Delta = u32Target - u32Previous;
        return u32Previous + ((u32Delta + 8u) >> 4);
    }

    u32Delta = u32Previous - u32Target;
    return u32Previous - ((u32Delta + 8u) >> 4);
}

#define GAIN_STAT_COLLECTION_SIZE   0x40
static unsigned gain_stat_counter = 0;
static unsigned gain_stat_Rg[GAIN_STAT_COLLECTION_SIZE];
static unsigned gain_stat_Bg[GAIN_STAT_COLLECTION_SIZE];

HI_S32 AwbRun_calibrate_gains_default(HI_S32 s32Handle, const ISP_AWB_INFO_S *pstAwbInfo, ISP_AWB_RESULT_S *pstAwbResult, HI_S32 s32Reserved)
{
    AWB_STUB_CTX_S *pstCtx;
    HI_U32 au32TargetGain[4];
    HI_U32 i;

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

/*
    if (1) fprintf(stderr,
        "  global AWB statistics: G/R=0x%03x, G/B=0x%03x, "
        "whitePoints=%u\n",
        pstAwbInfo->pstAwbStat1->u16RgRatio,
        pstAwbInfo->pstAwbStat1->u16BgRatio,
        pstAwbInfo->pstAwbStat1->u32WhitePointCount);
*/

    /* Structure assignment copies the complete 0x34-byte ARM32 result. */
    *pstAwbResult = pstCtx->stResult;

    /*
     * VERY SIMPLE AWB:
     *
     * Statistics are Q8 correction factors relative to the sensor's Q8
     * calibration offsets.  Q8 * Q8 directly produces Q16 result gains.
     * The context keeps the filtered gains, so invalid statistics naturally
     * hold the most recent valid result rather than returning to defaults.
     */
    if (pstAwbInfo->pstAwbStat1->u32WhitePointCount >= 1024u &&
        pstAwbInfo->pstAwbStat1->u16RgRatio != 0 &&
        pstAwbInfo->pstAwbStat1->u16BgRatio != 0) {

        HI_U32 u32RCorrection = pstAwbInfo->pstAwbStat1->u16RgRatio;
        HI_U32 u32BCorrection = pstAwbInfo->pstAwbStat1->u16BgRatio;

        au32TargetGain[0] = pstCtx->au32SaveTargetGain[0] * u32RCorrection;
        au32TargetGain[1] = pstCtx->au32SaveTargetGain[1] * 0x100u;
        au32TargetGain[2] = pstCtx->au32SaveTargetGain[2] * 0x100u;
        au32TargetGain[3] = pstCtx->au32SaveTargetGain[3] * u32BCorrection;

        pstCtx->au32SaveTargetGain[0] = (pstCtx->au32SaveTargetGain[0] + ((128 + au32TargetGain[0]) >> 8)) >> 1;
        pstCtx->au32SaveTargetGain[1] = (pstCtx->au32SaveTargetGain[1] + ((128 + au32TargetGain[1]) >> 8)) >> 1;
        pstCtx->au32SaveTargetGain[2] = (pstCtx->au32SaveTargetGain[2] + ((128 + au32TargetGain[2]) >> 8)) >> 1;
        pstCtx->au32SaveTargetGain[3] = (pstCtx->au32SaveTargetGain[3] + ((128 + au32TargetGain[3]) >> 8)) >> 1;

        for (i = 0; i < 4; ++i) {
            pstCtx->stResult.au32WhiteBalanceGain[i] =
                AwbFilterGain(pstCtx->stResult.au32WhiteBalanceGain[i], au32TargetGain[i]);

            pstAwbResult->au32WhiteBalanceGain[i] = pstCtx->stResult.au32WhiteBalanceGain[i];
        }

/*
        if (0) fprintf(stderr,
            "  simple AWB target Q16: R=0x%08x, Gr=0x%08x, "
            "Gb=0x%08x, B=0x%08x (IIR 1/16)\n",
            au32TargetGain[0],
            au32TargetGain[1],
            au32TargetGain[2],
            au32TargetGain[3]);*/

//        if (1) fprintf(stderr, "CALIBRATION default GAIN: R=0x%x, Gr=0x%x, Gb=0x%x, B=0x%x\n", pstCtx->au32SaveTargetGain[0], pstCtx->au32SaveTargetGain[1], pstCtx->au32SaveTargetGain[2], pstCtx->au32SaveTargetGain[3]);

        if (gain_stat_counter > GAIN_STAT_COLLECTION_SIZE) gain_stat_counter = 0;
        if (gain_stat_counter == GAIN_STAT_COLLECTION_SIZE)
        {
            unsigned avg_Rg = 0;
            unsigned avg_Bg = 0;
            for (unsigned i = 0; i < GAIN_STAT_COLLECTION_SIZE; ++i)
            {
                avg_Rg += gain_stat_Rg[i];
                avg_Bg += gain_stat_Bg[i];
            }

            avg_Rg /= GAIN_STAT_COLLECTION_SIZE;
            avg_Bg /= GAIN_STAT_COLLECTION_SIZE;
            fprintf(stderr, "CALIBRATION default GAIN: R=0x%x, Gr=0x100, Gb=0x100, B=0x%x\n", avg_Rg, avg_Bg);
        }

        gain_stat_Rg[gain_stat_counter] = pstCtx->au32SaveTargetGain[0];
        gain_stat_Bg[gain_stat_counter] = pstCtx->au32SaveTargetGain[3];

        gain_stat_counter++;
    }

  /*  if (1) fprintf(stderr,
        "  output WB gains Q16: R=0x%08x, Gr=0x%08x, "
        "Gb=0x%08x, B=0x%08x\n",
        pstAwbResult->au32WhiteBalanceGain[0],
        pstAwbResult->au32WhiteBalanceGain[1],
        pstAwbResult->au32WhiteBalanceGain[2],
        pstAwbResult->au32WhiteBalanceGain[3]);
*/

    pstCtx->stResult.stStatAttr.bChange = HI_FALSE;

    return HI_SUCCESS;
}
