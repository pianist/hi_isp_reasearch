#ifndef JUNK_IDA_ISP_CTX_H
#define JUNK_IDA_ISP_CTX_H

/*
 * Legacy HiSilicon ISP: unified userspace/kernel IDA reconstruction.
 * Target ABI: ARM32, 4-byte pointers and 4-byte enums.
 *
 * Contains the userspace ISP_CTX_S, the shared statistics ABI, the userspace
 * and kernel register-config layouts, and the hi3518_isp.ko ISP_DRV_CTX_S.
 * Names containing Field/Unknown
 * intentionally preserve uncertainty.  All stated offsets and sizes are
 * relative to the target ABI.
 */

typedef signed int         HI_S32;
typedef unsigned int       HI_U32;
typedef unsigned short     HI_U16;
typedef unsigned char      HI_U8;
typedef char               HI_CHAR;
typedef HI_S32             HI_BOOL;
typedef HI_S32             ISP_DEV;
typedef HI_S32             SENSOR_ID;

#define ISP_MAX_DEV_NUM       2
#define ISP_MAX_ALGS          32
#define ISP_MAX_AE_LIBS       2
#define ISP_MAX_AWB_LIBS      2
#define ISP_MAX_AF_LIBS       2
#define ALG_LIB_NAME_SIZE     20
#define AE_ZONE_ROW           15
#define AE_ZONE_COLUMN        17
#define AE_ZONE_NUM           255
#define AWB_ZONE_NUM          255
#define ISP_STAT_BUF_SIZE     0x141C
#define ISP_STAT_BUF_NUM      2
#define ISP_STAT_MMAP_SIZE    0x2838

typedef struct ALG_LIB_S
{
    HI_S32  s32Id;                         /* 0x00 */
    HI_CHAR acLibName[ALG_LIB_NAME_SIZE];  /* 0x04 */
} ALG_LIB_S;                               /* sizeof = 0x18 */

/* ------------------------------------------------------------------------- */
/* Statistics ABI                                                           */
/* ------------------------------------------------------------------------- */

typedef union ISP_STAT_KEY_U
{
    struct {
        HI_U32 bit1AeGlobalHist : 1;       /* bit 0 */
        HI_U32 bit1AeZonalHist  : 1;       /* bit 1 */
        HI_U32 bit1Ae256BinHist : 1;       /* bit 2 */
        HI_U32 bit1AwbGlobal    : 1;       /* bit 3 */
        HI_U32 bit1AwbZonal     : 1;       /* bit 4 */
        HI_U32 bit1Af           : 1;       /* bit 5 */
        HI_U32 bit1DefectCount  : 1;       /* bit 6 */
        HI_U32 bit1WbGain       : 1;       /* bit 7 */
        HI_U32 reserved         : 24;
    } bits;
    HI_U32 u32Key;
} ISP_STAT_KEY_U;

typedef struct ISP_AE_ZONE_HIST_S
{
    HI_U16 u16Seg0;                        /* 0x00 */
    HI_U16 u16Seg1;                        /* 0x02 */
    HI_U16 u16Seg23;                       /* 0x04, calculated by driver */
    HI_U16 u16Seg4;                        /* 0x06 */
    HI_U16 u16Seg5;                        /* 0x08 */
} ISP_AE_ZONE_HIST_S;                       /* sizeof = 0x0A */

typedef struct ISP_AE_STAT_1_S
{
    HI_U8  u8Thresh01;                     /* 0x00 */
    HI_U8  u8Thresh12;                     /* 0x01 */
    HI_U8  u8Thresh34;                     /* 0x02 */
    HI_U8  u8Thresh45;                     /* 0x03 */
    HI_U16 au16OuterHist[4];               /* 0x04 */
} ISP_AE_STAT_1_S;                          /* sizeof = 0x00C */

typedef struct ISP_AE_STAT_2_S
{
    HI_U8 u8Thresh01;                      /* 0x000 */
    HI_U8 u8Thresh12;                      /* 0x001 */
    HI_U8 u8Thresh34;                      /* 0x002 */
    HI_U8 u8Thresh45;                      /* 0x003 */
    ISP_AE_ZONE_HIST_S astZone[AE_ZONE_NUM]; /* 0x004 */
} ISP_AE_STAT_2_S;                          /* sizeof = 0x9FA */

typedef struct ISP_AE_STAT_3_S
{
    HI_U16 au16Hist[256];                  /* MMIO 0x205A1800 + 4*bin */
} ISP_AE_STAT_3_S;                          /* sizeof = 0x200 */

typedef struct ISP_AWB_STAT_1_S
{
    HI_U16 u16RgRatio;                     /* 0x00, unsigned Q4.8 */
    HI_U16 u16BgRatio;                     /* 0x02, unsigned Q4.8 */
    HI_U32 u32WhitePointCount;             /* 0x04 */
} ISP_AWB_STAT_1_S;                         /* sizeof = 0x008 */

typedef struct ISP_AWB_STAT_2_S
{
    HI_U16 au16RgRatio[AWB_ZONE_NUM];          /* 0x000, unsigned Q4.8 */
    HI_U16 au16BgRatio[AWB_ZONE_NUM];          /* 0x1FE, unsigned Q4.8 */
    HI_U16 au16WhitePointCount[AWB_ZONE_NUM];  /* 0x3FC */
} ISP_AWB_STAT_2_S;                            /* sizeof = 0x5FA */

