/*
 * Human-readable reconstruction of the legacy HiSilicon ISP AWB framework.
 * Target ABI: ARMv5TEJ, 32-bit little-endian.
 *
 * This is framework code, not the proprietary AWB algorithm itself.  The
 * actual algorithm is supplied through ISP_AWB_LIB_NODE_S callbacks.
 */

#include "junk_ida_isp_ctx.h"

/* AWB metering limits. */
#define METERING_AWB_WHITE_LEVEL      0x205A0640
#define METERING_AWB_BLACK_LEVEL      0x205A0644
#define METERING_AWB_CR_REF_MAX       0x205A0648
#define METERING_AWB_CR_REF_MIN       0x205A064C
#define METERING_AWB_CB_REF_MAX       0x205A0650
#define METERING_AWB_CB_REF_MIN       0x205A0654

/* Unity white-balance multipliers inside the CCM module. */
#define CCM_COEFFT_WBR                0x205A04A8
#define CCM_COEFFT_WBG                0x205A04AC
#define CCM_COEFFT_WBB                0x205A04B0

/* External ISP state. */
#define EXT_SYSTEM_WDR_MODE           0x12114
#define EXT_SYSTEM_WDR_ENABLE         0x0004

/* Common/private commands sent to an AWB library. */
#define ISP_WDR_MODE_SET              8000
#define ISP_AWB_ISO_SET               8003
#define ISP_AWB_RUNTIME_PARAM_SET     8005

extern ISP_CTX_S g_astIspCtx[ISP_MAX_DEV_NUM];

extern HI_U16 j_IO_READ16(HI_U32 u32Addr);
extern void j_IO_WRITE16(HI_U32 u32Addr, HI_U16 u16Value);

extern void *memset(void *pDst, int c, unsigned int u32Size);
extern int printf(const char *pszFormat, ...);

/*
 * Exact integer operation recovered from AwbCfgReg:
 *
 *     result = round(sqrt(value))
 *
 * The final comparison against floor * ceil implements nearest-integer
 * rounding without floating point.
 */
static HI_U16 AwbRoundedSqrt(HI_U32 u32Value)
{
    HI_U32 u32Root = 0;
    HI_U32 u32Bit;
    HI_U32 u32Candidate;
    HI_U32 u32Next;

    for (u32Bit = 0x4000; u32Bit >= 2; u32Bit >>= 1)
    {
        u32Candidate = u32Root + u32Bit;

        if (u32Value >= u32Candidate * u32Candidate)
            u32Root = u32Candidate;
    }

    u32Next = u32Root + 1;
    if (u32Value > u32Root * u32Next)
        u32Root = u32Next;

    return (HI_U16)u32Root;
}

static HI_U16 AwbWdrLevel(HI_U16 u16LinearLevel)
{
    /* Preserves the 10-bit level scale in the square-root WDR domain. */
    return AwbRoundedSqrt((HI_U32)u16LinearLevel << 10);
}

static HI_U16 AwbWdrRatio(HI_U16 u16LinearRatioQ8)
{
    /* Preserves Q4.8 while converting a linear ratio to sqrt(ratio). */
    return AwbRoundedSqrt((HI_U32)u16LinearRatioQ8 << 8);
}

static void AwbCopyMeteringConfig(
    const ISP_AWB_STAT_ATTR_S *pstStatAttr,
    ISP_AWB_METER_CFG_S *pstMetering)
{
    pstMetering->u16WhiteLevel =
        pstStatAttr->u16MeteringWhiteLevelAwb;
    pstMetering->u16BlackLevel =
        pstStatAttr->u16MeteringBlackLevelAwb;
    pstMetering->u16CrRefMax =
        pstStatAttr->u16MeteringCrRefMaxAwb;
    pstMetering->u16CrRefMin =
        pstStatAttr->u16MeteringCrRefMinAwb;
    pstMetering->u16CbRefMax =
        pstStatAttr->u16MeteringCbRefMaxAwb;
    pstMetering->u16CbRefMin =
        pstStatAttr->u16MeteringCbRefMinAwb;
}

