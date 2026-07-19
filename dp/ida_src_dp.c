HI_S32 ISP_DpCtrl(ISP_DEV IspDev, HI_U32 u32Cmd, void *pValue)
{
  return 0;
}

HI_S32 ISP_DpExit(ISP_DEV IspDev)
{
  return 0;
}

HI_S32 ISP_DpInit(ISP_DEV IspDev)
{
  ISP_DEV v1; // r4
  char v2; // r0
  ISP_DP_CTX_S *v3; // r4
  char v4; // r0
  __int16 v5; // r0
  __int16 v6; // r0
  char v7; // r0
  HI_S32 result; // r0

  v1 = IspDev;
  v2 = j_IO_READ8(0x205A01E8);
  v3 = &g_astDpCtx[v1];
  j_IO_WRITE8(0x205A01E8, v2 & 0xFB | 4);
  v4 = j_IO_READ8(0x10317);
  j_IO_WRITE8(0x10317, v4 & 0xFE);
  j_IO_WRITE16(0x12110, 256);
  j_IO_WRITE16(0x12112, 64);
  v5 = j_IO_READ16(0x12106);
  j_IO_WRITE16(0x12106, v5 & 0xFF00 | 0x1F);
  v6 = j_IO_READ16(0x12106);
  j_IO_WRITE16(0x12106, v6 & 0xFCFF);
  j_IO_WRITE16(0x12108, 1600);
  j_IO_WRITE16(0x12160, 512);
  j_IO_WRITE16(0x12162, 64);
  v7 = j_IO_READ8(0x205A01C0);
  j_IO_WRITE8(0x205A01C0, v7 & 0xFB | 4);
  result = 0;
  v3->u32StructSize = 28;
  v3->u8TrialCount = 0;
  v3->u8CalibrationState = 0;
  v3->u16DetectedPixelCount = 0;
  return result;
}