typedef struct ISP_AF_STAT_S
{
    HI_U16 u16Metrics;                     /* 0x000, 0x205A0680 */
    HI_U16 u16ThresholdRead;               /* 0x002, 0x205A0688 low */
    HI_U16 u16ThresholdWrite;              /* 0x004, 0x205A0684 */
    HI_U16 u16Intensity;                   /* 0x006, 0x205A0688 high */
    HI_U8  u8MetricsShift;                 /* 0x008, 0x205A068C[3:0] */
    HI_U8  u8NoiseProfileOffset;           /* 0x009, unsigned Q4.4 */
    HI_U16 au16ZoneMetrics[AE_ZONE_NUM];   /* 0x00A */
} ISP_AF_STAT_S;                            /* sizeof = 0x208 */

typedef struct ISP_WB_GAIN_S
{
    HI_U16 u16R;                           /* 0x00, unsigned Q4.8 */
    HI_U16 u16Gr;                          /* 0x02, unsigned Q4.8 */
    HI_U16 u16Gb;                          /* 0x04, unsigned Q4.8 */
    HI_U16 u16B;                           /* 0x06, unsigned Q4.8 */
} ISP_WB_GAIN_S;                            /* sizeof = 0x008 */

typedef struct ISP_STAT_BUFFER_S
{
    ISP_AE_STAT_1_S  stAeGlobalHist;       /* 0x0000, 0x00C */
    ISP_AE_STAT_2_S  stAeZonalHist;        /* 0x000C, 0x9FA */
    ISP_AE_STAT_3_S  stAe256BinHist;       /* 0x0A06, 0x200 */
    HI_U16           u16ReservedC06;       /* 0x0C06 */
    ISP_AWB_STAT_1_S stAwbGlobal;          /* 0x0C08, 0x008 */
    ISP_AWB_STAT_2_S stAwbZonal;           /* 0x0C10, 0x5FA */
    ISP_AF_STAT_S    stAf;                 /* 0x120A, 0x208 */
    HI_U16           u16DefectPixelCount;  /* 0x1412, HP_COUNT[9:0] */
    ISP_WB_GAIN_S    stWbGain;             /* 0x1414, 0x008 */
} ISP_STAT_BUFFER_S;                        /* sizeof = 0x141C */

typedef ISP_STAT_BUFFER_S ISP_STAT_S;

typedef struct ISP_STAT_MMZ_S
{
    ISP_STAT_BUFFER_S astBuffer[ISP_STAT_BUF_NUM];
} ISP_STAT_MMZ_S;                           /* sizeof = 0x2838 */

typedef struct ISP_STAT_INFO_S
{
    ISP_STAT_KEY_U    unKey;               /* 0x00 */
    HI_U32            u32Unknown04;        /* 0x04 */
    HI_U32            u32PhyAddr;          /* 0x08 */
    ISP_STAT_BUFFER_S *pstVirtAddr;        /* 0x0C */
} ISP_STAT_INFO_S;                          /* sizeof = 0x10 */

typedef struct ISP_STAT_CTX_S
{
    HI_U32         u32PhyBase;             /* 0x00 */
    void          *pVirBase;               /* 0x04 */
    HI_BOOL        bBufAcquired;            /* 0x08 */
    ISP_STAT_INFO_S stStatInfo;             /* 0x0C */
} ISP_STAT_CTX_S;                           /* sizeof = 0x1C */

/* ------------------------------------------------------------------------- */
/* AE/AWB algorithm ABI                                                     */
/* ------------------------------------------------------------------------- */

typedef struct ISP_AE_PARAM_S
{
    HI_S32 s32SensorId;                    /* 0x00 */
    HI_U32 u32Field04;                     /* 0x04, initialized to 512 */
    HI_U32 u32Field08;                     /* 0x08, initialized to 16 */
    HI_U32 u32Field0C;                     /* 0x0C, initialized to 4 */
} ISP_AE_PARAM_S;                           /* sizeof = 0x10 */

typedef struct ISP_AE_INFO_S
{
    HI_U32         u32FrameCnt;            /* 0x00 */
    ISP_AE_STAT_1_S *pstAeStat1;           /* 0x04, stat + 0x000 */
    ISP_AE_STAT_2_S *pstAeStat2;           /* 0x08, stat + 0x00C */
    ISP_AE_STAT_3_S *pstAeStat3;           /* 0x0C, stat + 0xA06 */
} ISP_AE_INFO_S;                            /* sizeof = 0x10 */

typedef struct ISP_AE_STAT_ATTR_S
{
    HI_BOOL bChange;                       /* 0x000 */
    HI_U8   au8HistThresh[4];              /* 0x004 */
    HI_U8   au8Weight[AE_ZONE_NUM];        /* 0x008 */
    HI_U8   u8Padding;                     /* 0x107 */
} ISP_AE_STAT_ATTR_S;                       /* sizeof = 0x108 */

typedef struct ISP_AE_RESULT_S
{
    HI_U32 u32Field00;                     /* 0x000 */
    HI_U32 u32Field04;                     /* 0x004 */
    HI_U32 u32Iso;                         /* 0x008 */
    ISP_AE_STAT_ATTR_S stStatAttr;          /* 0x00C */
} ISP_AE_RESULT_S;                          /* sizeof = 0x114 */

typedef struct ISP_AWB_PARAM_S
{
    HI_S32 s32SensorId;                    /* 0x00 */
} ISP_AWB_PARAM_S;                          /* sizeof = 0x04 */

