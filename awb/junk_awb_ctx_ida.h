#ifndef AWB_CTX_IDA_H
#define AWB_CTX_IDA_H

/*
 * Partial HiSilicon lib_hiawb.so type reconstruction for IDA.
 * Target ABI: 32-bit ARM, 4-byte pointers and HI_BOOL.
 *
 * Confirmed layout:
 *   sizeof(AWB_CTX_S)            = 0x464 (281 DWORDs)
 *   sizeof(AWB_WB_CTX_S)         = 0x2c8
 *   sizeof(AWB_CM_CTX_S)         = 0x0a4
 *   sizeof(AWB_SENSOR_DEFAULT_S) = 0x06c
 *   sizeof(ISP_AWB_RESULT_S)     = 0x034
 *   sizeof(AWB_DEBUG_CTX_S)      = 0x028
 *
 * Unknown fields deliberately remain byte arrays. Names marked "likely"
 * express current semantic reconstruction and may be revised.
 */

typedef unsigned char      HI_U8;
typedef signed char        HI_S8;
typedef unsigned short     HI_U16;
typedef signed short       HI_S16;
typedef unsigned int       HI_U32;
typedef signed int         HI_S32;
typedef unsigned int       HI_BOOL;
typedef void               HI_VOID;
typedef HI_S32             SENSOR_ID;

typedef enum hiISP_WDR_MODE_E
{
    ISP_SENSOR_LINEAR_MODE = 0,
    ISP_SENSOR_WDR_MODE    = 1,
    ISP_WDR_BUTT
} ISP_WDR_MODE_E;

typedef struct hiAWB_CCM_S
{
    HI_U16 u16HighColorTemp;
    HI_U16 au16HighCCM[9];
    HI_U16 u16MidColorTemp;
    HI_U16 au16MidCCM[9];
    HI_U16 u16LowColorTemp;
    HI_U16 au16LowCCM[9];
} AWB_CCM_S; /* 0x3c */

/* Internal CCM representation used for signed Q8 interpolation. */
typedef struct hiAWB_CCM_INTERNAL_S
{
    /* 0x00 */ HI_U16 u16HighColorTemp;
    /* 0x02 */ HI_S16 as16HighCCM[9];
    /* 0x14 */ HI_U16 u16MidColorTemp;
    /* 0x16 */ HI_S16 as16MidCCM[9];
    /* 0x28 */ HI_U16 u16LowColorTemp;
    /* 0x2a */ HI_S16 as16LowCCM[9];
} AWB_CCM_INTERNAL_S; /* 0x3c */

typedef enum hiAWB_CCM_REGION_E
{
    AWB_CCM_REGION_LOW      = 0,
    AWB_CCM_REGION_LOW_MID  = 1,
    AWB_CCM_REGION_MID      = 2,
    AWB_CCM_REGION_MID_HIGH = 3,
    AWB_CCM_REGION_HIGH     = 4
} AWB_CCM_REGION_E;

typedef struct hiAWB_AGC_TABLE_S
{
    HI_BOOL bValid;
    HI_U8   au8Saturation[8];
} AWB_AGC_TABLE_S; /* 0x0c */

typedef struct hiAWB_SENSOR_DEFAULT_S
{
    /* 0x00 */ HI_U16 u16WbRefTemp;
    /* 0x02 */ HI_U16 au16GainOffset[4];
    /* 0x0a */ HI_U8  au8Padding00A[2];
    /* 0x0c */ HI_S32 as32WbPara[6]; /* p2, p1, q1, a1, b1, c1 */
    /* 0x24 */ AWB_AGC_TABLE_S stAgcTbl;
    /* 0x30 */ AWB_CCM_S stCcm;
} AWB_SENSOR_DEFAULT_S; /* 0x6c */

typedef struct hiAWB_SENSOR_EXP_FUNC_S
{
    HI_S32 (*pfn_cmos_get_awb_default)(AWB_SENSOR_DEFAULT_S *pstAwbSnsDft);
} AWB_SENSOR_EXP_FUNC_S;

