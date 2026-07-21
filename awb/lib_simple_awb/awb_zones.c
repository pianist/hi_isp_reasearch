#include "isp_awb.h"
#include <stdio.h>

#define AWB_ZONE_MIN_POPULATION  64


static HI_U8 planck_neutrality_score(HI_U32 u32AbsOffset)
{
    /*
     * PlanckOffset измеряется в координатах Q8:
     *
     *   8  = 0.03125
     *   16 = 0.06250
     *   32 = 0.12500
     *
     * Это начальные диагностические пороги, а не калибровочные константы.
     */
    if (u32AbsOffset <= 8)
        return 5;

    if (u32AbsOffset <= 16)
        return 4;

    if (u32AbsOffset <= 24)
        return 3;

    if (u32AbsOffset <= 32)
        return 2;

    if (u32AbsOffset <= 48)
        return 1;

    return 0;
}


void best_white_RgBg_zones(
    const ISP_AWB_STAT_2_S *zs,
    const AWB_SENSOR_DEFAULT_S *snsDft)
{
    HI_U8 au8Score[AWB_ZONE_COUNT];
    HI_U8 au8Valid[AWB_ZONE_COUNT];

    HI_U32 x;
    HI_U32 y;
    HI_U32 i;
    HI_U32 u32ValidZones;

    HI_U32 u32ZoneGr;
    HI_U32 u32ZoneGb;
    HI_U32 u32SensorRg;
    HI_U32 u32SensorBg;
    HI_U32 u32Population;

    HI_U32 u32MiredQ8;
    HI_U32 u32ColorTemp;
    HI_S16 s16PlanckOffset;
    HI_S32 s32PlanckOffset;
    HI_U32 u32AbsOffset;

    HI_U32 u32BestIndex;
    HI_U32 u32BestAbsOffset;
    HI_U32 u32BestPopulation;

    if (zs == HI_NULL || snsDft == HI_NULL) {
        fprintf(stderr, "Planck neutrality map: null argument\n");
        return;
    }

    u32ValidZones = 0;
    u32BestIndex = AWB_ZONE_COUNT;
    u32BestAbsOffset = 0xFFFFFFFF;
    u32BestPopulation = 0;

    for (i = 0; i < AWB_ZONE_COUNT; ++i) {
        au8Score[i] = 0;
        au8Valid[i] = 0;

        u32ZoneGr = zs->au16RgRatio[i]; /* G/R, Q8 */
        u32ZoneGb = zs->au16BgRatio[i]; /* G/B, Q8 */
        u32Population = zs->au16WhitePointCount[i];

        if (u32Population <= AWB_ZONE_MIN_POPULATION)
            continue;

        if (u32ZoneGr == 0 || u32ZoneGb == 0)
            continue;

        /*
         * AwbToMired() работает в сенсорных координатах R/G и B/G.
         *
         * Входная statistics содержит обратные отношения G/R и G/B:
         *
         *     R/G = 1 / (G/R)
         *     B/G = 1 / (G/B)
         *
         * 0xFFFF/raw сохраняет Q8-формат и соответствует преобразованию
         * оригинальной lib_hiawb.
         */
        u32SensorRg = 0xFFFF / u32ZoneGr;
        u32SensorBg = 0xFFFF / u32ZoneGb;

        if (u32SensorRg == 0 || u32SensorBg == 0)
            continue;

        u32MiredQ8 = 0;
        s16PlanckOffset = 0;

        (void)AwbToMired(
            snsDft,
            u32SensorRg,
            u32SensorBg,
            &u32MiredQ8,
            &s16PlanckOffset);

        if (u32MiredQ8 == 0)
            continue;

        s32PlanckOffset = (HI_S32)s16PlanckOffset;

        if (s32PlanckOffset < 0)
            u32AbsOffset = (HI_U32)(-s32PlanckOffset);
        else
            u32AbsOffset = (HI_U32)s32PlanckOffset;

        au8Valid[i] = 1;
        au8Score[i] = planck_neutrality_score(u32AbsOffset);
        ++u32ValidZones;

        /*
         * Среди одинаково близких к locus зон предпочитаем зону
         * с большей статистической population.
         */
        if (u32AbsOffset < u32BestAbsOffset ||
            (u32AbsOffset == u32BestAbsOffset &&
             u32Population > u32BestPopulation)) {
            u32BestIndex = i;
            u32BestAbsOffset = u32AbsOffset;
            u32BestPopulation = u32Population;
        }
    }

    fprintf(
        stderr,
        "Planck neutrality map: "
        "5=very close, 0=far, .=population<=%u\n",
        (unsigned int)AWB_ZONE_MIN_POPULATION);

    fprintf(stderr, "       ");
    for (x = 0; x < AWB_ZONE_COLUMNS; ++x)
        fprintf(stderr, "%2u ", (unsigned int)x);
    fprintf(stderr, "\n");

    for (y = 0; y < AWB_ZONE_ROWS; ++y) {
        fprintf(stderr, "y=%02u: ", (unsigned int)y);

        for (x = 0; x < AWB_ZONE_COLUMNS; ++x) {
            i = y * AWB_ZONE_COLUMNS + x;

            if (au8Valid[i])
                fprintf(stderr, "%2u ", (unsigned int)au8Score[i]);
            else
                fprintf(stderr, " . ");
        }

        fprintf(stderr, "\n");
    }

    if (u32BestIndex >= AWB_ZONE_COUNT) {
        fprintf(stderr, "No statistically valid AWB zones\n");
        return;
    }

    u32ZoneGr = zs->au16RgRatio[u32BestIndex];
    u32ZoneGb = zs->au16BgRatio[u32BestIndex];
    u32SensorRg = 0xFFFF / u32ZoneGr;
    u32SensorBg = 0xFFFF / u32ZoneGb;

    (void)AwbToMired(
        snsDft,
        u32SensorRg,
        u32SensorBg,
        &u32MiredQ8,
        &s16PlanckOffset);

    u32ColorTemp =
        (u32MiredQ8 != 0)
            ? (256000000 + u32MiredQ8 / 2) / u32MiredQ8
            : 0;

    fprintf(
        stderr,
        "Best Planck zone: (%u,%u), score=%u, "
        "offset=%d, CCT=%uK, population=%u, "
        "G/R=0x%03x, G/B=0x%03x, "
        "sensor R/G=0x%03x, B/G=0x%03x\n",
        (unsigned int)(u32BestIndex % AWB_ZONE_COLUMNS),
        (unsigned int)(u32BestIndex / AWB_ZONE_COLUMNS),
        (unsigned int)au8Score[u32BestIndex],
        (int)s16PlanckOffset,
        (unsigned int)u32ColorTemp,
        (unsigned int)zs->au16WhitePointCount[u32BestIndex],
        (unsigned int)u32ZoneGr,
        (unsigned int)u32ZoneGb,
        (unsigned int)u32SensorRg,
        (unsigned int)u32SensorBg);
}