static void AwbConvertMeteringForWdr(
    const ISP_AWB_STAT_ATTR_S *pstStatAttr,
    ISP_AWB_METER_CFG_S *pstMetering)
{
    pstMetering->u16WhiteLevel =
        AwbWdrLevel(pstStatAttr->u16MeteringWhiteLevelAwb);
    pstMetering->u16BlackLevel =
        AwbWdrLevel(pstStatAttr->u16MeteringBlackLevelAwb);
    pstMetering->u16CrRefMax =
        AwbWdrRatio(pstStatAttr->u16MeteringCrRefMaxAwb);
    pstMetering->u16CrRefMin =
        AwbWdrRatio(pstStatAttr->u16MeteringCrRefMinAwb);
    pstMetering->u16CbRefMax =
        AwbWdrRatio(pstStatAttr->u16MeteringCbRefMaxAwb);
    pstMetering->u16CbRefMin =
        AwbWdrRatio(pstStatAttr->u16MeteringCbRefMinAwb);
}

void AwbCfgReg(
    const ISP_AWB_RESULT_S *pstAwbResult,
    HI_BOOL bWdrMode,
    HI_U32 u32Reserved0,
    HI_U32 u32Reserved1,
    ISP_REG_CFG_S *pstRegCfg)
{
    int i;

    (void)u32Reserved0;
    (void)u32Reserved1;

    if (pstAwbResult->stStatAttr.bChange)
    {
        if (bWdrMode)
        {
            AwbConvertMeteringForWdr(
                &pstAwbResult->stStatAttr,
                &pstRegCfg->stAwbMetering);
        }
        else
        {
            AwbCopyMeteringConfig(
                &pstAwbResult->stStatAttr,
                &pstRegCfg->stAwbMetering);
        }

        pstRegCfg->unKey.u32Key |= ISP_REG_CFG_AWB_METERING_UPDATE;
    }

    for (i = 0; i < 9; ++i)
        pstRegCfg->stCcm.au16Coeff[i] = pstAwbResult->au16ColorMatrix[i];

    pstRegCfg->stDgainCfg.stWbGain.u32R =
        pstAwbResult->au32WhiteBalanceGain[0];
    pstRegCfg->stDgainCfg.stWbGain.u32Gr =
        pstAwbResult->au32WhiteBalanceGain[1];
    pstRegCfg->stDgainCfg.stWbGain.u32Gb =
        pstAwbResult->au32WhiteBalanceGain[2];
    pstRegCfg->stDgainCfg.stWbGain.u32B =
        pstAwbResult->au32WhiteBalanceGain[3];

    pstRegCfg->unKey.u32Key |= ISP_REG_CFG_WB_CCM_UPDATE;
}

HI_S32 ISP_AwbCtrl(ISP_DEV IspDev, HI_U32 u32Cmd, void *pValue)
{
    ISP_CTX_S *pstIspCtx = &g_astIspCtx[IspDev];
    HI_S32 s32Result = -1;
    int i;

    for (i = 0; i < ISP_MAX_AWB_LIBS; ++i)
    {
        ISP_AWB_LIB_NODE_S *pstLib = &pstIspCtx->astAwbLibs[i];

        if (pstLib->bUsed && pstLib->pfnAwbCtrl)
        {
            s32Result = pstLib->pfnAwbCtrl(
                pstLib->stAlgLib.s32Id,
                u32Cmd,
                pValue);
        }
    }

    return s32Result;
}

HI_S32 ISP_AwbInit(ISP_DEV IspDev)
{
    ISP_CTX_S *pstIspCtx = &g_astIspCtx[IspDev];
    ISP_AWB_PARAM_S stAwbParam;
    HI_BOOL bWdrMode;
    int i;

    j_IO_WRITE16(METERING_AWB_WHITE_LEVEL, 940);
    j_IO_WRITE16(METERING_AWB_BLACK_LEVEL, 64);
    j_IO_WRITE16(METERING_AWB_CR_REF_MAX, 512);
    j_IO_WRITE16(METERING_AWB_CR_REF_MIN, 128);
    j_IO_WRITE16(METERING_AWB_CB_REF_MAX, 512);
    j_IO_WRITE16(METERING_AWB_CB_REF_MIN, 128);

    j_IO_WRITE16(CCM_COEFFT_WBR, 256);
    j_IO_WRITE16(CCM_COEFFT_WBG, 256);
    j_IO_WRITE16(CCM_COEFFT_WBB, 256);

    stAwbParam.s32SensorId = pstIspCtx->stBindAttr.SensorId;

    for (i = 0; i < ISP_MAX_AWB_LIBS; ++i)
    {
        ISP_AWB_LIB_NODE_S *pstLib = &pstIspCtx->astAwbLibs[i];

        if (pstLib->bUsed && pstLib->pfnAwbInit)
        {
            pstLib->pfnAwbInit(
                pstLib->stAlgLib.s32Id,
                &stAwbParam);
        }
    }

    if (j_IO_READ16(EXT_SYSTEM_WDR_MODE) & EXT_SYSTEM_WDR_ENABLE)
    {
        bWdrMode = 1;
        ISP_AwbCtrl(IspDev, ISP_WDR_MODE_SET, &bWdrMode);
    }

    return 0;
}