typedef struct ISP_AWB_INFO_S
{
    HI_U32          u32FrameCnt;           /* 0x00 */
    ISP_AWB_STAT_1_S *pstAwbStat1;         /* 0x04, stat + 0xC08 */
    ISP_AWB_STAT_2_S *pstAwbStat2;         /* 0x08, stat + 0xC10 */
} ISP_AWB_INFO_S;                           /* sizeof = 0x0C */

typedef struct ISP_AWB_STAT_ATTR_S
{
    HI_BOOL bChange;                       /* 0x00 */
    HI_U16  u16MeteringWhiteLevelAwb;      /* 0x04 */
    HI_U16  u16MeteringBlackLevelAwb;      /* 0x06 */
    HI_U16  u16MeteringCrRefMaxAwb;        /* 0x08, max R/G Q4.8 */
    HI_U16  u16MeteringCrRefMinAwb;        /* 0x0A, min R/G Q4.8 */
    HI_U16  u16MeteringCbRefMaxAwb;        /* 0x0C, max B/G Q4.8 */
    HI_U16  u16MeteringCbRefMinAwb;        /* 0x0E, min B/G Q4.8 */
} ISP_AWB_STAT_ATTR_S;                      /* sizeof = 0x10 */

typedef struct ISP_AWB_RESULT_S
{
    HI_U32 au32WhiteBalanceGain[4];        /* 0x00 */
    HI_U16 au16ColorMatrix[9];             /* 0x10 */
    HI_U16 u16Padding22;                   /* 0x22 */
    ISP_AWB_STAT_ATTR_S stStatAttr;         /* 0x24 */
} ISP_AWB_RESULT_S;                         /* sizeof = 0x34 */

/* AF argument structures still need reconstruction. */
typedef struct ISP_AF_PARAM_S  ISP_AF_PARAM_S;
typedef struct ISP_AF_INFO_S   ISP_AF_INFO_S;
typedef struct ISP_AF_RESULT_S ISP_AF_RESULT_S;

/* ------------------------------------------------------------------------- */
/* Userspace register configuration and kernel ioctl payload                 */
/* ------------------------------------------------------------------------- */

#define ISP_REG_CFG_AE_METERING_UPDATE   0x001
#define ISP_REG_CFG_ISP_DGAIN_UPDATE     0x002
#define ISP_REG_CFG_WB_CCM_UPDATE        0x004
#define ISP_REG_CFG_AWB_METERING_UPDATE  0x008
#define ISP_REG_CFG_AF_UPDATE            0x010
#define ISP_REG_CFG_WB_BLACK_UPDATE      0x020
#define ISP_REG_CFG_DRC_UPDATE           0x040
#define ISP_REG_CFG_DNR_DEMOSAIC_UPDATE  0x080
#define ISP_REG_CFG_DEFECT_UPDATE        0x100
#define ISP_REG_CFG_SHARPEN_UPDATE       0x200
#define ISP_REG_CFG_GE_UPDATE            0x400

typedef union ISP_REG_CFG_KEY_U
{
    struct {
        HI_U32 bit1AeMetering       : 1;  /* bit 0 */
        HI_U32 bit1IspDgain         : 1;  /* bit 1 */
        HI_U32 bit1WbGainAndCcm     : 1;  /* bit 2 */
        HI_U32 bit1AwbMetering      : 1;  /* bit 3 */
        HI_U32 bit1Af               : 1;  /* bit 4 */
        HI_U32 bit1WbBlackLevel     : 1;  /* bit 5 */
        HI_U32 bit1DrcStrength      : 1;  /* bit 6 */
        HI_U32 bit1DnrAndDemosaic   : 1;  /* bit 7 */
        HI_U32 bit1DefectPixel      : 1;  /* bit 8 */
        HI_U32 bit1DemosaicSharpen  : 1;  /* bit 9 */
        HI_U32 bit1GreenEqualization: 1;  /* bit 10 */
        HI_U32 reserved             : 21;
    } bits;
    HI_U32 u32Key;
} ISP_REG_CFG_KEY_U;                       /* sizeof = 0x04 */

typedef struct ISP_AE_REG_CFG_S
{
    HI_U8 u8HistThresh01;                  /* 0x000 */
    HI_U8 u8HistThresh12;                  /* 0x001 */
    HI_U8 u8HistThresh34;                  /* 0x002 */
    HI_U8 u8HistThresh45;                  /* 0x003 */
    HI_U8 au8ZoneWeight[AE_ZONE_NUM];      /* 0x004 */
    HI_U8 u8Reserved103;                   /* 0x103 */
} ISP_AE_REG_CFG_S;                        /* sizeof = 0x104 */

typedef struct ISP_DGAIN_PARAMS_S
{
    HI_U32 bUseSqrt;                       /* 0x00, WDR-dependent path */
    HI_U32 u32Gain;                        /* 0x04 */
    HI_U32 u32Shift;                       /* 0x08 */
} ISP_DGAIN_PARAMS_S;                      /* sizeof = 0x0C */

typedef struct ISP_WB_GAIN_CFG_S
{
    HI_U32 u32R;                           /* 0x00 */
    HI_U32 u32Gr;                          /* 0x04 */
    HI_U32 u32Gb;                          /* 0x08 */
    HI_U32 u32B;                           /* 0x0C */
} ISP_WB_GAIN_CFG_S;                       /* sizeof = 0x10 */

typedef struct ISP_DGAIN_CFG_S
{
    ISP_DGAIN_PARAMS_S stParams;           /* 0x00 */
    ISP_WB_GAIN_CFG_S  stWbGain;           /* 0x0C */
} ISP_DGAIN_CFG_S;                         /* sizeof = 0x1C */