typedef struct hiAWB_SENSOR_REGISTER_S
{
    AWB_SENSOR_EXP_FUNC_S stSnsExp;
} AWB_SENSOR_REGISTER_S; /* 0x04 on ARM32 */

typedef struct hiISP_AWB_STAT_ATTR_S
{
    /* 0x00 */ HI_BOOL bChange;
    /* 0x04 */ HI_U16 u16MeteringWhiteLevelAwb;
    /* 0x06 */ HI_U16 u16MeteringBlackLevelAwb;
    /* 0x08 */ HI_U16 u16MeteringCrRefMaxAwb;
    /* 0x0a */ HI_U16 u16MeteringCbRefMaxAwb;
    /* 0x0c */ HI_U16 u16MeteringCrRefMinAwb;
    /* 0x0e */ HI_U16 u16MeteringCbRefMinAwb;
} ISP_AWB_STAT_ATTR_S; /* 0x10 */

typedef struct hiISP_AWB_RESULT_S
{
    /* 0x00 */ HI_U32 au32WhiteBalanceGain[4];
    /* 0x10 */ HI_U16 au16ColorMatrix[9];
    /* 0x22 */ HI_U16 u16Padding022;
    /* 0x24 */ ISP_AWB_STAT_ATTR_S stStatAttr;
} ISP_AWB_RESULT_S; /* 0x34 */

typedef struct hiAWB_ZONE_DBG_S
{
    /* 0x00 */ HI_U16 u16Num;
    /* 0x02 */ HI_U16 u16Padding002;
    /* 0x04 */ HI_U32 u32Sum;
    /* 0x08 */ HI_U16 u16Rg;
    /* 0x0a */ HI_U16 u16Bg;
    /* 0x0c */ HI_U32 u32TK;
    /* 0x10 */ HI_S32 s32Shift;
    /* 0x14 */ HI_U32 u32Weight;
    /* 0x18 */ HI_S32 s32Dev;
} AWB_ZONE_DBG_S; /* 0x1c */

typedef struct hiAWB_DBG_STATUS_S
{
    /* 0x0000 */ HI_U32 u32FrmNumBgn;
    /* 0x0004 */ HI_U32 u32GlobalSum;
    /* 0x0008 */ HI_U32 u32GlobalRgSta;
    /* 0x000c */ HI_U32 u32GlobalBgSta;
    /* Color temperature in Kelvin. */
    /* 0x0010 */ HI_U32 u32ColorTemp;
    /* 0x0014 */ HI_U16 u16Rgain;
    /* 0x0016 */ HI_U16 u16Ggain;
    /* 0x0018 */ HI_U16 u16Bgain;
    /* 0x001a */ HI_U16 au16CCM[9];
    /* 0x002c */ AWB_ZONE_DBG_S astZoneDebug[255];
    /* 0x1c10 */ HI_U32 u32FrmNumEnd;
} AWB_DBG_STATUS_S; /* 0x1c14 = 7188 */

/* Actual payload used by AWB_DEBUG_ATTR_SET/GET in this library binary. */
typedef struct hiAWB_DEBUG_INFO_S
{
    /* 0x00 */ HI_BOOL bEnable;
    /* 0x04 */ HI_U32  u32PhyAddr;
    /* Number of AWB_DBG_STATUS_S records in the debug ring. */
    /* 0x08 */ HI_U32  u32Depth;
} AWB_DEBUG_INFO_S; /* 0x0c */

typedef struct hiAWB_DEBUG_CTX_S
{
    /* 0x00 */ HI_U32 bPrevEnable;
    /* 0x04 */ HI_U32 u32PrevPhyAddr;
    /* 0x08 */ HI_U32 u32PrevDepth;
    /* 0x0c */ HI_U32 u32PrevMapSize;
    /* 0x10 */ HI_U32 bEnable;
    /* 0x14 */ HI_U32 u32PhyAddr;
    /* 0x18 */ HI_U32 u32Depth;
    /* 0x1c */ HI_U32 u32MapSize;
    /* 0x20 */ HI_VOID *pMapBase;
    /* 0x24 */ AWB_DBG_STATUS_S *pstStatus;
} AWB_DEBUG_CTX_S; /* 0x28 */

