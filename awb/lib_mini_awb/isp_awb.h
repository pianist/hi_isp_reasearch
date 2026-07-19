#ifndef ISP_AWB_H
#define ISP_AWB_H

/*
 * Verbose entry-point and control-command tracing.
 *
 * Keep this enabled while bringing up the replacement library.  It may be
 * disabled without editing this file by compiling with:
 *
 *     -DAWB_VERBOSE_DEBUG=0
 */
#ifndef AWB_VERBOSE_DEBUG
#define AWB_VERBOSE_DEBUG 1
#endif

#if AWB_VERBOSE_DEBUG
#define AWB_DEBUG_PRINT(...)                                                \
    do {                                                                    \
        (void)fprintf(stderr, __VA_ARGS__);                                 \
    } while (0)
#else
#define AWB_DEBUG_PRINT(...) do { } while (0)
#endif

/*
 * Standalone ABI declarations for the replacement lib_hiawb.so.
 *
 * No HiSilicon SDK header is required.  The declarations below describe the
 * actual 32-bit ARM ABI reconstructed from the ISP framework and original
 * lib_hiawb.so.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------- */
/* Basic HiSilicon-compatible scalar types                                   */
/* ------------------------------------------------------------------------- */

typedef signed char        HI_S8;
typedef unsigned char      HI_U8;
typedef signed short       HI_S16;
typedef unsigned short     HI_U16;
typedef signed int         HI_S32;
typedef unsigned int       HI_U32;
typedef char               HI_CHAR;
typedef void               HI_VOID;

/* HI_BOOL and enums occupy one 32-bit ARM word in this SDK. */
typedef HI_S32             HI_BOOL;
typedef HI_S32             SENSOR_ID;

#define HI_NULL             0
#define HI_FALSE            0
#define HI_TRUE             1
#define HI_SUCCESS          0
#define HI_FAILURE          (-1)

/* Error values observed in the original ISP/AWB binaries. */
#define HI_ERR_ISP_ILLEGAL_PARAM ((HI_S32)0xA01C8003u)
#define HI_ERR_ISP_NULL_PTR      ((HI_S32)0xA01C8006u)

#define ALG_LIB_NAME_SIZE   20
#define AWB_MAX_HANDLES     2
#define AWB_ZONE_ROWS       15
#define AWB_ZONE_COLUMNS    17
#define AWB_ZONE_COUNT      (AWB_ZONE_ROWS * AWB_ZONE_COLUMNS)

#define HI_AWB_LIB_NAME     "hisi_awb_lib"

/* Framework commands delivered through pfn_awb_ctrl(). */
#define ISP_WDR_MODE_SET            8000u
#define ISP_PROC_WRITE              8001u
#define ISP_AWB_ISO_SET             8003u
#define ISP_AWB_RUNTIME_PARAM_SET   8005u

typedef enum hiAWB_CTRL_CMD_E
{
    AWB_SATURATION_SET = 0,
    AWB_SATURATION_GET = 1,
    AWB_DEBUG_ATTR_SET = 2,
    AWB_DEBUG_ATTR_GET = 3,
    AWB_CTRL_BUTT
} AWB_CTRL_CMD_E;

/* Public ISP operation mode used by the WB get/set API. */
typedef enum hiISP_OP_TYPE_E
{
    OP_TYPE_AUTO   = 0,
    OP_TYPE_MANUAL = 1,
    OP_TYPE_BUTT
} ISP_OP_TYPE_E;

typedef enum hiISP_AWB_ALG_TYPE_E
{
    AWB_ALG_DEFAULT = 0,
    AWB_ALG_ADVANCE = 1,
    AWB_ALG_BUTT
} ISP_AWB_ALG_TYPE_E;

