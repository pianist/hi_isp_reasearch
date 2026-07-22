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

/*
 * Integer square root used by the original AWB implementation.
 *
 * The final comparison rounds to the nearest integer instead of always
 * returning floor(sqrt(value)).  Keeping the same rounding is important in
 * AwbToMired(), where several successive fixed-point square roots are used.
 */
static HI_U32 AwbSqrt32(HI_U32 u32Value)
{
    HI_U32 u32Root;
    HI_U32 u32Trial;
    HI_U32 u32Bit;

    u32Root = (u32Value <= 0x3FFFFFFFu) ? 0u : 0x8000u;

    for (u32Bit = 0x4000u; u32Bit >= 2u; u32Bit >>= 1) {
        u32Trial = u32Root + u32Bit;
        if (u32Value >= u32Trial * u32Trial)
            u32Root = u32Trial;
    }

    u32Trial = u32Root + 1u;
    if (u32Trial * u32Trial <= u32Value) {
        u32Root = u32Trial;
        ++u32Trial;
    }

    if (u32Value > u32Root * u32Trial)
        return u32Trial;

    return u32Root;
}


/*
 * Convert a sensor-specific R/G,B/G point to reciprocal color temperature.
 *
 * This is a direct, named reconstruction of the original AwbToMired().
 * The six calibration coefficients come from AWB_SENSOR_DEFAULT_S and were
 * produced by HiSilicon's sensor AWB calibration tool:
 *
 *     [ p1, p2, q1 ]       Planckian-locus curve
 *     [ a1, b1, c1 ]       locus-coordinate -> mired mapping
 *
 * b1 is fixed at 0x80 by the public API and is implicit in this fixed-point
 * formula, so the original function does not read as32WbPara[4].
 *
 * u32Rg/u32Bg are sensor color coordinates in unsigned Q8.  They are the
 * reciprocals of the G/R and G/B correction ratios supplied by this ISP.
 * The returned Planck offset is a signed distance from the calibrated locus.
 */
static HI_S32 AwbToMired(
    const AWB_SENSOR_DEFAULT_S *pstSensorDefault,
    HI_U32 u32Rg,
    HI_U32 u32Bg,
    HI_U32 *pu32MiredQ8,
    HI_S16 *ps16PlanckOffset)
{
    HI_S32 s32P1;
    HI_S32 s32P2;
    HI_S32 s32Q1;
    HI_S32 s32A1;
    HI_S32 s32C1;
    HI_U32 u32ReferenceRg;
    HI_U32 u32HalfRg;
    HI_U32 u32SlopeQ8;
    HI_S32 s32SlopeQ8;
    HI_S32 s32TwiceSlopeQ8;
    HI_S32 s32CurveTerm;
    HI_S32 s32Radicand;
    HI_S32 s32LocusRg;
    HI_S32 s32LocusBg;
    HI_S32 s32MiredQ8;
    HI_S32 s32DeltaRg;
    HI_S32 s32DeltaBg;
    HI_U32 u32Distance;

    s32P1 = pstSensorDefault->as32WbPara[0];
    s32P2 = pstSensorDefault->as32WbPara[1];
    s32Q1 = pstSensorDefault->as32WbPara[2];
    s32A1 = pstSensorDefault->as32WbPara[3];
    s32C1 = pstSensorDefault->as32WbPara[5];

    if (u32Rg != 0u) {
        u32ReferenceRg = u32Rg;
        u32HalfRg = u32Rg >> 1;
    } else {
        /* Matches the original divide-by-zero guard. */
        u32ReferenceRg = 1u;
        u32HalfRg = 1u;
        u32Rg = 1u;
    }

    u32SlopeQ8 = (u32HalfRg + (u32Bg << 8)) / u32Rg;
    if (u32SlopeQ8 != 0u) {
        s32SlopeQ8 = (HI_S32)u32SlopeQ8;
        s32TwiceSlopeQ8 = 2 * s32SlopeQ8;
        s32CurveTerm =
            s32Q1 *
            ((s32Q1 *
              ((s32SlopeQ8 * s32SlopeQ8 + 128) >> 8) + 128) >> 8);
    } else {
        /* Matches the non-zero fallback in the original implementation. */
        s32SlopeQ8 = 1;
        s32TwiceSlopeQ8 = 2;
        s32CurveTerm = 0;
    }

    s32Radicand =
        s32CurveTerm +
        s32P1 * s32P1 +
        s32SlopeQ8 * 4 * s32P2 -
        s32Q1 * ((s32P1 * s32TwiceSlopeQ8 + 128) >> 8);

    /* Valid calibration data produces a non-negative radicand. */
    if (s32Radicand < 0)
        s32Radicand = 0;

    s32LocusRg =
        (s32P1 * 256) / s32TwiceSlopeQ8 -
        s32Q1 / 2 +
        ((HI_S32)AwbSqrt32((HI_U32)s32Radicand) * 128) /
            s32SlopeQ8;

    if (s32LocusRg < 0)
        s32LocusRg = 0;

    s32MiredQ8 =
        s32C1 +
        (s32A1 *
         (HI_S32)AwbSqrt32((HI_U32)s32LocusRg << 8) + 128) /
            256;

    if (s32MiredQ8 < 0)
        s32MiredQ8 = -s32MiredQ8;

    *pu32MiredQ8 = (HI_U32)s32MiredQ8;

    s32LocusBg =
        (s32LocusRg * s32SlopeQ8 + 128) >> 8;
    s32DeltaRg = s32LocusRg - (HI_S32)u32ReferenceRg;
    s32DeltaBg = s32LocusBg - (HI_S32)u32Bg;
    u32Distance = AwbSqrt32(
        (HI_U32)(s32DeltaRg * s32DeltaRg +
                 s32DeltaBg * s32DeltaBg));

//fprintf(stderr, "(Rg=0x%X, Bg=0x%X) LocusRg => %d, ReferenceRg=%d\n", u32Rg, u32Bg, s32LocusRg, u32ReferenceRg);//XXX
    if (s32LocusRg > (HI_S32)u32ReferenceRg) {
        *ps16PlanckOffset = (HI_S16)-(HI_S32)u32Distance;
        return -(HI_S32)u32Distance;
    }

    *ps16PlanckOffset = (HI_S16)u32Distance;
    return (HI_S32)u32Distance;
}