typedef struct ISP_CCM_CFG_S
{
    HI_U16 au16Coeff[9];                   /* sign/magnitude Q8.8 */
} ISP_CCM_CFG_S;                           /* sizeof = 0x12 */

typedef struct ISP_AWB_METER_CFG_S
{
    HI_U16 u16WhiteLevel;                  /* 0x00 */
    HI_U16 u16BlackLevel;                  /* 0x02 */
    HI_U16 u16CrRefMax;                    /* 0x04, R/G maximum */
    HI_U16 u16CrRefMin;                    /* 0x06, R/G minimum */
    HI_U16 u16CbRefMax;                    /* 0x08, B/G maximum */
    HI_U16 u16CbRefMin;                    /* 0x0A, B/G minimum */
} ISP_AWB_METER_CFG_S;                     /* sizeof = 0x0C */

typedef struct ISP_AF_REG_CFG_S
{
    HI_U16 u16ThresholdWrite;              /* 0x00 */
    HI_U8  u8MetricsShift;                 /* 0x02 */
    HI_U8  u8NoiseProfileOffset;           /* 0x03 */
} ISP_AF_REG_CFG_S;                        /* sizeof = 0x04 */

typedef struct ISP_DEFECT_PIXEL_REG_CFG_S
{
    HI_U8  bCorrectionEnable;              /* 0x00 */
    HI_U8  bDefectPixelEnable;             /* 0x01 */
    HI_U8  bPointerReset;                  /* 0x02 */
    HI_U8  u8CalibrationDpThreshold;       /* 0x03, low 8 bits written during calibration */
    HI_U8  bDetectionTrigger;              /* 0x04 */
    HI_U8  bShowStaticDefectPixels;        /* 0x05 */
    HI_U16 u16DpSlope;                     /* 0x06, GE_CTRL5.dpslope[11:0] */
    HI_U16 u16DpThreshold;                 /* 0x08, GE_CTRL3.dpthreshold[27:16] */
    HI_U16 u16DefectPixelCountIn;          /* 0x0A */
} ISP_DEFECT_PIXEL_REG_CFG_S;              /* sizeof = 0x0C */

typedef struct ISP_DRC_STRENGTH_CFG_S
{
    HI_U8 u8Strength;                      /* 0x00 */
    HI_U8 u8Reserved01;                    /* 0x01 */
} ISP_DRC_STRENGTH_CFG_S;                  /* sizeof = 0x02 */

typedef struct ISP_DNR_REG_CFG_S
{
    HI_U16 u16ExposureThreshold;           /* 0x00 */
    HI_U8  u8ShortRatio;                   /* 0x02 */
    HI_U8  u8LongRatio;                    /* 0x03 */
    HI_U8  u8ThresholdShort;               /* 0x04 */
    HI_U8  u8ThresholdLong;                /* 0x05 */
} ISP_DNR_REG_CFG_S;                       /* sizeof = 0x06 */

typedef struct ISP_DNR_DEMOSAIC_REG_CFG_S
{
    ISP_DNR_REG_CFG_S stDnr;               /* 0x00 */
    HI_U8 u8DemosaicNoiseProfileOffset;    /* 0x06 */
    HI_U8 u8Reserved07;                    /* 0x07 */
} ISP_DNR_DEMOSAIC_REG_CFG_S;              /* sizeof = 0x08 */

typedef struct ISP_GREEN_EQUALIZATION_CFG_S
{
    HI_U8 u8Strength;                      /* 0x00 */
    HI_U8 u8Reserved01;                    /* 0x01 */
} ISP_GREEN_EQUALIZATION_CFG_S;            /* sizeof = 0x02 */

typedef struct ISP_DEMOSAIC_SHARPEN_CFG_S
{
    signed char s8DirectionalStrength;     /* 0x00 */
    signed char s8FlatStrength;            /* 0x01 */
    HI_U16 u16LuminanceThreshold;          /* 0x02 */
} ISP_DEMOSAIC_SHARPEN_CFG_S;              /* sizeof = 0x04 */

typedef struct ISP_WB_BLACK_LEVEL_CFG_S
{
    HI_U16 u16R;                           /* 0x00 */
    HI_U16 u16Gr;                          /* 0x02 */
    HI_U16 u16Gb;                          /* 0x04 */
    HI_U16 u16B;                           /* 0x06 */
} ISP_WB_BLACK_LEVEL_CFG_S;                /* sizeof = 0x08 */

typedef struct ISP_REG_CFG_S
{
    ISP_REG_CFG_KEY_U            unKey;              /* 0x000 */
    ISP_AE_REG_CFG_S             stAe;               /* 0x004 */
    ISP_DGAIN_CFG_S              stDgainCfg;         /* 0x108 */
    ISP_CCM_CFG_S                stCcm;              /* 0x124 */
    ISP_AWB_METER_CFG_S          stAwbMetering;      /* 0x136 */
    ISP_AF_REG_CFG_S             stAf;               /* 0x142 */
    ISP_DEFECT_PIXEL_REG_CFG_S   stDefectPixel;      /* 0x146 */
    ISP_DRC_STRENGTH_CFG_S       stDrcStrength;      /* 0x152 */
    ISP_DNR_DEMOSAIC_REG_CFG_S   stDnrAndDemosaic;   /* 0x154 */
    ISP_GREEN_EQUALIZATION_CFG_S stGreenEqualization;/* 0x15C */
    ISP_DEMOSAIC_SHARPEN_CFG_S   stDemosaicSharpen;  /* 0x15E */
    ISP_WB_BLACK_LEVEL_CFG_S     stWbBlackLevel;     /* 0x162 */
    HI_U8                        au8Unknown16A[0x10A];/* 0x16A */
} ISP_REG_CFG_S;                              /* sizeof = 0x274 */

