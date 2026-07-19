void AwbCfgReg(
        const ISP_AWB_RESULT_S *pstAwbResult,
        HI_BOOL bWdrMode,
        HI_U32 u32Reserved0,
        HI_U32 u32Reserved1,
        ISP_REG_CFG_S *pstRegCfg)
{
  unsigned int v5; // r1
  unsigned int v6; // r2
  int v7; // r12
  bool v8; // cc
  unsigned int v9; // r2
  unsigned int v10; // r1
  int v11; // r12
  bool v12; // cc
  unsigned int v13; // r1
  unsigned int v14; // r2
  int v15; // r12
  bool v16; // cc
  unsigned int v17; // r2
  unsigned int v18; // r1
  int v19; // r12
  bool v20; // cc
  unsigned int v21; // r1
  unsigned int v22; // r2
  int v23; // r12
  bool v24; // cc
  int v25; // r2
  unsigned int v26; // r1
  int v27; // r12
  HI_U32 v28; // r2
  HI_U32 v29; // r2
  char bits; // r1
  HI_U32 v31; // r2
  HI_U32 v32; // r2

  if ( pstAwbResult->stStatAttr.bChange )
  {
    if ( bWdrMode )
    {
      v5 = 0;
      v6 = pstAwbResult->stStatAttr.u16MeteringWhiteLevelAwb << 10;
      if ( v6 >= 0x10000000 )
        v5 = 0x4000;
      if ( v6 >= (v5 + 0x2000) * (v5 + 0x2000) )
        v5 = (unsigned __int16)(v5 + 0x2000);
      if ( v6 >= (v5 + 4096) * (v5 + 4096) )
        v5 = (unsigned __int16)(v5 + 4096);
      if ( v6 >= (v5 + 2048) * (v5 + 2048) )
        v5 = (unsigned __int16)(v5 + 2048);
      if ( v6 >= (v5 + 1024) * (v5 + 1024) )
        v5 = (unsigned __int16)(v5 + 1024);
      if ( v6 >= (v5 + 512) * (v5 + 512) )
        v5 = (unsigned __int16)(v5 + 512);
      if ( v6 >= (v5 + 256) * (v5 + 256) )
        v5 = (unsigned __int16)(v5 + 256);
      if ( v6 >= (v5 + 128) * (v5 + 128) )
        v5 = (unsigned __int16)(v5 + 128);
      if ( v6 >= (v5 + 64) * (v5 + 64) )
        v5 = (unsigned __int16)(v5 + 64);
      if ( v6 >= (v5 + 32) * (v5 + 32) )
        v5 = (unsigned __int16)(v5 + 32);
      if ( v6 >= (v5 + 16) * (v5 + 16) )
        v5 = (unsigned __int16)(v5 + 16);
      if ( v6 >= (v5 + 8) * (v5 + 8) )
        v5 = (unsigned __int16)(v5 + 8);
      if ( v6 >= (v5 + 4) * (v5 + 4) )
        v5 = (unsigned __int16)(v5 + 4);
      if ( v6 >= (v5 + 2) * (v5 + 2) )
        v5 = (unsigned __int16)(v5 + 2);
      v7 = v5 + 1;
      if ( v6 >= v7 * v7 )
      {
        v5 = (unsigned __int16)(v5 + 1);
        v7 = (unsigned __int16)v7 + 1;
      }
      v8 = v6 > v5 * v7;
      if ( v6 > v5 * v7 )
        v5 = v7 << 16;
      v9 = 0;
      if ( v8 )
        v5 >>= 16;
      pstRegCfg->stAwbMetering.u16WhiteLevel = v5;
      v10 = pstAwbResult->stStatAttr.u16MeteringBlackLevelAwb << 10;
      if ( v10 >= 0x10000000 )
        v9 = 0x4000;
      if ( v10 >= (v9 + 0x2000) * (v9 + 0x2000) )
        v9 = (unsigned __int16)(v9 + 0x2000);
      if ( v10 >= (v9 + 4096) * (v9 + 4096) )
        v9 = (unsigned __int16)(v9 + 4096);
      if ( v10 >= (v9 + 2048) * (v9 + 2048) )
        v9 = (unsigned __int16)(v9 + 2048);
      if ( v10 >= (v9 + 1024) * (v9 + 1024) )
        v9 = (unsigned __int16)(v9 + 1024);
      if ( v10 >= (v9 + 512) * (v9 + 512) )
        v9 = (unsigned __int16)(v9 + 512);
      if ( v10 >= (v9 + 256) * (v9 + 256) )
        v9 = (unsigned __int16)(v9 + 256);
      if ( v10 >= (v9 + 128) * (v9 + 128) )
        v9 = (unsigned __int16)(v9 + 128);
      if ( v10 >= (v9 + 64) * (v9 + 64) )
        v9 = (unsigned __int16)(v9 + 64);
      if ( v10 >= (v9 + 32) * (v9 + 32) )
        v9 = (unsigned __int16)(v9 + 32);
      if ( v10 >= (v9 + 16) * (v9 + 16) )
        v9 = (unsigned __int16)(v9 + 16);
      if ( v10 >= (v9 + 8) * (v9 + 8) )
        v9 = (unsigned __int16)(v9 + 8);
      if ( v10 >= (v9 + 4) * (v9 + 4) )
        v9 = (unsigned __int16)(v9 + 4);
      if ( v10 >= (v9 + 2) * (v9 + 2) )
        v9 = (unsigned __int16)(v9 + 2);
      v11 = v9 + 1;
      if ( v10 >= v11 * v11 )
      {
        v9 = (unsigned __int16)(v9 + 1);
        v11 = (unsigned __int16)v11 + 1;
      }
      v12 = v10 > v9 * v11;
      if ( v10 > v9 * v11 )
        v9 = v11 << 16;
      v13 = 0;
      if ( v12 )
        v9 >>= 16;
      pstRegCfg->stAwbMetering.u16BlackLevel = v9;
      v14 = pstAwbResult->stStatAttr.u16MeteringCrRefMaxAwb << 8;
      if ( v14 >= 0x10000000 )
        v13 = 0x4000;
      if ( v14 >= (v13 + 0x2000) * (v13 + 0x2000) )
        v13 = (unsigned __int16)(v13 + 0x2000);
      if ( v14 >= (v13 + 4096) * (v13 + 4096) )
        v13 = (unsigned __int16)(v13 + 4096);
      if ( v14 >= (v13 + 2048) * (v13 + 2048) )
        v13 = (unsigned __int16)(v13 + 2048);
      if ( v14 >= (v13 + 1024) * (v13 + 1024) )
        v13 = (unsigned __int16)(v13 + 1024);
      if ( v14 >= (v13 + 512) * (v13 + 512) )
        v13 = (unsigned __int16)(v13 + 512);
      if ( v14 >= (v13 + 256) * (v13 + 256) )
        v13 = (unsigned __int16)(v13 + 256);
      if ( v14 >= (v13 + 128) * (v13 + 128) )
        v13 = (unsigned __int16)(v13 + 128);
      if ( v14 >= (v13 + 64) * (v13 + 64) )
        v13 = (unsigned __int16)(v13 + 64);
      if ( v14 >= (v13 + 32) * (v13 + 32) )
        v13 = (unsigned __int16)(v13 + 32);
      if ( v14 >= (v13 + 16) * (v13 + 16) )
        v13 = (unsigned __int16)(v13 + 16);
      if ( v14 >= (v13 + 8) * (v13 + 8) )
        v13 = (unsigned __int16)(v13 + 8);
      if ( v14 >= (v13 + 4) * (v13 + 4) )
        v13 = (unsigned __int16)(v13 + 4);
      if ( v14 >= (v13 + 2) * (v13 + 2) )
        v13 = (unsigned __int16)(v13 + 2);
      v15 = v13 + 1;
      if ( v14 >= v15 * v15 )
      {
        v13 = (unsigned __int16)(v13 + 1);
        v15 = (unsigned __int16)v15 + 1;
      }
      v16 = v14 > v13 * v15;
      if ( v14 > v13 * v15 )
        v13 = v15 << 16;
      v17 = 0;
      if ( v16 )
        v13 >>= 16;
      pstRegCfg->stAwbMetering.u16CrRefMax = v13;
      v18 = pstAwbResult->stStatAttr.u16MeteringCbRefMaxAwb << 8;
      if ( v18 >= 0x10000000 )
        v17 = 0x4000;
      if ( v18 >= (v17 + 0x2000) * (v17 + 0x2000) )
        v17 = (unsigned __int16)(v17 + 0x2000);
      if ( v18 >= (v17 + 4096) * (v17 + 4096) )
        v17 = (unsigned __int16)(v17 + 4096);
      if ( v18 >= (v17 + 2048) * (v17 + 2048) )
        v17 = (unsigned __int16)(v17 + 2048);
      if ( v18 >= (v17 + 1024) * (v17 + 1024) )
        v17 = (unsigned __int16)(v17 + 1024);
      if ( v18 >= (v17 + 512) * (v17 + 512) )
        v17 = (unsigned __int16)(v17 + 512);
      if ( v18 >= (v17 + 256) * (v17 + 256) )
        v17 = (unsigned __int16)(v17 + 256);
      if ( v18 >= (v17 + 128) * (v17 + 128) )
        v17 = (unsigned __int16)(v17 + 128);
      if ( v18 >= (v17 + 64) * (v17 + 64) )
        v17 = (unsigned __int16)(v17 + 64);
      if ( v18 >= (v17 + 32) * (v17 + 32) )
        v17 = (unsigned __int16)(v17 + 32);
      if ( v18 >= (v17 + 16) * (v17 + 16) )
        v17 = (unsigned __int16)(v17 + 16);
      if ( v18 >= (v17 + 8) * (v17 + 8) )
        v17 = (unsigned __int16)(v17 + 8);
      if ( v18 >= (v17 + 4) * (v17 + 4) )
        v17 = (unsigned __int16)(v17 + 4);
      if ( v18 >= (v17 + 2) * (v17 + 2) )
        v17 = (unsigned __int16)(v17 + 2);
      v19 = v17 + 1;
      if ( v18 >= v19 * v19 )
      {
        v17 = (unsigned __int16)(v17 + 1);
        v19 = (unsigned __int16)v19 + 1;
      }
      v20 = v18 > v17 * v19;
      if ( v18 > v17 * v19 )
        v17 = v19 << 16;
      v21 = 0;
      if ( v20 )
        v17 >>= 16;
      pstRegCfg->stAwbMetering.u16CbRefMax = v17;
      v22 = pstAwbResult->stStatAttr.u16MeteringCrRefMinAwb << 8;
      if ( v22 >= 0x10000000 )
        v21 = 0x4000;
      if ( v22 >= (v21 + 0x2000) * (v21 + 0x2000) )
        v21 = (unsigned __int16)(v21 + 0x2000);
      if ( v22 >= (v21 + 4096) * (v21 + 4096) )
        v21 = (unsigned __int16)(v21 + 4096);
      if ( v22 >= (v21 + 2048) * (v21 + 2048) )
        v21 = (unsigned __int16)(v21 + 2048);
      if ( v22 >= (v21 + 1024) * (v21 + 1024) )
        v21 = (unsigned __int16)(v21 + 1024);
      if ( v22 >= (v21 + 512) * (v21 + 512) )
        v21 = (unsigned __int16)(v21 + 512);
      if ( v22 >= (v21 + 256) * (v21 + 256) )
        v21 = (unsigned __int16)(v21 + 256);
      if ( v22 >= (v21 + 128) * (v21 + 128) )
        v21 = (unsigned __int16)(v21 + 128);
      if ( v22 >= (v21 + 64) * (v21 + 64) )
        v21 = (unsigned __int16)(v21 + 64);
      if ( v22 >= (v21 + 32) * (v21 + 32) )
        v21 = (unsigned __int16)(v21 + 32);
      if ( v22 >= (v21 + 16) * (v21 + 16) )
        v21 = (unsigned __int16)(v21 + 16);
      if ( v22 >= (v21 + 8) * (v21 + 8) )
        v21 = (unsigned __int16)(v21 + 8);
      if ( v22 >= (v21 + 4) * (v21 + 4) )
        v21 = (unsigned __int16)(v21 + 4);
      if ( v22 >= (v21 + 2) * (v21 + 2) )
        v21 = (unsigned __int16)(v21 + 2);
      v23 = v21 + 1;
      if ( v22 >= v23 * v23 )
      {
        v21 = (unsigned __int16)(v21 + 1);
        v23 = (unsigned __int16)v23 + 1;
      }
      v24 = v22 > v21 * v23;
      if ( v22 > v21 * v23 )
        v21 = v23 << 16;
      v25 = 0;
      if ( v24 )
        v21 >>= 16;
      pstRegCfg->stAwbMetering.u16CrRefMin = v21;
      v26 = pstAwbResult->stStatAttr.u16MeteringCbRefMinAwb << 8;
      if ( v26 >= 0x10000000 )
        v25 = 0x4000;
      if ( v26 >= (v25 + 0x2000) * (v25 + 0x2000) )
        v25 = (unsigned __int16)(v25 + 0x2000);
      if ( v26 >= (v25 + 4096) * (v25 + 4096) )
        v25 = (unsigned __int16)(v25 + 4096);
      if ( v26 >= (v25 + 2048) * (v25 + 2048) )
        v25 = (unsigned __int16)(v25 + 2048);
      if ( v26 >= (v25 + 1024) * (v25 + 1024) )
        v25 = (unsigned __int16)(v25 + 1024);
      if ( v26 >= (v25 + 512) * (v25 + 512) )
        v25 = (unsigned __int16)(v25 + 512);
      if ( v26 >= (v25 + 256) * (v25 + 256) )
        v25 = (unsigned __int16)(v25 + 256);
      if ( v26 >= (v25 + 128) * (v25 + 128) )
        v25 = (unsigned __int16)(v25 + 128);
      if ( v26 >= (v25 + 64) * (v25 + 64) )
        v25 = (unsigned __int16)(v25 + 64);
      if ( v26 >= (v25 + 32) * (v25 + 32) )
        v25 = (unsigned __int16)(v25 + 32);
      if ( v26 >= (v25 + 16) * (v25 + 16) )
        v25 = (unsigned __int16)(v25 + 16);
      if ( v26 >= (v25 + 8) * (v25 + 8) )
        v25 = (unsigned __int16)(v25 + 8);
      if ( v26 >= (v25 + 4) * (v25 + 4) )
        v25 = (unsigned __int16)(v25 + 4);
      if ( v26 >= (v25 + 2) * (v25 + 2) )
        v25 = (unsigned __int16)(v25 + 2);
      v27 = v25 + 1;
      if ( v26 >= v27 * v27 )
      {
        v25 = (unsigned __int16)(v25 + 1);
        v27 = (unsigned __int16)v27 + 1;
      }
      if ( v26 > v25 * v27 )
        LOWORD(v25) = v27;
      pstRegCfg->stAwbMetering.u16CbRefMin = v25;
    }
    else
    {
      pstRegCfg->stAwbMetering = *(ISP_AWB_METER_CFG_S *)&pstAwbResult->stStatAttr.u16MeteringWhiteLevelAwb;
    }
    *(_BYTE *)&pstRegCfg->unKey.bits |= 8u;
  }
  pstRegCfg->stCcm.au16Coeff[0] = pstAwbResult->au16ColorMatrix[0];
  pstRegCfg->stCcm.au16Coeff[1] = pstAwbResult->au16ColorMatrix[1];
  pstRegCfg->stCcm.au16Coeff[2] = pstAwbResult->au16ColorMatrix[2];
  pstRegCfg->stCcm.au16Coeff[3] = pstAwbResult->au16ColorMatrix[3];
  pstRegCfg->stCcm.au16Coeff[4] = pstAwbResult->au16ColorMatrix[4];
  v28 = pstAwbResult->au32WhiteBalanceGain[0];
  pstRegCfg->stCcm.au16Coeff[5] = pstAwbResult->au16ColorMatrix[5];
  pstRegCfg->stDgainCfg.stWbGain.u32R = v28;
  v29 = pstAwbResult->au32WhiteBalanceGain[1];
  bits = (char)pstRegCfg->unKey.bits;
  pstRegCfg->stCcm.au16Coeff[6] = pstAwbResult->au16ColorMatrix[6];
  pstRegCfg->stDgainCfg.stWbGain.u32Gr = v29;
  v31 = pstAwbResult->au32WhiteBalanceGain[2];
  pstRegCfg->stCcm.au16Coeff[7] = pstAwbResult->au16ColorMatrix[7];
  pstRegCfg->stDgainCfg.stWbGain.u32Gb = v31;
  v32 = pstAwbResult->au32WhiteBalanceGain[3];
  pstRegCfg->stCcm.au16Coeff[8] = pstAwbResult->au16ColorMatrix[8];
  pstRegCfg->stDgainCfg.stWbGain.u32B = v32;
  *(_BYTE *)&pstRegCfg->unKey.bits = bits | 4;
}