typedef struct hiISP_AWB_IN_OUT_ATTR_S
{
    /* 0x00 */ HI_BOOL       bEnable;
    /* 0x04 */ ISP_OP_TYPE_E enOpType;
    /* 0x08 */ HI_BOOL       bOutdoorStatus;
    /* 0x0C */ HI_U32        u32OutThresh;
    /* 0x10 */ HI_U16        u16LowStart;
    /* 0x12 */ HI_U16        u16LowStop;
    /* 0x14 */ HI_U16        u16HighStart;
    /* 0x16 */ HI_U16        u16HighStop;
    /* 0x18 */ HI_BOOL       bGreenEnhanceEn;
} ISP_AWB_IN_OUT_ATTR_S; /* 0x1C */

typedef struct hiISP_AWB_CT_LIMIT_ATTR_S
{
    /* 0x00 */ HI_BOOL       bEnable;
    /* 0x04 */ ISP_OP_TYPE_E enOpType;
    /* 0x08 */ HI_U16        u16HighRgLimit;
    /* 0x0A */ HI_U16        u16HighBgLimit;
    /* 0x0C */ HI_U16        u16LowRgLimit;
    /* 0x0E */ HI_U16        u16LowBgLimit;
} ISP_AWB_CT_LIMIT_ATTR_S; /* 0x10 */

typedef struct hiISP_ADV_AWB_ATTR_S
{
    /* 0x00 */ HI_BOOL bAccuPrior;
    /* 0x04 */ HI_U8   u8Tolerance;
    /* 0x05 */ HI_U8   u8Padding005;
    /* 0x06 */ HI_U16  u16CurveLLimit;
    /* 0x08 */ HI_U16  u16CurveRLimit;
    /* 0x0A */ HI_U8   au8Padding00A[2];
    /* 0x0C */ HI_BOOL bGainNormEn;
    /* 0x10 */ ISP_AWB_IN_OUT_ATTR_S   stInOrOut;
    /* 0x2C */ ISP_AWB_CT_LIMIT_ATTR_S stCTLimit;
} ISP_ADV_AWB_ATTR_S; /* 0x3C */

/* ------------------------------------------------------------------------- */
/* ISP algorithm library descriptor                                         */
/* ------------------------------------------------------------------------- */

typedef struct hiALG_LIB_S
{
    /* 0x00 */ HI_S32  s32Id;
    /* 0x04 */ HI_CHAR acLibName[ALG_LIB_NAME_SIZE];
} ALG_LIB_S; /* 0x18 */

/* ------------------------------------------------------------------------- */
/* Sensor -> AWB calibration ABI                                             */
/* ------------------------------------------------------------------------- */

typedef struct hiAWB_CCM_S
{
    /* 0x00 */ HI_U16 u16HighColorTemp;
    /* 0x02 */ HI_U16 au16HighCCM[9];
    /* 0x14 */ HI_U16 u16MidColorTemp;
    /* 0x16 */ HI_U16 au16MidCCM[9];
    /* 0x28 */ HI_U16 u16LowColorTemp;
    /* 0x2A */ HI_U16 au16LowCCM[9];
} AWB_CCM_S; /* 0x3C */

typedef struct hiAWB_AGC_TABLE_S
{
    /* 0x00 */ HI_BOOL bValid;
    /* 0x04 */ HI_U8   au8Saturation[8];
} AWB_AGC_TABLE_S; /* 0x0C */

typedef struct hiAWB_SENSOR_DEFAULT_S
{
    /* 0x00 */ HI_U16 u16WbRefTemp;
    /* 0x02 */ HI_U16 au16GainOffset[4]; /* R, Gr, Gb, B; unsigned Q8 */
    /* 0x0A */ HI_U8  au8Padding00A[2];
    /* 0x0C */ HI_S32 as32WbPara[6];     /* p2, p1, q1, a1, b1, c1 */
    /* 0x24 */ AWB_AGC_TABLE_S stAgcTbl;
    /* 0x30 */ AWB_CCM_S       stCcm;
} AWB_SENSOR_DEFAULT_S; /* 0x6C */