/*
 * Exact 0x16C-byte suffix copied by ioctl 0x416C490A.  In userspace the
 * payload starts at &ISP_REG_CFG_S::stDgainCfg (offset 0x108).  The kernel
 * stores two ping-pong copies of this suffix and therefore has neither
 * unKey nor stAe at the start of its configuration bank.
 */
typedef struct ISP_DRV_REG_CFG_S
{
    ISP_DGAIN_CFG_S              stDgainCfg;          /* 0x000 */
    ISP_CCM_CFG_S                stCcm;               /* 0x01C */
    ISP_AWB_METER_CFG_S          stAwbMetering;       /* 0x02E */
    ISP_AF_REG_CFG_S             stAf;                /* 0x03A */
    ISP_DEFECT_PIXEL_REG_CFG_S   stDefectPixel;       /* 0x03E */
    ISP_DRC_STRENGTH_CFG_S       stDrcStrength;        /* 0x04A */
    ISP_DNR_DEMOSAIC_REG_CFG_S   stDnrAndDemosaic;    /* 0x04C */
    ISP_GREEN_EQUALIZATION_CFG_S stGreenEqualization; /* 0x054 */
    ISP_DEMOSAIC_SHARPEN_CFG_S   stDemosaicSharpen;   /* 0x056 */
    ISP_WB_BLACK_LEVEL_CFG_S     stWbBlackLevel;       /* 0x05A */
    HI_U8                        au8Unknown062[0x10A]; /* 0x062 */
} ISP_DRV_REG_CFG_S;                         /* sizeof = 0x16C */

typedef struct ISP_REG_CFG_USER_CTX_S
{
    HI_BOOL      bInitialized;              /* 0x000 */
    ISP_REG_CFG_S stRegCfg;                 /* 0x004 */
} ISP_REG_CFG_USER_CTX_S;                    /* sizeof = 0x278 */

/* Per-device context of the built-in defect-pixel calibration algorithm. */
typedef struct ISP_DP_CTX_S
{
    HI_U8  u8CalibrationDpThreshold;        /* 0x00 */
    HI_U8  u8CalibrationState;              /* 0x01, state machine 0..7 */
    HI_U8  bShowStaticDefectPixels;         /* 0x02 */
    signed char s8CalibrationResult;        /* 0x03, -1/0/1 */
    HI_U8  bCalibrationActive;              /* 0x04 */
    HI_U8  u8TrialCount;                    /* 0x05 */
    HI_U8  u8MaxTrialCount;                 /* 0x06, ext 0x12108 >> 3 */
    HI_U8  u8Padding07;                     /* 0x07 */
    HI_U16 u16DetectedPixelCount;           /* 0x08, statistics + 0x1412 */
    HI_U16 u16Padding0A;                    /* 0x0A */
    HI_BOOL bDefectPixelEnable;              /* 0x0C, GE_CTRL1[2] */
    HI_U16 u16DpSlope;                      /* 0x10, ext 0x12160 */
    HI_U16 u16DpThreshold;                  /* 0x12, ext 0x12162 */
    HI_BOOL bCalibrationTrigger;             /* 0x14, ext 0x10317[0] */
    HI_U32 u32StructSize;                   /* 0x18, initialized/checked as 0x1C */
} ISP_DP_CTX_S;                              /* sizeof = 0x1C */

typedef HI_S32 (*ISP_AE_INIT_FN)(HI_S32, const ISP_AE_PARAM_S *);
typedef HI_S32 (*ISP_AE_RUN_FN)(HI_S32, const ISP_AE_INFO_S *, ISP_AE_RESULT_S *, HI_S32);
typedef HI_S32 (*ISP_AE_CTRL_FN)(HI_S32, HI_U32, void *);
typedef HI_S32 (*ISP_AE_EXIT_FN)(HI_S32);

typedef HI_S32 (*ISP_AWB_INIT_FN)(HI_S32, const ISP_AWB_PARAM_S *);
typedef HI_S32 (*ISP_AWB_RUN_FN)(HI_S32, const ISP_AWB_INFO_S *, ISP_AWB_RESULT_S *, HI_S32);
typedef HI_S32 (*ISP_AWB_CTRL_FN)(HI_S32, HI_U32, void *);
typedef HI_S32 (*ISP_AWB_EXIT_FN)(HI_S32);

typedef HI_S32 (*ISP_AF_INIT_FN)(HI_S32, const ISP_AF_PARAM_S *);
typedef HI_S32 (*ISP_AF_RUN_FN)(HI_S32, const ISP_AF_INFO_S *, ISP_AF_RESULT_S *, HI_S32);
typedef HI_S32 (*ISP_AF_CTRL_FN)(HI_S32, HI_U32, void *);
typedef HI_S32 (*ISP_AF_EXIT_FN)(HI_S32);

typedef struct ISP_AE_LIB_NODE_S
{
    HI_BOOL        bUsed;                  /* 0x00 */
    ALG_LIB_S      stAlgLib;               /* 0x04 */
    ISP_AE_INIT_FN pfnAeInit;              /* 0x1C */
    ISP_AE_RUN_FN  pfnAeRun;               /* 0x20 */
    ISP_AE_CTRL_FN pfnAeCtrl;              /* 0x24 */
    ISP_AE_EXIT_FN pfnAeExit;              /* 0x28 */
} ISP_AE_LIB_NODE_S;                        /* sizeof = 0x2C */