typedef struct hiAWB_WB_MATRIX_ROW_S
{
    /* 0x00 */ HI_U16 u16Gain;
    /* 0x02 */ HI_U16 u16Unknown002;
    /* 0x04 */ HI_U16 u16Unknown004;
    /* 0x06 */ HI_U16 u16Unknown006;
} AWB_WB_MATRIX_ROW_S; /* 0x08 */

/*
 * Four homogeneous 8-byte records are read from VREG +0x170..+0x18f
 * when WB status bit 4 is enabled.  Field widths and array shape are
 * confirmed; their algorithmic meaning is not yet known.
 */
typedef struct hiAWB_WB_RULE_S
{
    /* 0x00 */ HI_U16 u16Param0;
    /* 0x02 */ HI_U16 u16Param1;
    /* 0x04 */ HI_U16 u16Param2;
    /* 0x06 */ HI_U8  u8Mode;   /* only bits [1:0] are accepted */
    /* 0x07 */ HI_U8  u8Param7;
} AWB_WB_RULE_S; /* 0x08 */

/* Per-zone working record used for all 15x17 = 255 metering zones. */
typedef struct hiAWB_ZONE_WORK_S
{
    /* 0x00 */ HI_U16 u16Rg;
    /* 0x02 */ HI_U16 u16Bg;
    /* 0x04 */ HI_U32 u32RawSum;
    /* 0x08 */ HI_U16 u16Unknown008;
    /* 0x0a */ HI_U16 u16EffectiveSum;
    /* 0x0c */ HI_U16 u16Weight;
    /* 0x0e */ HI_S16 s16PlanckOffset;
    /* 0x10 */ HI_U16 u16Deviation;
    /* 0x12 */ HI_U16 u16Unknown012;
} AWB_ZONE_WORK_S; /* 0x14 */

typedef enum hiAWB_ALGORITHM_FLAG_E
{
    AWB_ALG_FLAG_CCT_CLIP                  = 0x01,
    AWB_ALG_FLAG_USE_CONFIGURED_CLIP_GAINS = 0x02,
    AWB_ALG_FLAG_PLANCK_OFFSET_CLIP         = 0x04
} AWB_ALGORITHM_FLAG_E;