#if AWB_VERBOSE_DEBUG
static void dump_awb_stat2(const ISP_AWB_STAT_2_S *s)
{
    const AWB_STUB_CTX_S *ctx;
    const AWB_SENSOR_DEFAULT_S *dft;
    HI_U32 appliedR;
    HI_U32 appliedG;
    HI_U32 appliedB;
    HI_U32 offsetR;
    HI_U32 offsetG;
    HI_U32 offsetB;
    HI_U32 row;
    HI_U32 col;

    if (s == HI_NULL)
        return;

    /*
     * Заданная сигнатура не содержит handle, поэтому используется
     * основной AWB-контекст handle 0.
     */
    ctx = &g_astAwbStubCtx[0];
    dft = &ctx->stSensorDefault;

    /*
     * ISP result gains имеют Q16. Для зональной математики оригинальная
     * библиотека переводит их обратно в Q8.
     */
    appliedR =
        (ctx->stResult.au32WhiteBalanceGain[0] + 128u) >> 8;
    appliedG =
        (ctx->stResult.au32WhiteBalanceGain[1] + 128u) >> 8;
    appliedB =
        (ctx->stResult.au32WhiteBalanceGain[3] + 128u) >> 8;

    if (appliedR == 0u)
        appliedR = 1u;
    if (appliedG < 0x100u)
        appliedG = 0x100u;
    if (appliedB == 0u)
        appliedB = 1u;

    offsetR = dft->au16GainOffset[0];
    offsetG = dft->au16GainOffset[1];
    offsetB = dft->au16GainOffset[3];

    /*
     * Такие же защитные значения использует AwbWbMatrixCalculate().
     */
    if (offsetR < 0x100u)
        offsetR = 0x100u;
    if (offsetG < 0x100u)
        offsetG = 0x100u;
    if (offsetB < 0x100u)
        offsetB = 0x100u;

    for (row = 0; row < AWB_ZONE_ROWS; ++row) {
        for (col = 0; col < AWB_ZONE_COLUMNS; ++col) {
            HI_U32 z;
            HI_U32 population;
            HI_U32 statRg;
            HI_U32 statBg;
            HI_U32 candidateR;
            HI_U32 candidateB;
            HI_U32 coordinateRg;
            HI_U32 coordinateBg;
            HI_U32 miredQ8;
            HI_U32 mired;
            HI_U32 temperature;
            HI_S16 planckOffset;
            unsigned long long numerator;
            unsigned long long denominator;

            z = row * AWB_ZONE_COLUMNS + col;
            population = s->au16WhitePointCount[z];
            statRg = s->au16RgRatio[z];
            statBg = s->au16BgRatio[z];

            /*
             * Оригинальная библиотека считает зону непригодной при
             * population <= 0x40.
             */
            if (population <= 0x40u ||
                statRg == 0u ||
                statBg == 0u) {
                fputs("      ", stderr);
                continue;
            }

            if (ctx->bWdrMode != HI_FALSE) {
                statRg = (statRg * statRg) >> 8;
                statBg = (statBg * statBg) >> 8;
            }

            /*
             * Восстановление candidate correction gains этой зоны:
             *
             * R = appliedR * offsetG * stat(G/R)
             *     --------------------------------
             *          appliedG * offsetR
             *
             * B = appliedB * offsetG * stat(G/B)
             *     --------------------------------
             *          appliedG * offsetB
             */
            numerator =
                (unsigned long long)appliedR *
                offsetG *
                statRg;
            denominator =
                (unsigned long long)appliedG *
                offsetR;

            candidateR = (HI_U32)(numerator / denominator);

            numerator =
                (unsigned long long)appliedB *
                offsetG *
                statBg;
            denominator =
                (unsigned long long)appliedG *
                offsetB;

            candidateB = (HI_U32)(numerator / denominator);

            if (candidateR == 0u)
                candidateR = 1u;
            if (candidateB == 0u)
                candidateB = 1u;

            if (candidateR > 0xFFFFu)
                candidateR = 0xFFFFu;
            if (candidateB > 0xFFFFu)
                candidateB = 0xFFFFu;

            coordinateRg = 0xFFFFu / candidateR;
            coordinateBg = 0xFFFFu / candidateB;

            if (coordinateRg == 0u || coordinateBg == 0u) {
                fputs("      ", stderr);
                continue;
            }

            (void)AwbToMired(
                dft,
                coordinateRg,
                coordinateBg,
                &miredQ8,
                &planckOffset);

            if (abs(planckOffset) > 64)
            {
                fputs("      ", stderr);
            } else {
                fprintf(stderr, "%4d  ", planckOffset);
            }
/*
            mired = (miredQ8 + 128u) >> 8;
            if (mired == 0u) {
                fputs("      ", stderr);
                continue;
            }*/

            temperature = 1000000u / mired;

            /*
             * Сохраняем фиксированную ширину матрицы: четыре символа
             * температуры и два пробела. Значения >= 10000 K насыщаются.
             */
            if (temperature > 9999u)
                temperature = 9999u;

        }

        fputc('\n', stderr);
    }
}
#endif

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