typedef struct ISP_AWB_LIB_NODE_S
{
    HI_BOOL         bUsed;                 /* 0x00 */
    ALG_LIB_S       stAlgLib;              /* 0x04 */
    ISP_AWB_INIT_FN pfnAwbInit;            /* 0x1C */
    ISP_AWB_RUN_FN  pfnAwbRun;             /* 0x20 */
    ISP_AWB_CTRL_FN pfnAwbCtrl;             /* 0x24 */
    ISP_AWB_EXIT_FN pfnAwbExit;             /* 0x28 */
} ISP_AWB_LIB_NODE_S;                       /* sizeof = 0x2C */

typedef struct ISP_AF_LIB_NODE_S
{
    HI_BOOL        bUsed;                  /* 0x00 */
    ALG_LIB_S      stAlgLib;               /* 0x04 */
    ISP_AF_INIT_FN pfnAfInit;              /* 0x1C */
    ISP_AF_RUN_FN  pfnAfRun;               /* 0x20 */
    ISP_AF_CTRL_FN pfnAfCtrl;              /* 0x24 */
    ISP_AF_EXIT_FN pfnAfExit;              /* 0x28 */
} ISP_AF_LIB_NODE_S;                        /* sizeof = 0x2C */

typedef enum ISP_ALG_TYPE_E
{
    ISP_ALG_AE  = 0,
    ISP_ALG_AF  = 1, /* inferred */
    ISP_ALG_AWB = 2
} ISP_ALG_TYPE_E;

typedef HI_S32 (*ISP_ALG_INIT_FN)(ISP_DEV IspDev);
typedef HI_S32 (*ISP_ALG_RUN_FN)(ISP_DEV, const ISP_STAT_S *, ISP_REG_CFG_S *);
typedef HI_S32 (*ISP_ALG_CTRL_FN)(ISP_DEV, HI_U32, void *);
typedef HI_S32 (*ISP_ALG_EXIT_FN)(ISP_DEV);

typedef struct ISP_ALG_NODE_S
{
    HI_BOOL         bUsed;                  /* 0x00 */
    ISP_ALG_TYPE_E  enAlgType;              /* 0x04 */
    ISP_ALG_INIT_FN pfnAlgInit;             /* 0x08 */
    ISP_ALG_RUN_FN  pfnAlgRun;              /* 0x0C */
    ISP_ALG_CTRL_FN pfnAlgCtrl;             /* 0x10 */
    ISP_ALG_EXIT_FN pfnAlgExit;             /* 0x14 */
} ISP_ALG_NODE_S;                            /* sizeof = 0x18 */

typedef struct ISP_BIND_ATTR_S
{
    SENSOR_ID SensorId;                    /* 0x00 */
    ALG_LIB_S stAeLib;                     /* 0x04 */
    ALG_LIB_S stAfLib;                     /* 0x1C */
    ALG_LIB_S stAwbLib;                    /* 0x34 */
} ISP_BIND_ATTR_S;                          /* sizeof = 0x4C */

typedef struct ISP_DBG_CTRL_S
{
    HI_U32 bEnable;                        /* 0x00, ext 0x12150 bit 0 */
    HI_U32 u32Field04;                     /* 0x04, ext 0x12154/56 */
    HI_U32 u32Field08;                     /* 0x08, ext 0x1215C */
    HI_U32 u32Field0C;                     /* 0x0C, ext 0x12158/5A */
    HI_U32 u32Field10;                     /* 0x10 */
    HI_U32 u32Field14;                     /* 0x14 */
} ISP_DBG_CTRL_S;                           /* sizeof = 0x18 */

typedef struct ISP_MUTEX32_S
{
    HI_U8 au8Data[0x18];
} ISP_MUTEX32_S;                            /* target pthread_mutex_t */

/* ------------------------------------------------------------------------- */
/* Main ISP context                                                         */
/* ------------------------------------------------------------------------- */

typedef struct ISP_CTX_S
{
    HI_U32  u32Field000;                   /* 0x000 */
    HI_BOOL bSensorRegistered;             /* 0x004 */
    HI_U8   au8Field008[0x08];             /* 0x008 */
    HI_BOOL bImageAttrUpdate;              /* 0x010 */
    HI_U8   au8Field014[0x290];            /* 0x014 */
    ISP_MUTEX32_S stImageAttrMutex;        /* 0x2A4 */

    HI_U32 u32Field2BC;                    /* 0x2BC, ext 0x10311 bit 7 */
    HI_U32 enWdrMode;                      /* 0x2C0, applied/current */
    HI_U32 enWdrModeExt;                   /* 0x2C4, requested */
    HI_U32 u32FrameCnt;                    /* 0x2C8 */

    HI_U8  u8FpsBase;                      /* 0x2CC, default 30 */
    HI_U8  u8Field2CD;                     /* 0x2CD */
    HI_U16 u16ImageWidth;                  /* 0x2CE */
    HI_U16 u16ImageHeight;                 /* 0x2D0 */
    HI_U16 u16Field2D2;                    /* 0x2D2 */
    HI_U16 u16ActiveWidth;                 /* 0x2D4 */
    HI_U16 u16ActiveHeight;                /* 0x2D6 */
    HI_U16 u16FpsBaseExt;                  /* 0x2D8, ext 0x11400 */
    HI_U16 u16Field2DA;                    /* 0x2DA */

    ISP_BIND_ATTR_S stBindAttr;            /* 0x2DC */
    HI_S32 s32ActiveAeLib;                 /* 0x328 */
    ISP_AE_LIB_NODE_S astAeLibs[2];        /* 0x32C */
    HI_S32 s32ActiveAwbLib;                /* 0x384 */
    ISP_AWB_LIB_NODE_S astAwbLibs[2];      /* 0x388 */
    HI_S32 s32ActiveAfLib;                 /* 0x3E0 */
    ISP_AF_LIB_NODE_S astAfLibs[2];        /* 0x3E4 */
    ISP_ALG_NODE_S astAlgs[32];            /* 0x43C */

    HI_BOOL bFreezeFmw;                    /* 0x73C, gates AE/AWB during DP calibration */
    HI_U32 u32Field740;                    /* 0x740, unused AwbCfgReg arg */
    HI_U32 u32Field744;                    /* 0x744, unused AwbCfgReg arg */
    HI_U32 u32Iso;                         /* 0x748, AWB cmd 8003 */
    HI_U32 u32AwbParam8005;                /* 0x74C, default 0x100 */
    ISP_DBG_CTRL_S stDbgCtrl;              /* 0x750 */
} ISP_CTX_S;                                /* sizeof = 0x768 */