HI_S32 ISP_AwbRun(ISP_DEV a1, const ISP_STAT_S *a2, ISP_REG_CFG_S *a3)
{
  ISP_CTX_S *v4; // r4
  HI_S32 s32ActiveAwbLib; // r2
  HI_S32 v6; // r6
  HI_BOOL bUsed; // lr
  ISP_AWB_LIB_NODE_S *astAwbLibs; // r8
  ISP_AWB_CTRL_FN pfnAwbCtrl; // r3
  ISP_AWB_CTRL_FN v10; // r3
  HI_S32 (*pfnAwbRun)(HI_S32, const ISP_AWB_INFO_S *, ISP_AWB_RESULT_S *, HI_S32); // r12
  HI_S32 v12; // r0
  ISP_AWB_RESULT_S v14; // [sp+8h] [bp-60h] BYREF
  const ISP_AWB_INFO_S v15; // [sp+3Ch] [bp-2Ch] BYREF

  v4 = &g_astIspCtx[a1];
  s32ActiveAwbLib = v4->s32ActiveAwbLib;
  if ( v4->bFreezeFmw )
    return 0;
  memset(&v14, 0, sizeof(v14));
  bUsed = v4->astAwbLibs[0].bUsed;
  v15.u32FrameCnt = v4->u32FrameCnt;
  v15.pstAwbStat1 = &a2->stAwbGlobal;
  v15.pstAwbStat2 = &a2->stAwbZonal;
  astAwbLibs = &v4->astAwbLibs[s32ActiveAwbLib];
  if ( bUsed )
  {
    pfnAwbCtrl = v4->astAwbLibs[0].pfnAwbCtrl;
    astAwbLibs = v4->astAwbLibs;
    if ( pfnAwbCtrl )
    {
      pfnAwbCtrl(v4->astAwbLibs[0].stAlgLib.s32Id, 8003, &v4->u32Iso);
      v4->astAwbLibs[0].pfnAwbCtrl(v4->astAwbLibs[0].stAlgLib.s32Id, 8005, &v4->u32AwbParam8005);
    }
  }
  if ( v4->astAwbLibs[1].bUsed )
  {
    v10 = v4->astAwbLibs[1].pfnAwbCtrl;
    astAwbLibs = &v4->astAwbLibs[1];
    if ( v10 )
    {
      v10(v4->astAwbLibs[1].stAlgLib.s32Id, 8003, &v4->u32Iso);
      v4->astAwbLibs[1].pfnAwbCtrl(v4->astAwbLibs[1].stAlgLib.s32Id, 8005, &v4->u32AwbParam8005);
    }
  }
  pfnAwbRun = astAwbLibs->pfnAwbRun;
  if ( pfnAwbRun )
  {
    v12 = pfnAwbRun(astAwbLibs->stAlgLib.s32Id, &v15, &v14, 0);
    v6 = v12;
    if ( v12 )
      printf("WARNING!! run awb lib err 0x%x!\n", v12);
  }
  else
  {
    v6 = -1;
  }
  j_AwbCfgReg(&v14, v4->enWdrModeExt, v4->u32Field740, v4->u32Field744, a3);
  return v6;
}