static HI_VOID simple_ccm(
    const AWB_CCM_S *pstDefaultCcm,
    HI_U32 u32ColorTemp,
    HI_U16 au16DstCcm[9])
{
    const HI_U16 *pu16CcmA;
    const HI_U16 *pu16CcmB;
    HI_U32 u32TempA;
    HI_U32 u32TempB;
    HI_U32 u32MiredQ8;
    HI_U32 u32MiredAQ8;
    HI_U32 u32MiredBQ8;
    HI_U32 u32WeightQ16;
    HI_S32 as32Ccm[9];
    HI_U32 i;

    if (pstDefaultCcm == HI_NULL || au16DstCcm == HI_NULL)
        return;

    /*
     * Invalid calibration fallback: use the middle matrix.
     */
    if (u32ColorTemp == 0u ||
        pstDefaultCcm->u16HighColorTemp == 0u ||
        pstDefaultCcm->u16MidColorTemp == 0u ||
        pstDefaultCcm->u16LowColorTemp == 0u ||
        pstDefaultCcm->u16HighColorTemp <=
            pstDefaultCcm->u16MidColorTemp ||
        pstDefaultCcm->u16MidColorTemp <=
            pstDefaultCcm->u16LowColorTemp) {
        for (i = 0; i < 9; ++i)
            au16DstCcm[i] = pstDefaultCcm->au16MidCCM[i];

        return;
    }

    /*
     * Clamp outside the calibrated color-temperature interval.
     */
    if (u32ColorTemp >= pstDefaultCcm->u16HighColorTemp) {
        for (i = 0; i < 9; ++i)
            au16DstCcm[i] = pstDefaultCcm->au16HighCCM[i];

        return;
    }

    if (u32ColorTemp <= pstDefaultCcm->u16LowColorTemp) {
        for (i = 0; i < 9; ++i)
            au16DstCcm[i] = pstDefaultCcm->au16LowCCM[i];

        return;
    }

    /*
     * Select one of the two interpolation intervals.
     *
     * A is always the higher-temperature endpoint.
     * B is always the lower-temperature endpoint.
     */
    if (u32ColorTemp >= pstDefaultCcm->u16MidColorTemp) {
        pu16CcmA = pstDefaultCcm->au16HighCCM;
        pu16CcmB = pstDefaultCcm->au16MidCCM;
        u32TempA = pstDefaultCcm->u16HighColorTemp;
        u32TempB = pstDefaultCcm->u16MidColorTemp;
    } else {
        pu16CcmA = pstDefaultCcm->au16MidCCM;
        pu16CcmB = pstDefaultCcm->au16LowCCM;
        u32TempA = pstDefaultCcm->u16MidColorTemp;
        u32TempB = pstDefaultCcm->u16LowColorTemp;
    }

    /*
     * Reciprocal-temperature coordinates in mired Q8:
     *
     *     miredQ8 = (1,000,000 / Kelvin) * 256
     */
    u32MiredQ8  = 256000000u / u32ColorTemp;
    u32MiredAQ8 = 256000000u / u32TempA;
    u32MiredBQ8 = 256000000u / u32TempB;

    /*
     * Weight of endpoint B in unsigned Q16.
     */
    if (u32MiredQ8 <= u32MiredAQ8) {
        u32WeightQ16 = 0u;
    } else if (u32MiredQ8 >= u32MiredBQ8) {
        u32WeightQ16 = 0x10000u;
    } else {
        HI_U32 u32Position = u32MiredQ8 - u32MiredAQ8;
        HI_U32 u32Interval = u32MiredBQ8 - u32MiredAQ8;

        u32WeightQ16 = (HI_U32)(
            ((unsigned long long)u32Position * 0x10000u +
             u32Interval / 2u) /
            u32Interval);
    }

    for (i = 0; i < 9; ++i) {
        HI_S32 s32A;
        HI_S32 s32B;
        long long s64Value;

        /*
         * API CCM representation is sign-magnitude:
         *
         *     0x01A8 = +424
         *     0x8091 = -145
         */
        s32A = (pu16CcmA[i] & 0x8000u)
             ? -(HI_S32)(pu16CcmA[i] & 0x7FFFu)
             :  (HI_S32)pu16CcmA[i];

        s32B = (pu16CcmB[i] & 0x8000u)
             ? -(HI_S32)(pu16CcmB[i] & 0x7FFFu)
             :  (HI_S32)pu16CcmB[i];

        s64Value =
            (long long)s32A * (0x10000u - u32WeightQ16) +
            (long long)s32B * u32WeightQ16;

        /* Symmetric rounding for positive and negative coefficients. */
        if (s64Value >= 0)
            as32Ccm[i] = (HI_S32)((s64Value + 0x8000) / 0x10000);
        else
            as32Ccm[i] =
                -(HI_S32)((-s64Value + 0x8000) / 0x10000);
    }

    /*
     * Preserve neutral RGB exactly: each row must sum to Q8 unity (256).
     * Correct the corresponding diagonal element after coefficient rounding.
     */
    for (i = 0; i < 3; ++i) {
        HI_U32 u32Base = i * 3u;
        HI_U32 u32Diagonal = u32Base + i;
        HI_S32 s32RowSum =
            as32Ccm[u32Base] +
            as32Ccm[u32Base + 1u] +
            as32Ccm[u32Base + 2u];

        as32Ccm[u32Diagonal] += 256 - s32RowSum;
    }

    /* Convert signed coefficients back to API sign-magnitude form. */
    for (i = 0; i < 9; ++i) {
        HI_S32 s32Value = as32Ccm[i];

        if (s32Value > 0x7FFF)
            s32Value = 0x7FFF;
        else if (s32Value < -0x7FFF)
            s32Value = -0x7FFF;

        if (s32Value < 0)
            au16DstCcm[i] =
                (HI_U16)(0x8000u | (HI_U16)(-s32Value));
        else
            au16DstCcm[i] = (HI_U16)s32Value;
    }
}