/* ------------------------------------------------------------------------- */
/* Kernel hi3518_isp.ko context                                              */
/* ------------------------------------------------------------------------- */

#define ISP_DRV_CTX_SIZE           0x738
#define ISP_SENSOR_MAX_REGS        16
#define ISP_SENSOR_REGS_INFO_SIZE  0x1CC

typedef struct ISP_LIST_HEAD32_S
{
    struct ISP_LIST_HEAD32_S *next;
    struct ISP_LIST_HEAD32_S *prev;
} ISP_LIST_HEAD32_S;                        /* sizeof = 0x08 */

typedef struct ISP_WAIT_QUEUE_HEAD32_S
{
    ISP_LIST_HEAD32_S stTaskList;
} ISP_WAIT_QUEUE_HEAD32_S;                  /* sizeof = 0x08 */

typedef struct ISP_SEMAPHORE32_S
{
    HI_S32            s32Count;
    ISP_LIST_HEAD32_S stWaitList;
} ISP_SEMAPHORE32_S;                        /* sizeof = 0x0C */

typedef enum ISP_SNS_TYPE_E
{
    ISP_SNS_I2C_TYPE = 0,
    ISP_SNS_SSP_TYPE = 1
} ISP_SNS_TYPE_E;

typedef struct ISP_I2C_DATA_S
{
    HI_U32 bDelayCfg;                        /* 0x00 */
    HI_U8  u8DevAddr;                        /* 0x04 */
    HI_U8  au8Reserved05[3];                 /* 0x05 */
    HI_U32 u32RegAddr;                       /* 0x08 */
    HI_U32 u32AddrByteNum;                   /* 0x0C */
    HI_U32 u32Data;                          /* 0x10 */
    HI_U32 u32DataByteNum;                   /* 0x14 */
} ISP_I2C_DATA_S;                            /* sizeof = 0x18 */

typedef struct ISP_SSP_DATA_S
{
    HI_U32 bDelayCfg;                        /* 0x00 */
    HI_U32 u32DevAddr;                       /* 0x04 */
    HI_U32 u32DevAddrByteNum;                /* 0x08 */
    HI_U32 u32RegAddr;                       /* 0x0C */
    HI_U32 u32RegAddrByteNum;                /* 0x10 */
    HI_U32 u32Data;                          /* 0x14 */
    HI_U32 u32DataByteNum;                   /* 0x18 */
} ISP_SSP_DATA_S;                            /* sizeof = 0x1C */

typedef union ISP_SENSOR_REG_DATA_U
{
    ISP_I2C_DATA_S astI2cData[ISP_SENSOR_MAX_REGS];
    ISP_SSP_DATA_S astSspData[ISP_SENSOR_MAX_REGS];
} ISP_SENSOR_REG_DATA_U;                     /* sizeof = 0x1C0 */

typedef struct ISP_SENSOR_REGS_INFO_S
{
    ISP_SNS_TYPE_E       enSnsType;          /* 0x000 */
    HI_U32               u32RegNum;          /* 0x004 */
    HI_U32               u32Cfg2ValidDelayMax; /* 0x008 */
    ISP_SENSOR_REG_DATA_U unRegData;         /* 0x00C */
} ISP_SENSOR_REGS_INFO_S;                    /* sizeof = 0x1CC */

typedef void (*ISP_I2C_WRITE_FN)(
    HI_U8 u8DevAddr,
    HI_U32 u32RegAddr,
    HI_U32 u32AddrByteNum,
    HI_U32 u32Data,
    HI_U32 u32DataByteNum);

typedef void (*ISP_SSP_WRITE_FN)(
    HI_U32 u32DevAddr,
    HI_U32 u32DevAddrByteNum,
    HI_U32 u32RegAddr,
    HI_U32 u32RegAddrByteNum,
    HI_U32 u32Data,
    HI_U32 u32DataByteNum);

typedef struct ISP_BUS_CALLBACK_S
{
    ISP_I2C_WRITE_FN pfnI2cWrite;            /* 0x00 */
    ISP_SSP_WRITE_FN pfnSspWrite;            /* 0x04 */
} ISP_BUS_CALLBACK_S;                        /* sizeof = 0x08 */

typedef struct ISP_DGAIN_STATE_S
{
    ISP_DGAIN_PARAMS_S stCurrent;            /* 0x00 */
    ISP_DGAIN_PARAMS_S stSaved;              /* 0x0C */
} ISP_DGAIN_STATE_S;                         /* sizeof = 0x18 */