int __fastcall ISP_AwbInit(int a1)
{
  ISP_CTX_S *v2; // r4
  HI_BOOL bUsed; // r2
  void (__fastcall *pfnAwbInit)(_DWORD, _DWORD); // r3
  void (__fastcall *v5)(_DWORD, _DWORD); // r3
  _DWORD v7[2]; // [sp+4h] [bp-1Ch] BYREF
  int v8; // [sp+Ch] [bp-14h] BYREF

  v2 = &g_astIspCtx[a1];
  j_IO_WRITE16(0x205A0640, 940);
  j_IO_WRITE16(0x205A0644, 64);
  j_IO_WRITE16(0x205A0648, 512);
  j_IO_WRITE16(0x205A064C, 128);
  j_IO_WRITE16(0x205A0650, 512);
  j_IO_WRITE16(0x205A0654, 128);
  j_IO_WRITE16(0x205A04A8, 256);
  j_IO_WRITE16(0x205A04AC, 256);
  j_IO_WRITE16(0x205A04B0, 256);
  bUsed = v2->astAwbLibs[0].bUsed;
  v7[0] = v2->stBindAttr.SensorId;
  if ( bUsed )
  {
    pfnAwbInit = (void (__fastcall *)(_DWORD, _DWORD))v2->astAwbLibs[0].pfnAwbInit;
    if ( pfnAwbInit )
      pfnAwbInit(v2->astAwbLibs[0].stAlgLib.s32Id, v7);
  }
  if ( v2->astAwbLibs[1].bUsed )
  {
    v5 = (void (__fastcall *)(_DWORD, _DWORD))v2->astAwbLibs[1].pfnAwbInit;
    if ( v5 )
      v5(v2->astAwbLibs[1].stAlgLib.s32Id, v7);
  }
  if ( (j_IO_READ16(0x12114) & 4) != 0 )
  {
    v8 = 1;
    j_ISP_AwbCtrl(a1, 0x1F40u, &v8);
  }
  return 0;
}