HI_S32 AwbRun(
    HI_S32 s32Handle,
    const ISP_AWB_INFO_S *pstAwbInfo,
    ISP_AWB_RESULT_S *pstAwbResult,
    HI_S32 s32Reserved)
{
    AWB_STUB_CTX_S *pstCtx;
    HI_U32 au32TargetGain[4];
    HI_U32 i;

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

    if (1) fprintf(stderr,
        "  global AWB statistics: G/R=0x%03x, G/B=0x%03x, "
        "whitePoints=%u\n",
        pstAwbInfo->pstAwbStat1->u16RgRatio,
        pstAwbInfo->pstAwbStat1->u16BgRatio,
        pstAwbInfo->pstAwbStat1->u32WhitePointCount);

#if AWB_VERBOSE_DEBUG
    dump_awb_stat2(pstAwbInfo->pstAwbStat2);
#endif

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

        if (0) fprintf(stderr,
            "  simple AWB target Q16: R=0x%08x, Gr=0x%08x, "
            "Gb=0x%08x, B=0x%08x (IIR 1/16)\n",
            au32TargetGain[0],
            au32TargetGain[1],
            au32TargetGain[2],
            au32TargetGain[3]);

        if (1) fprintf(stderr, "CALIBRATION default GAIN: R=0x%x, Gr=0x%x, Gb=0x%x, B=0x%x\n", pstCtx->au32SaveTargetGain[0], pstCtx->au32SaveTargetGain[1], pstCtx->au32SaveTargetGain[2], pstCtx->au32SaveTargetGain[3]);
    }

    if (0) fprintf(stderr,
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