typedef struct ISP_PROC_MEM_S
{
    HI_U32 u32Size;                          /* 0x00 */
    HI_U32 u32PhyAddr;                       /* 0x04 */
    char  *pszProcBuffer;                    /* 0x08 */
} ISP_PROC_MEM_S;                            /* sizeof = 0x0C */

typedef struct ISP_STAT_NODE32_S
{
    ISP_STAT_INFO_S   stInfo;                /* 0x00 */
    ISP_LIST_HEAD32_S stList;                /* 0x10 */
} ISP_STAT_NODE32_S;                         /* sizeof = 0x18 */

typedef struct ISP_STAT_BUF_MGR32_S
{
    HI_U32            u32MmzPhyBase;         /* 0x00 */
    ISP_STAT_MMZ_S   *pstMmzVirtBase;        /* 0x04 */
    ISP_STAT_NODE32_S astNode[2];            /* 0x08 */
    HI_U32            u32UserCount;          /* 0x38 */
    HI_U32            u32ReadyCount;         /* 0x3C */
    HI_U32            u32FreeCount;          /* 0x40 */
    ISP_LIST_HEAD32_S stUserList;            /* 0x44 */
    ISP_LIST_HEAD32_S stReadyList;           /* 0x4C */
    ISP_LIST_HEAD32_S stFreeList;            /* 0x54 */
} ISP_STAT_BUF_MGR32_S;                      /* sizeof = 0x5C */

typedef struct ISP_DRV_CTX_S
{
    HI_U32 u32IntStatus;                     /* 0x000, word 0 */
    HI_U32 bFrameEdgeReady;                  /* 0x004, word 1 */
    HI_U32 bIrqReady;                        /* 0x008, word 2 */
    HI_U32 u32FrameCount;                    /* 0x00C, word 3 */

    HI_U32 u32SensorRegCfgIndex;             /* 0x010, word 4 */
    ISP_SENSOR_REGS_INFO_S astSensorRegCfg[2]; /* 0x014 */

    ISP_DGAIN_STATE_S stDgainState;          /* 0x3AC */
    ISP_STAT_BUF_MGR32_S stStatBuf;          /* 0x3C4 */

    HI_U32 u32RegCfgIndex;                   /* 0x420 */
    ISP_DRV_REG_CFG_S astRegCfg[2];          /* 0x424 */

    ISP_BUS_CALLBACK_S stBusCallback;        /* 0x6FC */
    ISP_PROC_MEM_S stProcMem;                /* 0x704 */
    ISP_SEMAPHORE32_S stIspSemaphore;        /* 0x710 */
    ISP_WAIT_QUEUE_HEAD32_S stFrameEdgeWait; /* 0x71C */
    ISP_WAIT_QUEUE_HEAD32_S stIrqWait;       /* 0x724 */
    ISP_SEMAPHORE32_S stIrqWaitSemaphore;    /* 0x72C */
} ISP_DRV_CTX_S;                             /* sizeof = 0x738 */

/* Useful public and internal declarations. */
typedef struct ISP_BLACK_LEVEL_S
{
    HI_U16 au16BlackLevel[4];              /* R, Gr, Gb, B */
} ISP_BLACK_LEVEL_S;

HI_S32 ISP_AeRun(ISP_DEV, const ISP_STAT_S *, ISP_REG_CFG_S *);
HI_S32 ISP_AwbRun(ISP_DEV, const ISP_STAT_S *, ISP_REG_CFG_S *);
HI_S32 ISP_DpInit(ISP_DEV IspDev);
HI_S32 ISP_DpRun(
    ISP_DEV IspDev,
    const ISP_STAT_BUFFER_S *pstStat,
    ISP_REG_CFG_S *pstRegCfg);
HI_S32 ISP_DpCtrl(ISP_DEV IspDev, HI_U32 u32Cmd, void *pValue);
HI_S32 ISP_DpExit(ISP_DEV IspDev);
HI_S32 ISP_RegConfig(ISP_REG_CFG_S *pstRegCfg);
HI_S32 ISP_RegConfigInit(ISP_REG_CFG_S *pstRegCfg);
HI_S32 ISP_RegCfgInit(ISP_DEV, ISP_REG_CFG_S **ppstRegCfg);
HI_S32 ISP_RegCfgSet(ISP_DEV);
HI_S32 ISP_DRV_StatisticsRead(ISP_STAT_INFO_S *pstStatInfo);
HI_S32 ISP_DRV_RegConfigSensor(ISP_DRV_CTX_S *pstCtx);
HI_S32 ISP_DRV_RegConfigIspDgain(ISP_DRV_CTX_S *pstCtx);
void AwbCfgReg(
    const ISP_AWB_RESULT_S *pstAwbResult,
    HI_BOOL bWdrMode,
    HI_U32 u32Reserved0,
    HI_U32 u32Reserved1,
    ISP_REG_CFG_S *pstRegCfg);

/*
 * IDA globals for this binary:
 *   ISP_CTX_S      g_astIspCtx[2];      // userspace, [0] at 0x20350
 *   ISP_STAT_CTX_S g_astStatCtx[2];     // userspace mmap state
 *   ISP_DP_CTX_S   g_astDpCtx[2];       // userspace DP module state
 *   ISP_DRV_CTX_S  g_astIspDrvCtx[...]; // kernel driver
 */

#endif /* JUNK_IDA_ISP_CTX_H */
