/*
 * Human-readable reconstruction of the built-in HiSilicon defect-pixel
 * algorithm.  Target ABI: ARMv5TEJ, 32-bit little-endian.
 *
 * The implementation follows the recovered ISP_DpInit/ISP_DpRun control
 * flow.  Names of the external configuration registers are descriptive;
 * their original vendor macro names are not yet available.
 */

#include "junk_ida_isp_ctx.h"

/* Physical ISP registers. */
#define GE_CTRL1                       0x205A01C0
#define GE_CTRL3_DP_THRESHOLD_HI16     0x205A01CA
#define GE_CTRL5                       0x205A01D0
#define HP_DEFECT_PIXEL_CTRL           0x205A01E8

/* External ISP control registers. */
#define EXT_DP_TRIGGER                 0x10317
#define EXT_DP_CALIB_THRESHOLD         0x12106
#define EXT_DP_MAX_TRIALS_X8           0x12108
#define EXT_DP_COUNT_UPPER_LIMIT       0x12110
#define EXT_DP_COUNT_LOWER_LIMIT       0x12112
#define EXT_DP_SLOPE                   0x12160
#define EXT_DP_THRESHOLD               0x12162

/* HP_DEFECT_PIXEL_CTRL fields. */
#define HP_CORRECTION_ENABLE           0x04
#define HP_SHOW_STATIC_PIXELS          0x08

/* GE_CTRL1 fields. */
#define GE_DEFECT_PIXEL_ENABLE         0x04

/* Calibration status exported to the control layer. */
#define DP_CALIB_STATUS_SUCCESS        1
#define DP_CALIB_STATUS_TIMEOUT        2

/* Calibration states which perform work after the per-frame increment. */
#define DP_STATE_PROGRAM_TABLE         3
#define DP_STATE_START_DETECTION       4
#define DP_STATE_STOP_DETECTION        5
#define DP_STATE_EVALUATE_COUNT        7

extern ISP_CTX_S    g_astIspCtx[ISP_MAX_DEV_NUM];
extern ISP_DP_CTX_S g_astDpCtx[ISP_MAX_DEV_NUM];

extern HI_U8  j_IO_READ8(HI_U32 u32Addr);
extern HI_U16 j_IO_READ16(HI_U32 u32Addr);
extern void j_IO_WRITE8(HI_U32 u32Addr, HI_U8 u8Value);
extern void j_IO_WRITE16(HI_U32 u32Addr, HI_U16 u16Value);

extern HI_S32 j_ISP_SensorSetPixelDetect(ISP_DEV IspDev, HI_BOOL bEnable);
extern void hi_ext_system_bad_pixel_trigger_status_write(HI_U32 u32Status);

/* Recovered separately from the empty framework callback ISP_DpExit(). */
extern void DpExit(ISP_DEV IspDev, ISP_DP_CTX_S *pstDpCtx);

extern int printf(const char *pszFormat, ...);

static HI_U8 DpReadBit(HI_U32 u32Addr, HI_U8 u8Mask)
{
    return (j_IO_READ8(u32Addr) & u8Mask) != 0;
}

static void DpWriteBit(HI_U32 u32Addr, HI_U8 u8Mask, HI_BOOL bEnable)
{
    HI_U8 u8Value = j_IO_READ8(u32Addr);

    if (bEnable)
        u8Value |= u8Mask;
    else
        u8Value &= (HI_U8)~u8Mask;

    j_IO_WRITE8(u32Addr, u8Value);
}

static void DpSetCalibrationStatus(HI_U32 u32Status)
{
    hi_ext_system_bad_pixel_trigger_status_write(u32Status);
}

static void DpFinishCalibration(
    ISP_DEV IspDev,
    ISP_DP_CTX_S *pstDpCtx,
    ISP_REG_CFG_S *pstRegCfg,
    signed char s8Result,
    HI_U32 u32Status)
{
    DpExit(IspDev, pstDpCtx);

    pstRegCfg->stGreenEqualization.u8Strength = 0;
    pstRegCfg->unKey.u32Key |= ISP_REG_CFG_GE_UPDATE;

    pstDpCtx->s8CalibrationResult = s8Result;
    DpSetCalibrationStatus(u32Status);
}