int __fastcall ISP_AwbExit(int a1)
{
  ISP_CTX_S *v1; // r4
  void (__fastcall *pfnAwbExit)(HI_S32); // r3
  void (__fastcall *v3)(HI_S32); // r3

  v1 = &g_astIspCtx[a1];
  if ( v1->astAwbLibs[0].bUsed )
  {
    pfnAwbExit = (void (__fastcall *)(HI_S32))v1->astAwbLibs[0].pfnAwbExit;
    if ( pfnAwbExit )
      pfnAwbExit(v1->astAwbLibs[0].stAlgLib.s32Id);
  }
  if ( v1->astAwbLibs[1].bUsed )
  {
    v3 = (void (__fastcall *)(HI_S32))v1->astAwbLibs[1].pfnAwbExit;
    if ( v3 )
      v3(v1->astAwbLibs[1].stAlgLib.s32Id);
  }
  return 0;
}

int __fastcall ISP_AwbCtrl(int a1, HI_U32 a2, void *a3)
{
  ISP_CTX_S *v4; // r4
  ISP_AWB_CTRL_FN pfnAwbCtrl; // r3
  int result; // r0
  HI_S32 (*v8)(HI_S32, HI_U32, void *); // r3

  v4 = &g_astIspCtx[a1];
  if ( v4->astAwbLibs[0].bUsed && (pfnAwbCtrl = v4->astAwbLibs[0].pfnAwbCtrl) != 0 )
  {
    result = pfnAwbCtrl(v4->astAwbLibs[0].stAlgLib.s32Id, a2, a3);
    if ( !v4->astAwbLibs[1].bUsed )
      return result;
  }
  else
  {
    result = -1;
    if ( !v4->astAwbLibs[1].bUsed )
      return result;
  }
  v8 = v4->astAwbLibs[1].pfnAwbCtrl;
  if ( v8 )
    return v8(v4->astAwbLibs[1].stAlgLib.s32Id, a2, a3);
  return result;
}