typedef struct hiAWB_SENSOR_EXP_FUNC_S
{
    HI_S32 (*pfn_cmos_get_awb_default)(
        AWB_SENSOR_DEFAULT_S *pstAwbSnsDft);
} AWB_SENSOR_EXP_FUNC_S; /* 0x04 on ARM32 */

typedef struct hiAWB_SENSOR_REGISTER_S
{
    AWB_SENSOR_EXP_FUNC_S stSnsExp;
} AWB_SENSOR_REGISTER_S; /* 0x04 on ARM32 */

/* ------------------------------------------------------------------------- */
/* ISP framework -> AWB callback ABI                                         */
/* ------------------------------------------------------------------------- */

/* Actual init parameter object constructed by this framework revision. */
typedef struct hiISP_AWB_PARAM_S
{
    /* 0x00 */ SENSOR_ID SensorId;
} ISP_AWB_PARAM_S; /* 0x04 */

/*
 * Despite the historical Rg/Bg names, these are correction ratios G/R and
 * G/B in unsigned Q4.8.  AwbWbMatrixCalculate multiplies the current R/B
 * correction gains by the corresponding input ratio.
 */
typedef struct hiISP_AWB_STAT_1_S
{
    /* 0x00 */ HI_U16 u16RgRatio;          /* G/R, MMIO 0x205A0658 */
    /* 0x02 */ HI_U16 u16BgRatio;          /* G/B, MMIO 0x205A065C */
    /* 0x04 */ HI_U32 u32WhitePointCount;  /* MMIO 0x205A0660 */
} ISP_AWB_STAT_1_S; /* 0x08 */

typedef struct hiISP_AWB_STAT_2_S
{
    /* 0x000 */ HI_U16 au16RgRatio[AWB_ZONE_COUNT];
    /* 0x1FE */ HI_U16 au16BgRatio[AWB_ZONE_COUNT];
    /* 0x3FC */ HI_U16 au16WhitePointCount[AWB_ZONE_COUNT];
} ISP_AWB_STAT_2_S; /* 0x5FA */

typedef struct hiISP_AWB_INFO_S
{
    /* 0x00 */ HI_U32 u32FrameCnt;
    /* 0x04 */ ISP_AWB_STAT_1_S *pstAwbStat1; /* stat buffer + 0x0C08 */
    /* 0x08 */ ISP_AWB_STAT_2_S *pstAwbStat2; /* stat buffer + 0x0C10 */
} ISP_AWB_INFO_S; /* 0x0C on ARM32 */

typedef struct hiISP_AWB_STAT_ATTR_S
{
    /* 0x00 */ HI_BOOL bChange;
    /* 0x04 */ HI_U16 u16MeteringWhiteLevelAwb;
    /* 0x06 */ HI_U16 u16MeteringBlackLevelAwb;
    /* 0x08 */ HI_U16 u16MeteringCrRefMaxAwb;
    /* 0x0A */ HI_U16 u16MeteringCrRefMinAwb;
    /* 0x0C */ HI_U16 u16MeteringCbRefMaxAwb;
    /* 0x0E */ HI_U16 u16MeteringCbRefMinAwb;
} ISP_AWB_STAT_ATTR_S; /* 0x10 */

typedef struct hiISP_AWB_RESULT_S
{
    /* 0x00 */ HI_U32 au32WhiteBalanceGain[4]; /* R, Gr, Gb, B; Q16 */
    /* 0x10 */ HI_U16 au16ColorMatrix[9];      /* direct/sign Q8 */
    /* 0x22 */ HI_U16 u16Padding022;
    /* 0x24 */ ISP_AWB_STAT_ATTR_S stStatAttr;
} ISP_AWB_RESULT_S; /* 0x34 */

typedef HI_S32 (*ISP_AWB_INIT_FN)(
    HI_S32 s32Handle,
    const ISP_AWB_PARAM_S *pstAwbParam);