HI_S32 ISP_AwbExit(ISP_DEV IspDev)
{
    ISP_CTX_S *pstIspCtx = &g_astIspCtx[IspDev];
    int i;

    for (i = 0; i < ISP_MAX_AWB_LIBS; ++i)
    {
        ISP_AWB_LIB_NODE_S *pstLib = &pstIspCtx->astAwbLibs[i];

        if (pstLib->bUsed && pstLib->pfnAwbExit)
            pstLib->pfnAwbExit(pstLib->stAlgLib.s32Id);
    }

    return 0;
}

static void AwbSendRuntimeState(
    ISP_AWB_LIB_NODE_S *pstLib,
    ISP_CTX_S *pstIspCtx)
{
    if (!pstLib->pfnAwbCtrl)
        return;

    pstLib->pfnAwbCtrl(
        pstLib->stAlgLib.s32Id,
        ISP_AWB_ISO_SET,
        &pstIspCtx->u32Iso);

    pstLib->pfnAwbCtrl(
        pstLib->stAlgLib.s32Id,
        ISP_AWB_RUNTIME_PARAM_SET,
        &pstIspCtx->u32AwbParam8005);
}

HI_S32 ISP_AwbRun(
    ISP_DEV IspDev,
    const ISP_STAT_S *pstStat,
    ISP_REG_CFG_S *pstRegCfg)
{
    ISP_CTX_S *pstIspCtx = &g_astIspCtx[IspDev];
    ISP_AWB_LIB_NODE_S *pstRunLib;
    ISP_AWB_INFO_S stAwbInfo;
    ISP_AWB_RESULT_S stAwbResult;
    HI_S32 s32Result;
    int i;

    if (pstIspCtx->bFreezeFmw)
        return 0;

    memset(&stAwbResult, 0, sizeof(stAwbResult));

    stAwbInfo.u32FrameCnt = pstIspCtx->u32FrameCnt;
    /* The legacy ISP_AWB_INFO_S ABI does not const-qualify nested pointers. */
    stAwbInfo.pstAwbStat1 =
        (ISP_AWB_STAT_1_S *)&pstStat->stAwbGlobal;
    stAwbInfo.pstAwbStat2 =
        (ISP_AWB_STAT_2_S *)&pstStat->stAwbZonal;

    /*
     * This apparently redundant initial choice is present in the binary.
     * Every bUsed slot below replaces it; if both slots are used, slot 1
     * becomes the Run library regardless of s32ActiveAwbLib.
     */
    pstRunLib =
        &pstIspCtx->astAwbLibs[pstIspCtx->s32ActiveAwbLib];

    for (i = 0; i < ISP_MAX_AWB_LIBS; ++i)
    {
        ISP_AWB_LIB_NODE_S *pstLib = &pstIspCtx->astAwbLibs[i];

        if (pstLib->bUsed)
        {
            pstRunLib = pstLib;
            AwbSendRuntimeState(pstLib, pstIspCtx);
        }
    }

    if (pstRunLib->pfnAwbRun)
    {
        s32Result = pstRunLib->pfnAwbRun(
            pstRunLib->stAlgLib.s32Id,
            &stAwbInfo,
            &stAwbResult,
            0);

        if (s32Result != 0)
            printf("WARNING!! run awb lib err 0x%x!\n", s32Result);
    }
    else
    {
        s32Result = -1;
    }

    /* Applied even when the callback is missing or reports an error. */
    AwbCfgReg(
        &stAwbResult,
        pstIspCtx->enWdrModeExt,
        pstIspCtx->u32Field740,
        pstIspCtx->u32Field744,
        pstRegCfg);

    return s32Result;
}