HI_S32 ISP_DpCtrl(ISP_DEV IspDev, HI_U32 u32Cmd, void *pValue)
{
    (void)IspDev;
    (void)u32Cmd;
    (void)pValue;
    return 0;
}

HI_S32 ISP_DpExit(ISP_DEV IspDev)
{
    (void)IspDev;
    return 0;
}

HI_S32 ISP_DpInit(ISP_DEV IspDev)
{
    ISP_DP_CTX_S *pstDpCtx = &g_astDpCtx[IspDev];
    HI_U16 u16Value;

    DpWriteBit(HP_DEFECT_PIXEL_CTRL, HP_CORRECTION_ENABLE, 1);
    DpWriteBit(EXT_DP_TRIGGER, 0x01, 0);

    j_IO_WRITE16(EXT_DP_COUNT_UPPER_LIMIT, 256);
    j_IO_WRITE16(EXT_DP_COUNT_LOWER_LIMIT, 64);

    u16Value = j_IO_READ16(EXT_DP_CALIB_THRESHOLD);
    j_IO_WRITE16(
        EXT_DP_CALIB_THRESHOLD,
        (HI_U16)((u16Value & 0xFF00) | 0x1F));

    u16Value = j_IO_READ16(EXT_DP_CALIB_THRESHOLD);
    j_IO_WRITE16(EXT_DP_CALIB_THRESHOLD, (HI_U16)(u16Value & 0xFCFF));

    j_IO_WRITE16(EXT_DP_MAX_TRIALS_X8, 1600);
    j_IO_WRITE16(EXT_DP_SLOPE, 512);
    j_IO_WRITE16(EXT_DP_THRESHOLD, 64);

    DpWriteBit(GE_CTRL1, GE_DEFECT_PIXEL_ENABLE, 1);

    pstDpCtx->u32StructSize = sizeof(*pstDpCtx);
    pstDpCtx->u8TrialCount = 0;
    pstDpCtx->u8CalibrationState = 0;
    pstDpCtx->u16DetectedPixelCount = 0;

    return 0;
}

static void DpRefreshRuntimeState(ISP_DP_CTX_S *pstDpCtx)
{
    pstDpCtx->u8MaxTrialCount =
        (HI_U8)(j_IO_READ16(EXT_DP_MAX_TRIALS_X8) >> 3);
    pstDpCtx->bCalibrationTrigger =
        DpReadBit(EXT_DP_TRIGGER, 0x01);
    pstDpCtx->u16DpSlope = j_IO_READ16(EXT_DP_SLOPE);
    pstDpCtx->u16DpThreshold = j_IO_READ16(EXT_DP_THRESHOLD);
    pstDpCtx->bDefectPixelEnable =
        DpReadBit(GE_CTRL1, GE_DEFECT_PIXEL_ENABLE);
    pstDpCtx->bShowStaticDefectPixels =
        DpReadBit(HP_DEFECT_PIXEL_CTRL, HP_SHOW_STATIC_PIXELS);
}

static void DpStartCalibration(
    ISP_DEV IspDev,
    ISP_DP_CTX_S *pstDpCtx,
    ISP_REG_CFG_S *pstRegCfg)
{
    j_ISP_SensorSetPixelDetect(IspDev, 1);

    pstDpCtx->bCalibrationActive = 1;
    pstDpCtx->u8CalibrationDpThreshold =
        (HI_U8)j_IO_READ16(EXT_DP_CALIB_THRESHOLD);

    pstRegCfg->stDefectPixel.u16DpSlope = 512;
    pstRegCfg->stGreenEqualization.u8Strength = 32;
    pstRegCfg->unKey.u32Key |=
        ISP_REG_CFG_DEFECT_UPDATE | ISP_REG_CFG_GE_UPDATE;
}

static void DpProgramCalibrationTable(
    const ISP_DP_CTX_S *pstDpCtx,
    ISP_REG_CFG_S *pstRegCfg)
{
    ISP_DEFECT_PIXEL_REG_CFG_S *pstDpCfg = &pstRegCfg->stDefectPixel;

    pstDpCfg->u8CalibrationDpThreshold =
        pstDpCtx->u8CalibrationDpThreshold;
    pstDpCfg->u16DefectPixelCountIn = 0;
    pstDpCfg->bPointerReset = 1;
    pstDpCfg->bShowStaticDefectPixels = 1;
    pstDpCfg->bCorrectionEnable = 1;

    pstRegCfg->unKey.u32Key |= ISP_REG_CFG_DEFECT_UPDATE;
}