typedef HI_S32 (*ISP_AWB_RUN_FN)(
    HI_S32 s32Handle,
    const ISP_AWB_INFO_S *pstAwbInfo,
    ISP_AWB_RESULT_S *pstAwbResult,
    HI_S32 s32Reserved);

typedef HI_S32 (*ISP_AWB_CTRL_FN)(
    HI_S32 s32Handle,
    HI_U32 u32Cmd,
    HI_VOID *pValue);

typedef HI_S32 (*ISP_AWB_EXIT_FN)(HI_S32 s32Handle);

typedef struct hiISP_AWB_EXP_FUNC_S
{
    /* 0x00 */ ISP_AWB_INIT_FN pfn_awb_init;
    /* 0x04 */ ISP_AWB_RUN_FN  pfn_awb_run;
    /* 0x08 */ ISP_AWB_CTRL_FN pfn_awb_ctrl;
    /* 0x0C */ ISP_AWB_EXIT_FN pfn_awb_exit;
} ISP_AWB_EXP_FUNC_S; /* 0x10 on ARM32 */

typedef struct hiISP_AWB_REGISTER_S
{
    ISP_AWB_EXP_FUNC_S stAwbExpFunc;
} ISP_AWB_REGISTER_S; /* 0x10 on ARM32 */

/* ------------------------------------------------------------------------- */
/* Private state shared by the replacement library modules                   */
/* ------------------------------------------------------------------------- */

typedef struct hiAWB_STUB_CTX_S
{
    HI_BOOL bSensorRegistered;
    HI_BOOL bInitialized;
    SENSOR_ID SensorId;
    HI_BOOL bWdrMode;

    HI_U32 u32Iso;
    HI_U32 u32RuntimeParam8005;
    HI_U32 u32Saturation;
    ISP_OP_TYPE_E enWbType;
    ISP_AWB_ALG_TYPE_E enAwbAlgType;
    ISP_ADV_AWB_ATTR_S stAdvAwbAttr;

    AWB_SENSOR_REGISTER_S stSensorRegister;
    AWB_SENSOR_DEFAULT_S  stSensorDefault;
    ISP_AWB_RESULT_S      stResult;
} AWB_STUB_CTX_S;

extern AWB_STUB_CTX_S g_astAwbStubCtx[AWB_MAX_HANDLES];

/* Rebuild the constant output after sensor defaults have changed. */
HI_VOID AwbPrepareDefaultResult(AWB_STUB_CTX_S *pstCtx);

/* ------------------------------------------------------------------------- */
/* Replacement algorithm callbacks                                          */
/* ------------------------------------------------------------------------- */

HI_S32 AwbInit(
    HI_S32 s32Handle,
    const ISP_AWB_PARAM_S *pstAwbParam);

HI_S32 AwbRun(
    HI_S32 s32Handle,
    const ISP_AWB_INFO_S *pstAwbInfo,
    ISP_AWB_RESULT_S *pstAwbResult,
    HI_S32 s32Reserved);

HI_S32 AwbCtrl(
    HI_S32 s32Handle,
    HI_U32 u32Cmd,
    HI_VOID *pValue);

HI_S32 AwbExit(HI_S32 s32Handle);

/* ------------------------------------------------------------------------- */
/* Public lib_hiawb registration API                                         */
/* ------------------------------------------------------------------------- */

HI_S32 HI_MPI_AWB_Register(ALG_LIB_S *pstAwbLib);
HI_S32 HI_MPI_AWB_UnRegister(ALG_LIB_S *pstAwbLib);

HI_S32 HI_MPI_AWB_SensorRegCallBack(
    ALG_LIB_S *pstAwbLib,
    SENSOR_ID SensorId,
    AWB_SENSOR_REGISTER_S *pstRegister);