typedef struct hiAWB_WB_CTX_S
{
    /* VREG +0x000 bit 3. */
    /* 0x000 */ HI_U8  u8Enable;

    /* Set whenever a watched VREG value changes. */
    /* 0x001 */ HI_U8  bConfigChanged;
    /* Scales (Rgain - unity) by value/128 near the final output. */
    /* 0x002 */ HI_U8  u8RgainStrength;
    /* Printed as "Zones", but mathematically scales Bgain by value/128. */
    /* 0x003 */ HI_U8  u8BgainStrength;
    /* 0x004 */ HI_U8  u8Unknown004;
    /* 0x005 */ HI_U8  u8Unknown005; /* VREG +0x00e */
    /* 0x006 */ HI_U8  u8Unknown006; /* VREG +0x00d */
    /* 0x007 */ HI_U8  u8Padding007;

    /* Working copy of AWB_SENSOR_DEFAULT_S.u16WbRefTemp. */
    /* 0x008 */ HI_U16 u16WbRefTemp;
    /* 0x00a */ HI_U16 u16Padding00A;

    /* Minimum global population/sum required to update automatic gains. */
    /* VREG is 16-bit; storage width here is now confirmed as 16-bit. */
    /* 0x00c */ HI_U16 u16GlobalSumThreshold;
    /* 0x00e */ HI_U16 u16Padding00E;

    /*
     * CCT clipping limits in units of 100 K.  Defaults 100 and 25
     * correspond to 10000 K and 2500 K; VREGs themselves are U8.
     */
    /* 0x010 */ HI_U32 u32MaxColorTemp100K;
    /* 0x014 */ HI_U32 u32MinColorTemp100K;

    /* Four Bayer gains used by AwbRun in manual mode: R, Gr, Gb, B. */
    /* 0x018 */ HI_U16 au16ManualGain[4];

    /* Copied from AWB_SENSOR_DEFAULT_S.au16GainOffset by AwbWbGainSet. */
    /* 0x020 */ HI_U16 au16GainOffset[4];

    /* Copied by AwbWbParaSet; a1 is forced to 1 when supplied as zero. */
    /* 0x028 */ HI_S32 as32WbPara[6]; /* p2, p1, q1, a1, b1, c1 */

    /* Age/counter of the cached reference-zone scene, saturated at 0x7fff. */
    /* 0x040 */ HI_U32 u32ReferenceAge;

    /* Global valid-data population/sum exported to the debug record. */
    /* 0x044 */ HI_U32 u32GlobalSum;

    /* Working automatic gains, Q8, updated by AwbCoefClip. */
    /* 0x048 */ HI_U16 u16Rgain;
    /* 0x04a */ HI_U16 u16Bgain;

    /* Signed displacement from the calibrated Planckian curve. */
    /* 0x04c */ HI_S16 s16PlanckOffset;

    /* Current calculated color temperature. Exported to VREG + 0x012. */
    /* 0x04e */ HI_U16 u16ColorTemp;

    /* Q12 AWB convergence coefficient, nominal range 0..0x1000. */
    /* 0x050 */ HI_U16 u16Speed;

    /* Two complete 8-byte matrix rows initialized by AwbWbMatrixInit. */
    /* 0x052 */ AWB_WB_MATRIX_ROW_S astRgbMatrix[2];

    /* Third initialized Q8 matrix gain; following fields are not a row. */
    /* 0x062 */ HI_U16 u16ThirdMatrixGain;
    /* 0x064 */ HI_U16 u16Unknown064;
    /* 0x066 */ HI_U16 u16Unknown066;

    /* Reciprocal color temperature (mired) in Q8. */
    /* Kelvin = 1000000 / (u32MiredQ8 >> 8). */
    /* 0x068 */ HI_U32 u32MiredQ8;

    /* Temporally filtered target gains, Q16, initialized to unity. */
    /* 0x06c */ HI_U32 u32FilteredRgain;
    /* 0x070 */ HI_U32 u32FilteredBgain;

    /* Bits assembled from separate one-byte VREG switches. */
    /* 0x074 */ HI_U8  u8StatusFlags;
    /* 0x075 */ HI_U8  u8AlgorithmFlags;
    /* Previous combined flag word; changes set bConfigChanged. */
    /* 0x076 */ HI_U16 u16PreviousFlags;

    /* Ten aligned configuration slots; several receive U8 VREGs. */
    /* 0x078 */ HI_U16 au16ConfigParam[10];

    /* Global gain pair at the last reference-zone cache refresh. */
    /* 0x08c */ HI_U16 u16ReferenceRgain;
    /* 0x08e */ HI_U16 u16ReferenceBgain;

    /* Current gains saved immediately before AwbWbMatrixNormalize. */
    /* 0x090 */ HI_U16 u16UnnormalizedRgain;
    /* 0x092 */ HI_U16 u16UnnormalizedBgain;

    /* Spatial indices and classes of 20 scene-change reference zones. */
    /* 0x094 */ HI_U16 au16ReferenceZoneIndex[20];
    /* 0x0bc */ HI_U8  au8ReferenceZoneClass[20];

    /* Cached 20-byte records selected from the current 255-zone array. */
    /* 0x0d0 */ AWB_ZONE_WORK_S astReferenceZone[20];

    /* Previous global estimate used by the scene stability detector. */
    /* 0x260 */ HI_U16 u16PrevGlobalRgain;
    /* 0x262 */ HI_U16 u16PrevGlobalBgain;
    /* 0x264 */ HI_U32 u32PrevGlobalSum;
    /* 0x268 */ HI_U16 u16StableFrameCount;
    /* Minimum stable frames before zonal processing; VREG +0x195. */
    /* 0x26a */ HI_U8  u8StableFrameThreshold;
    /* 0x26b */ HI_U8  au8Padding26B[5];
    /* 0x270 */ HI_U32 u32Unknown270; /* VREG +0x1ac */
    /* 0x274 */ HI_U32 u32Unknown274; /* initialized to 17280 (0x4380) */
    /* Separate WB-side snapshot of VREG ISO. */
    /* 0x278 */ HI_U32 u32Iso;
    /* 0x27c */ HI_U32 u32Unknown27C; /* VREG +0x1b0 */
    /* 0x280 */ HI_U32 u32Unknown280;

    /* Four nonzero limits from VREG +0x1b4..+0x1ba. */
    /* Stored order differs from VREG address order; semantics unknown. */
    /* 0x284 */ HI_U16 au16RangeLimit[4];
    /* 0x28c */ HI_U8  au8Unknown28C[8];

    /* VREG +0x170, +0x178, +0x180 and +0x188. */
    /* 0x294 */ AWB_WB_RULE_S astRule[4];

    /*
     * Q8 target R/B gains used outside the configured CCT interval:
     * [0..1] high-CCT side, [2..3] low-CCT side.
     */
    /* 0x2b4 */ HI_U16 au16CctClipGain[4];
    /* 0x2bc */ HI_U16 au16Unknown2BC[2];
    /* 0x2c0 */ HI_U32 u32Unknown2C0;
    /* 0x2c4 */ HI_U8  au8Unknown2C4[0x04];
} AWB_WB_CTX_S; /* 0x2c8 */