HI_S32 ISP_DpRun(ISP_DEV IspDev, const ISP_STAT_BUFFER_S *pstStat, ISP_REG_CFG_S *pstRegCfg)
{
  ISP_CTX_S *v4; // r9
  ISP_DP_CTX_S *v6; // r4
  char v7; // r0
  HI_BOOL bCalibrationTrigger; // r11
  HI_U8 v9; // r0
  HI_U16 u16DpSlope; // r3
  HI_U8 bDefectPixelEnable; // r4
  char v12; // r0
  HI_U8 bShowStaticDefectPixels; // r4
  char v14; // r0
  HI_S32 result; // r0
  char v16; // r3
  unsigned int u8CalibrationState; // r9
  int v18; // r9
  char v19; // r0
  HI_U8 u8CalibrationDpThreshold; // r3
  unsigned int u16DefectPixelCount; // r11
  char v22; // r12
  HI_U8 u8TrialCount; // r3
  HI_U8 v24; // r0
  char v25; // r3
  char v26; // r0
  unsigned int u16DetectedPixelCount; // r9
  HI_U8 v28; // r12
  HI_U8 u8MaxTrialCount; // r1
  char v30; // r3

  v4 = &g_astIspCtx[IspDev];
  v6 = &g_astDpCtx[IspDev];
  v6->u8MaxTrialCount = (unsigned int)j_IO_READ16(0x12108) >> 3;
  v6->bCalibrationTrigger = j_IO_READ8(0x10317) & 1;
  v6->u16DpSlope = j_IO_READ16(0x12160);
  v6->u16DpThreshold = j_IO_READ16(0x12162);
  v6->bDefectPixelEnable = (j_IO_READ8(0x205A01C0) & 4) != 0;
  v7 = j_IO_READ8(0x205A01E8);
  bCalibrationTrigger = v6->bCalibrationTrigger;
  v9 = (v7 & 8) != 0;
  v6->bShowStaticDefectPixels = v9;
  if ( bCalibrationTrigger )
  {
    v4->bFreezeFmw = 1;
    pstRegCfg->unKey.u32Key = 0;
    if ( v6->u32StructSize != 28 )
      return -1;
    u8CalibrationState = v6->u8CalibrationState;
    v6->s8CalibrationResult = 0;
    if ( u8CalibrationState > 7 )
      return 0;
    if ( u8CalibrationState == 1 )
    {
      j_ISP_SensorSetPixelDetect(IspDev, 1);
      v24 = j_IO_READ16(0x12106);
      v6->bCalibrationActive = 1;
      g_astDpCtx[IspDev].u8CalibrationDpThreshold = v24;
      v25 = *((_BYTE *)&pstRegCfg->unKey.bits + 1);
      pstRegCfg->stDefectPixel.u16DpSlope = 512;
      *((_BYTE *)&pstRegCfg->unKey.bits + 1) = v25 | 4;
      v26 = *((_BYTE *)&pstRegCfg->unKey.bits + 1);
      pstRegCfg->stGreenEqualization.u8Strength = 32;
      *((_BYTE *)&pstRegCfg->unKey.bits + 1) = v26 | 1;
      LOBYTE(u8CalibrationState) = v6->u8CalibrationState;
    }
    v18 = (unsigned __int8)(u8CalibrationState + 1);
    v6->u8CalibrationState = v18;
    switch ( v18 )
    {
      case 3:
        u8CalibrationDpThreshold = g_astDpCtx[IspDev].u8CalibrationDpThreshold;
        *((_BYTE *)&pstRegCfg->unKey.bits + 1) |= 1u;
        pstRegCfg->stDefectPixel.u8CalibrationDpThreshold = u8CalibrationDpThreshold;
        pstRegCfg->stDefectPixel.u16DefectPixelCountIn = 0;
        pstRegCfg->stDefectPixel.bPointerReset = 1;
        pstRegCfg->stDefectPixel.bShowStaticDefectPixels = 1;
        pstRegCfg->stDefectPixel.bCorrectionEnable = 1;
        break;
      case 4:
        *((_BYTE *)&pstRegCfg->unKey.bits + 1) |= 1u;
        pstRegCfg->stDefectPixel.bPointerReset = 0;
        pstRegCfg->stDefectPixel.bDetectionTrigger = 1;
        break;
      case 5:
        *((_BYTE *)&pstRegCfg->unKey.bits + 1) |= 1u;
        pstRegCfg->stDefectPixel.bDetectionTrigger = 0;
        break;
      case 7:
        u16DefectPixelCount = pstStat->u16DefectPixelCount;
        v6->u16DetectedPixelCount = u16DefectPixelCount;
        v22 = *((_BYTE *)&pstRegCfg->unKey.bits + 1);
        pstRegCfg->stDefectPixel.bShowStaticDefectPixels = 0;
        *((_BYTE *)&pstRegCfg->unKey.bits + 1) = v22 | 1;
        if ( v6->u8TrialCount >= (unsigned int)v6->u8MaxTrialCount )
        {
          if ( u16DefectPixelCount <= j_IO_READ16(0x12110) )
          {
            printf("BAD PIXEL CALIBRATION TIME OUT  %x\n", v6->u8MaxTrialCount);
            DpExit(IspDev, &g_astDpCtx[IspDev]);
            *((_BYTE *)&pstRegCfg->unKey.bits + 1) |= 4u;
            pstRegCfg->stGreenEqualization.u8Strength = 0;
            v6->s8CalibrationResult = -1;
            hi_ext_system_bad_pixel_trigger_status_write(2);
            break;
          }
          u16DefectPixelCount = v6->u16DetectedPixelCount;
        }
        if ( j_IO_READ16(0x12110) >= u16DefectPixelCount )
        {
          u16DetectedPixelCount = v6->u16DetectedPixelCount;
          if ( u16DetectedPixelCount >= j_IO_READ16(0x12112) )
          {
            printf("trial: %x, findshed: %x\n", v6->u8TrialCount, v6->u16DetectedPixelCount);
            DpExit(IspDev, &g_astDpCtx[IspDev]);
            v30 = *((_BYTE *)&pstRegCfg->unKey.bits + 1);
            pstRegCfg->stGreenEqualization.u8Strength = 0;
            *((_BYTE *)&pstRegCfg->unKey.bits + 1) = v30 | 4;
            v6->s8CalibrationResult = 1;
            hi_ext_system_bad_pixel_trigger_status_write(1);
          }
          else
          {
            printf(
              "BAD_PIXEL_COUNT_LOWER_LIMIT %x, %x\n",
              g_astDpCtx[IspDev].u8CalibrationDpThreshold,
              v6->u16DetectedPixelCount);
            v28 = g_astDpCtx[IspDev].u8CalibrationDpThreshold;
            v6->u8CalibrationState = 2;
            if ( v28 == 1 )
            {
              u8MaxTrialCount = v6->u8MaxTrialCount;
              v6->u8TrialCount = u8MaxTrialCount;
            }
            else
            {
              u8MaxTrialCount = v6->u8TrialCount;
            }
            g_astDpCtx[IspDev].u8CalibrationDpThreshold = v28 - 1;
            v6->u8TrialCount = u8MaxTrialCount + 1;
          }
        }
        else
        {
          printf(
            "BAD_PIXEL_COUNT_UPPER_LIMIT %x, %x\n",
            g_astDpCtx[IspDev].u8CalibrationDpThreshold,
            v6->u16DetectedPixelCount);
          u8TrialCount = v6->u8TrialCount;
          ++g_astDpCtx[IspDev].u8CalibrationDpThreshold;
          v6->u8CalibrationState = 2;
          v6->u8TrialCount = u8TrialCount + 1;
        }
        break;
    }
    if ( v6->s8CalibrationResult )
    {
      v19 = j_IO_READ8(0x10317);
      j_IO_WRITE8(0x10317, v19 & 0xFE);
      return 0;
    }
    return 0;
  }
  if ( v6->bCalibrationActive )
  {
    DpExit(IspDev, &g_astDpCtx[IspDev]);
    v16 = *((_BYTE *)&pstRegCfg->unKey.bits + 1);
    pstRegCfg->stGreenEqualization.u8Strength = 0;
    *((_BYTE *)&pstRegCfg->unKey.bits + 1) = v16 | 1;
    *((_BYTE *)&pstRegCfg->unKey.bits + 1) |= 4u;
  }
  else
  {
    pstRegCfg->stDefectPixel.bDefectPixelEnable = v6->bDefectPixelEnable;
    u16DpSlope = v6->u16DpSlope;
    pstRegCfg->stDefectPixel.u16DpSlope = u16DpSlope;
    pstRegCfg->stDefectPixel.u16DpThreshold = v6->u16DpThreshold;
    pstRegCfg->stDefectPixel.bShowStaticDefectPixels = v9;
    j_IO_WRITE16(0x205A01D0, u16DpSlope & 0xFFF);
    j_IO_WRITE16(0x205A01CA, pstRegCfg->stDefectPixel.u16DpThreshold & 0xFFF);
    bDefectPixelEnable = pstRegCfg->stDefectPixel.bDefectPixelEnable;
    v12 = j_IO_READ8(0x205A01C0);
    j_IO_WRITE8(0x205A01C0, v12 & 0xFB | (4 * (bDefectPixelEnable & 1)));
    bShowStaticDefectPixels = pstRegCfg->stDefectPixel.bShowStaticDefectPixels;
    v14 = j_IO_READ8(0x205A01E8);
    j_IO_WRITE8(0x205A01E8, v14 & 0xF7 | (8 * (bShowStaticDefectPixels & 1)));
  }
  result = 0;
  v4->bFreezeFmw = 0;
  return result;
}