HI_S32 HI_MPI_AWB_SensorUnRegCallBack(
    ALG_LIB_S *pstAwbLib,
    SENSOR_ID SensorId);

/* Public ISP AWB controls implemented locally by the replacement library. */
HI_S32 HI_MPI_ISP_SetWBType(ISP_OP_TYPE_E enWBType);
HI_S32 HI_MPI_ISP_GetWBType(ISP_OP_TYPE_E *penWBType);
HI_S32 HI_MPI_ISP_SetAdvAWBAttr(ISP_ADV_AWB_ATTR_S *pstAdvAWBAttr);
HI_S32 HI_MPI_ISP_GetAdvAWBAttr(ISP_ADV_AWB_ATTR_S *pstAdvAWBAttr);
HI_S32 HI_MPI_ISP_SetAWBAlgType(ISP_AWB_ALG_TYPE_E enALGType);
HI_S32 HI_MPI_ISP_GetAWBAlgType(ISP_AWB_ALG_TYPE_E *penALGType);

/* Imports supplied by the closed ISP framework library. */
HI_S32 HI_MPI_ISP_AwbLibRegCallBack(
    ALG_LIB_S *pstAwbLib,
    ISP_AWB_REGISTER_S *pstRegister);

HI_S32 HI_MPI_ISP_AwbLibUnRegCallBack(ALG_LIB_S *pstAwbLib);

/* Compile-time ABI checks, enabled only when pointers are 32-bit. */
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L) && \
    defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ == 4)
_Static_assert(sizeof(ALG_LIB_S)               == 0x18, "ALG_LIB_S");
_Static_assert(sizeof(AWB_CCM_S)               == 0x3C, "AWB_CCM_S");
_Static_assert(sizeof(AWB_AGC_TABLE_S)         == 0x0C, "AWB_AGC_TABLE_S");
_Static_assert(sizeof(AWB_SENSOR_DEFAULT_S)    == 0x6C, "AWB_SENSOR_DEFAULT_S");
_Static_assert(sizeof(ISP_AWB_PARAM_S)         == 0x04, "ISP_AWB_PARAM_S");
_Static_assert(sizeof(ISP_AWB_STAT_1_S)        == 0x08, "ISP_AWB_STAT_1_S");
_Static_assert(sizeof(ISP_AWB_STAT_2_S)        == 0x5FA, "ISP_AWB_STAT_2_S");
_Static_assert(sizeof(ISP_AWB_INFO_S)          == 0x0C, "ISP_AWB_INFO_S");
_Static_assert(sizeof(ISP_AWB_STAT_ATTR_S)     == 0x10, "ISP_AWB_STAT_ATTR_S");
_Static_assert(sizeof(ISP_AWB_RESULT_S)        == 0x34, "ISP_AWB_RESULT_S");
_Static_assert(sizeof(ISP_AWB_EXP_FUNC_S)      == 0x10, "ISP_AWB_EXP_FUNC_S");
_Static_assert(sizeof(ISP_AWB_REGISTER_S)      == 0x10, "ISP_AWB_REGISTER_S");
_Static_assert(sizeof(ISP_OP_TYPE_E)           == 0x04, "ISP_OP_TYPE_E");
_Static_assert(sizeof(ISP_AWB_ALG_TYPE_E)      == 0x04, "ISP_AWB_ALG_TYPE_E");
_Static_assert(sizeof(ISP_AWB_IN_OUT_ATTR_S)   == 0x1C, "ISP_AWB_IN_OUT_ATTR_S");
_Static_assert(sizeof(ISP_AWB_CT_LIMIT_ATTR_S) == 0x10, "ISP_AWB_CT_LIMIT_ATTR_S");
_Static_assert(sizeof(ISP_ADV_AWB_ATTR_S)      == 0x3C, "ISP_ADV_AWB_ATTR_S");
#endif

#ifdef __cplusplus
}
#endif

#endif /* ISP_AWB_H */