typedef struct hiAWB_CM_CTX_S
{
    /* VREG +0x000 bits [5:4], range 0..3; nonzero selects manual gains. */
    /* 0x00 */ HI_U8  u8ManualMode;
    /* 0x01 */ HI_U8  au8Padding001[3];

    /* Selects u8SatTarget instead of automatic saturation-by-ISO. */
    /* 0x04 */ HI_BOOL bSatManualEnable;
    /* 0x08 */ HI_U8  u8SatTarget;
    /* Current calculated saturation; output of AwbSaturationCalculate. */
    /* 0x09 */ HI_U8  u8Saturation;
    /* 0x0a */ HI_U8  au8Padding00A[2];

    /* Hysteretic five-state selection of low/mid/high CCM regions. */
    /* 0x0c */ AWB_CCM_REGION_E enCcmRegion;
    /* 0x10 */ AWB_CCM_REGION_E enCcmRegionNew;

    /*
     * Updated from stWb.u16ColorTemp before AwbCcMatrixCalculate.
     */
    /* 0x14 */ HI_U16 u16ColorTemp;
    /* 0x16 */ HI_U16 u16Padding016;

    /* Smoothed/working color temperature in Kelvin Q8. */
    /* 0x18 */ HI_U32 u32ColorTempQ8;

    /* Saturation values for ISO 100..12800 in octave steps. */
    /* 0x1c */ HI_U8  au8SaturationTable[8];

    /* Input to automatic saturation selection. */
    /* 0x24 */ HI_U32 u32Iso;

    /* Three signed Q8 calibration matrices and their temperatures. */
    /* 0x28 */ AWB_CCM_INTERNAL_S stCcm;

    /* Final CCM in public direct/sign-magnitude representation. */
    /* 0x64 */ HI_U16 au16OutputCCM[9];

    /* Base/interpolated CCM in internal signed Q8 representation. */
    /* 0x76 */ HI_S16 as16InterpolatedCCM[9];

    /* RGB saturation matrix, internal signed Q8. */
    /* 0x88 */ HI_S16 as16SaturationMatrix[9];

    /* 0x9a */ HI_U8  au8Unknown09A[0x0a];
} AWB_CM_CTX_S; /* 0x0a4 */