static void DpStartDetection(ISP_REG_CFG_S *pstRegCfg)
{
    pstRegCfg->stDefectPixel.bPointerReset = 0;
    pstRegCfg->stDefectPixel.bDetectionTrigger = 1;
    pstRegCfg->unKey.u32Key |= ISP_REG_CFG_DEFECT_UPDATE;
}

static void DpStopDetection(ISP_REG_CFG_S *pstRegCfg)
{
    pstRegCfg->stDefectPixel.bDetectionTrigger = 0;
    pstRegCfg->unKey.u32Key |= ISP_REG_CFG_DEFECT_UPDATE;
}

static void DpRetryWithLowerThreshold(ISP_DP_CTX_S *pstDpCtx)
{
    HI_U8 u8NextTrial = pstDpCtx->u8TrialCount;

    pstDpCtx->u8CalibrationState = 2;

    if (pstDpCtx->u8CalibrationDpThreshold == 1)
        u8NextTrial = pstDpCtx->u8MaxTrialCount;

    --pstDpCtx->u8CalibrationDpThreshold;
    pstDpCtx->u8TrialCount = (HI_U8)(u8NextTrial + 1);
}

static void DpRetryWithHigherThreshold(ISP_DP_CTX_S *pstDpCtx)
{
    ++pstDpCtx->u8CalibrationDpThreshold;
    pstDpCtx->u8CalibrationState = 2;
    ++pstDpCtx->u8TrialCount;
}

static void DpEvaluateCalibration(
    ISP_DEV IspDev,
    ISP_DP_CTX_S *pstDpCtx,
    const ISP_STAT_BUFFER_S *pstStat,
    ISP_REG_CFG_S *pstRegCfg)
{
    HI_U16 u16Count = pstStat->u16DefectPixelCount;
    HI_U16 u16UpperLimit = j_IO_READ16(EXT_DP_COUNT_UPPER_LIMIT);
    HI_U16 u16LowerLimit = j_IO_READ16(EXT_DP_COUNT_LOWER_LIMIT);

    pstDpCtx->u16DetectedPixelCount = u16Count;
    pstRegCfg->stDefectPixel.bShowStaticDefectPixels = 0;
    pstRegCfg->unKey.u32Key |= ISP_REG_CFG_DEFECT_UPDATE;

    if (pstDpCtx->u8TrialCount >= pstDpCtx->u8MaxTrialCount &&
        u16Count <= u16UpperLimit)
    {
        printf("BAD PIXEL CALIBRATION TIME OUT  %x\n",
               pstDpCtx->u8MaxTrialCount);
        DpFinishCalibration(
            IspDev,
            pstDpCtx,
            pstRegCfg,
            -1,
            DP_CALIB_STATUS_TIMEOUT);
        return;
    }

    if (u16Count > u16UpperLimit)
    {
        printf("BAD_PIXEL_COUNT_UPPER_LIMIT %x, %x\n",
               pstDpCtx->u8CalibrationDpThreshold,
               pstDpCtx->u16DetectedPixelCount);
        DpRetryWithHigherThreshold(pstDpCtx);
        return;
    }

    if (u16Count < u16LowerLimit)
    {
        printf("BAD_PIXEL_COUNT_LOWER_LIMIT %x, %x\n",
               pstDpCtx->u8CalibrationDpThreshold,
               pstDpCtx->u16DetectedPixelCount);
        DpRetryWithLowerThreshold(pstDpCtx);
        return;
    }

    printf("trial: %x, findshed: %x\n",
           pstDpCtx->u8TrialCount,
           pstDpCtx->u16DetectedPixelCount);
    DpFinishCalibration(
        IspDev,
        pstDpCtx,
        pstRegCfg,
        1,
        DP_CALIB_STATUS_SUCCESS);
}