typedef struct hiAWB_CTX_S
{
    /* 0x000 */ HI_BOOL bInitialized;
    /* 0x004 */ HI_U32  u32Unknown004;
    /* 0x008 */ HI_U32  u32Unknown008;

    /* Requested/applied WDR pair; exact source-level names unknown. */
    /* 0x00c */ ISP_WDR_MODE_E enAppliedWdrMode;
    /* 0x010 */ ISP_WDR_MODE_E enRequestedWdrMode;

    /* 0x014 */ AWB_WB_CTX_S stWb;
    /* 0x2dc */ AWB_CM_CTX_S stCm;

    /* 0x380 */ HI_U32 u32FrameCnt;
    /* 0x384 */ HI_U8  au8Unknown384[0x0c];

    /* Cached output copied verbatim by AwbRun. */
    /* 0x390 */ ISP_AWB_RESULT_S stResult;

    /* 0x3c4 */ HI_BOOL bSnsRegistered;
    /* 0x3c8 */ SENSOR_ID SensorId;

    /* Refreshed by the sensor callback at registration and WDR changes. */
    /* 0x3cc */ AWB_SENSOR_DEFAULT_S stSnsDefault;
    /* 0x438 */ AWB_SENSOR_REGISTER_S stSnsRegister;

    /* 0x43c */ AWB_DEBUG_CTX_S stDebug;
} AWB_CTX_S; /* 0x464 */

/* Expected declaration of the library-global storage. */
extern AWB_CTX_S g_astAwbCtx[2];

/* Reconstructed helper prototypes useful when importing this file in IDA. */
HI_U32 AwbSqrt32(HI_U32 u32Value);

HI_U16 AwbDirectToComplement(HI_U16 u16Direct);
HI_U16 AwbComplementToDirect(HI_U16 u16Complement);

HI_VOID AwbMatrixIdentity(HI_S16 *ps16Matrix, HI_S32 s32Size);
HI_VOID AwbMatrixCopy(const HI_S16 *ps16Source,
                      HI_S16 *ps16Destination,
                      HI_S32 s32Rows,
                      HI_S32 s32Columns);
HI_VOID AwbMatrixMultiply(const HI_S16 *ps16Left,
                          const HI_S16 *ps16Right,
                          HI_S16 *ps16Output,
                          HI_S32 s32LeftRows,
                          HI_S32 s32CommonSize,
                          HI_S32 s32RightColumns);

HI_VOID AwbToRgBg(const AWB_WB_CTX_S *pstWb,
                  HI_S32 s32MiredQ8,
                  HI_S32 s32PlanckOffset,
                  HI_U16 *pu16Rg,
                  HI_U16 *pu16Bg);
HI_VOID AwbToMired(const AWB_WB_CTX_S *pstWb,
                   HI_U16 u16Rg,
                   HI_U16 u16Bg,
                   HI_U32 *pu32MiredQ8,
                   HI_S16 *ps16PlanckOffset);
HI_VOID AwbCoefClip(AWB_WB_CTX_S *pstWb,
                    HI_S32 s32MaxPlanckOffset);
HI_U8 AwbGlobalRelateCoef(HI_U32 u32Rg,
                          HI_U32 u32Bg,
                          HI_U32 u32ReferenceRg,
                          HI_U32 u32ReferenceBg);

HI_VOID AwbCmInit(AWB_CM_CTX_S *pstCm,
                  const AWB_SENSOR_DEFAULT_S *pstSensorDefault);
HI_S32 AwbDbgSet(HI_S32 s32Handle,
                 const AWB_DEBUG_INFO_S *pstDebugInfo);
HI_S32 AwbDbgGet(HI_S32 s32Handle,
                 AWB_DEBUG_INFO_S *pstDebugInfo);
HI_S32 AwbExit(HI_S32 s32Handle);
HI_S32 AwbCtrl(HI_S32 s32Handle, HI_U32 u32Cmd, HI_VOID *pValue);

#endif /* AWB_CTX_IDA_H */