static HI_S32 DpRunCalibration(
    ISP_DEV IspDev,
    ISP_CTX_S *pstIspCtx,
    ISP_DP_CTX_S *pstDpCtx,
    const ISP_STAT_BUFFER_S *pstStat,
    ISP_REG_CFG_S *pstRegCfg)
{
    HI_U8 u8State;

    pstIspCtx->bFreezeFmw = 1;
    pstRegCfg->unKey.u32Key = 0;

    if (pstDpCtx->u32StructSize != sizeof(*pstDpCtx))
        return -1;

    u8State = pstDpCtx->u8CalibrationState;
    pstDpCtx->s8CalibrationResult = 0;

    if (u8State > DP_STATE_EVALUATE_COUNT)
        return 0;

    if (u8State == 1)
        DpStartCalibration(IspDev, pstDpCtx, pstRegCfg);

    u8State = (HI_U8)(u8State + 1);
    pstDpCtx->u8CalibrationState = u8State;

    switch (u8State)
    {
        case DP_STATE_PROGRAM_TABLE:
            DpProgramCalibrationTable(pstDpCtx, pstRegCfg);
            break;

        case DP_STATE_START_DETECTION:
            DpStartDetection(pstRegCfg);
            break;

        case DP_STATE_STOP_DETECTION:
            DpStopDetection(pstRegCfg);
            break;

        case DP_STATE_EVALUATE_COUNT:
            DpEvaluateCalibration(IspDev, pstDpCtx, pstStat, pstRegCfg);
            break;

        default:
            break;
    }

    if (pstDpCtx->s8CalibrationResult != 0)
        DpWriteBit(EXT_DP_TRIGGER, 0x01, 0);

    return 0;
}

static void DpApplyNormalConfiguration(
    ISP_DP_CTX_S *pstDpCtx,
    ISP_REG_CFG_S *pstRegCfg)
{
    ISP_DEFECT_PIXEL_REG_CFG_S *pstDpCfg = &pstRegCfg->stDefectPixel;

    pstDpCfg->bDefectPixelEnable = pstDpCtx->bDefectPixelEnable;
    pstDpCfg->u16DpSlope = pstDpCtx->u16DpSlope;
    pstDpCfg->u16DpThreshold = pstDpCtx->u16DpThreshold;
    pstDpCfg->bShowStaticDefectPixels =
        pstDpCtx->bShowStaticDefectPixels;

    j_IO_WRITE16(GE_CTRL5, (HI_U16)(pstDpCfg->u16DpSlope & 0x0FFF));
    j_IO_WRITE16(
        GE_CTRL3_DP_THRESHOLD_HI16,
        (HI_U16)(pstDpCfg->u16DpThreshold & 0x0FFF));

    DpWriteBit(
        GE_CTRL1,
        GE_DEFECT_PIXEL_ENABLE,
        pstDpCfg->bDefectPixelEnable);
    DpWriteBit(
        HP_DEFECT_PIXEL_CTRL,
        HP_SHOW_STATIC_PIXELS,
        pstDpCfg->bShowStaticDefectPixels);
}

HI_S32 ISP_DpRun(
    ISP_DEV IspDev,
    const ISP_STAT_BUFFER_S *pstStat,
    ISP_REG_CFG_S *pstRegCfg)
{
    ISP_CTX_S *pstIspCtx = &g_astIspCtx[IspDev];
    ISP_DP_CTX_S *pstDpCtx = &g_astDpCtx[IspDev];

    DpRefreshRuntimeState(pstDpCtx);

    if (pstDpCtx->bCalibrationTrigger)
        return DpRunCalibration(
            IspDev,
            pstIspCtx,
            pstDpCtx,
            pstStat,
            pstRegCfg);

    if (pstDpCtx->bCalibrationActive)
    {
        DpExit(IspDev, pstDpCtx);
        pstRegCfg->stGreenEqualization.u8Strength = 0;
        pstRegCfg->unKey.u32Key |=
            ISP_REG_CFG_DEFECT_UPDATE | ISP_REG_CFG_GE_UPDATE;
    }
    else
    {
        DpApplyNormalConfiguration(pstDpCtx, pstRegCfg);
    }

    pstIspCtx->bFreezeFmw = 0;
    return 0;
}
