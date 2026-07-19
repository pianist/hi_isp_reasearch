/*
 * Minimal implementations of public HI_MPI_ISP_* AWB controls.
 *
 * The original functions store their state in AWB VREGs beginning at
 * 0x30000.  This replacement owns no VREG block, so equivalent state is
 * retained in AWB handle 0, which is the handle addressed by those wrappers.
 */

#include "isp_awb.h"

#include <stdio.h>


HI_S32 HI_MPI_ISP_SetWBType(ISP_OP_TYPE_E enWBType)
{
    AWB_DEBUG_PRINT(
        "Calling HI_MPI_ISP_SetWBType with params: enWBType=%d\n",
        enWBType);

    if ((HI_U32)enWBType >= (HI_U32)OP_TYPE_BUTT) {
        puts("Invalid WB type!");
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    /*
     * Original VREG encoding at 0x30000:
     *   bit 3      = automatic AWB enable (1 for AUTO, 0 for MANUAL)
     *   bits [5:4] = ISP_OP_TYPE_E
     * Keeping the enum is sufficient because this stub never exposes VREGs.
     */
    g_astAwbStubCtx[0].enWbType = enWBType;

    return HI_SUCCESS;
}


HI_S32 HI_MPI_ISP_GetWBType(ISP_OP_TYPE_E *penWBType)
{
    AWB_DEBUG_PRINT(
        "Calling HI_MPI_ISP_GetWBType with params: penWBType=%p\n",
        (void *)penWBType);

    if (penWBType == HI_NULL)
        return HI_ERR_ISP_NULL_PTR;

    *penWBType = g_astAwbStubCtx[0].enWbType;

    AWB_DEBUG_PRINT("  WB type=%d\n", *penWBType);

    return HI_SUCCESS;
}


HI_S32 HI_MPI_ISP_SetAdvAWBAttr(ISP_ADV_AWB_ATTR_S *pstAdvAWBAttr)
{
    ISP_ADV_AWB_ATTR_S *pstStored;

    AWB_DEBUG_PRINT(
        "Calling HI_MPI_ISP_SetAdvAWBAttr with params: pstAdvAWBAttr=%p\n",
        (void *)pstAdvAWBAttr);

    if (pstAdvAWBAttr == HI_NULL)
        return HI_ERR_ISP_NULL_PTR;

    AWB_DEBUG_PRINT(
        "  base: accuPrior=%d, tolerance=%u, curve=[%u..%u], "
        "gainNorm=%d\n",
        pstAdvAWBAttr->bAccuPrior,
        pstAdvAWBAttr->u8Tolerance,
        pstAdvAWBAttr->u16CurveLLimit,
        pstAdvAWBAttr->u16CurveRLimit,
        pstAdvAWBAttr->bGainNormEn);
    AWB_DEBUG_PRINT(
        "  in/out: enable=%d, opType=%d, outdoor=%d, outThresh=%u, "
        "temps={lowStop=%u, lowStart=%u, highStart=%u, highStop=%u}, "
        "greenEnhance=%d\n",
        pstAdvAWBAttr->stInOrOut.bEnable,
        pstAdvAWBAttr->stInOrOut.enOpType,
        pstAdvAWBAttr->stInOrOut.bOutdoorStatus,
        pstAdvAWBAttr->stInOrOut.u32OutThresh,
        pstAdvAWBAttr->stInOrOut.u16LowStop,
        pstAdvAWBAttr->stInOrOut.u16LowStart,
        pstAdvAWBAttr->stInOrOut.u16HighStart,
        pstAdvAWBAttr->stInOrOut.u16HighStop,
        pstAdvAWBAttr->stInOrOut.bGreenEnhanceEn);
    AWB_DEBUG_PRINT(
        "  CT limit: enable=%d, opType=%d, high={Rg=%u,Bg=%u}, "
        "low={Rg=%u,Bg=%u}\n",
        pstAdvAWBAttr->stCTLimit.bEnable,
        pstAdvAWBAttr->stCTLimit.enOpType,
        pstAdvAWBAttr->stCTLimit.u16HighRgLimit,
        pstAdvAWBAttr->stCTLimit.u16HighBgLimit,
        pstAdvAWBAttr->stCTLimit.u16LowRgLimit,
        pstAdvAWBAttr->stCTLimit.u16LowBgLimit);

    if (pstAdvAWBAttr->u16CurveLLimit > 0x0100u ||
        pstAdvAWBAttr->u16CurveRLimit <= 0x00FFu ||
        (HI_U32)pstAdvAWBAttr->bAccuPrior > 1u) {
        puts("Invalid Parameter Input!");
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    if ((HI_U32)pstAdvAWBAttr->bGainNormEn > 1u) {
        puts("Invalid input of parameter bGainNorm!");
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    if ((HI_U32)pstAdvAWBAttr->stInOrOut.bEnable > 1u) {
        puts("Invalid input of parameter InOrOut enable!");
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    if (pstAdvAWBAttr->stInOrOut.bEnable == HI_TRUE) {
        if ((HI_U32)pstAdvAWBAttr->stInOrOut.bGreenEnhanceEn > 1u) {
            puts("Invalid input of parameter bGreenEnhanceEn enable!");
            return HI_ERR_ISP_ILLEGAL_PARAM;
        }

        if ((HI_U32)pstAdvAWBAttr->stInOrOut.enOpType >=
            (HI_U32)OP_TYPE_BUTT) {
            puts("Invalid input of parameter InOrOut enOpType!");
            return HI_ERR_ISP_ILLEGAL_PARAM;
        }

        if (pstAdvAWBAttr->stInOrOut.u16LowStop >=
                pstAdvAWBAttr->stInOrOut.u16LowStart ||
            pstAdvAWBAttr->stInOrOut.u16LowStart >=
                pstAdvAWBAttr->stInOrOut.u16HighStart ||
            pstAdvAWBAttr->stInOrOut.u16HighStop <=
                pstAdvAWBAttr->stInOrOut.u16HighStart) {
            puts("Invalid input of parameter InOrOut Temp Range!");
            return HI_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    if ((HI_U32)pstAdvAWBAttr->stCTLimit.bEnable > 1u) {
        puts("Invalid input of parameter CT enable!");
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    if ((HI_U32)pstAdvAWBAttr->stCTLimit.enOpType >=
        (HI_U32)OP_TYPE_BUTT) {
        puts("Invalid input of parameter CT enOpType!");
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    if (pstAdvAWBAttr->stCTLimit.enOpType == OP_TYPE_MANUAL) {
        if (pstAdvAWBAttr->stCTLimit.u16HighRgLimit <=
            pstAdvAWBAttr->stCTLimit.u16LowRgLimit) {
            puts("Invalid input of parameter CT RgLimit!");
            return HI_ERR_ISP_ILLEGAL_PARAM;
        }

        if (pstAdvAWBAttr->stCTLimit.u16HighBgLimit >=
            pstAdvAWBAttr->stCTLimit.u16LowBgLimit) {
            puts("Invalid input of parameter CT BgLimit!");
            return HI_ERR_ISP_ILLEGAL_PARAM;
        }
    }

    pstStored = &g_astAwbStubCtx[0].stAdvAwbAttr;

    pstStored->bAccuPrior     = pstAdvAWBAttr->bAccuPrior;
    pstStored->u8Tolerance    = pstAdvAWBAttr->u8Tolerance;
    pstStored->u16CurveLLimit = pstAdvAWBAttr->u16CurveLLimit;
    pstStored->u16CurveRLimit = pstAdvAWBAttr->u16CurveRLimit;
    pstStored->bGainNormEn    = pstAdvAWBAttr->bGainNormEn;

    pstStored->stInOrOut.bEnable =
        pstAdvAWBAttr->stInOrOut.bEnable;
    pstStored->stInOrOut.enOpType =
        (ISP_OP_TYPE_E)((HI_U32)pstAdvAWBAttr->stInOrOut.enOpType & 1u);
    pstStored->stInOrOut.bOutdoorStatus =
        (HI_BOOL)((HI_U32)pstAdvAWBAttr->stInOrOut.bOutdoorStatus & 1u);
    pstStored->stInOrOut.u32OutThresh =
        pstAdvAWBAttr->stInOrOut.u32OutThresh;
    pstStored->stInOrOut.bGreenEnhanceEn =
        (HI_BOOL)((HI_U32)pstAdvAWBAttr->stInOrOut.bGreenEnhanceEn & 1u);
    pstStored->stInOrOut.u16LowStart =
        pstAdvAWBAttr->stInOrOut.u16LowStart;
    pstStored->stInOrOut.u16LowStop =
        pstAdvAWBAttr->stInOrOut.u16LowStop;
    pstStored->stInOrOut.u16HighStart =
        pstAdvAWBAttr->stInOrOut.u16HighStart;
    pstStored->stInOrOut.u16HighStop =
        pstAdvAWBAttr->stInOrOut.u16HighStop;

    pstStored->stCTLimit.bEnable =
        pstAdvAWBAttr->stCTLimit.bEnable;
    pstStored->stCTLimit.enOpType =
        pstAdvAWBAttr->stCTLimit.enOpType;

    /* The original writes these four VREGs only in manual CT-limit mode. */
    if (pstAdvAWBAttr->stCTLimit.enOpType == OP_TYPE_MANUAL) {
        pstStored->stCTLimit.u16HighRgLimit =
            pstAdvAWBAttr->stCTLimit.u16HighRgLimit & 0x0FFFu;
        pstStored->stCTLimit.u16HighBgLimit =
            pstAdvAWBAttr->stCTLimit.u16HighBgLimit & 0x0FFFu;
        pstStored->stCTLimit.u16LowRgLimit =
            pstAdvAWBAttr->stCTLimit.u16LowRgLimit & 0x0FFFu;
        pstStored->stCTLimit.u16LowBgLimit =
            pstAdvAWBAttr->stCTLimit.u16LowBgLimit & 0x0FFFu;
    }

    return HI_SUCCESS;
}


HI_S32 HI_MPI_ISP_GetAdvAWBAttr(ISP_ADV_AWB_ATTR_S *pstAdvAWBAttr)
{
    AWB_DEBUG_PRINT(
        "Calling HI_MPI_ISP_GetAdvAWBAttr with params: pstAdvAWBAttr=%p\n",
        (void *)pstAdvAWBAttr);

    if (pstAdvAWBAttr == HI_NULL)
        return HI_ERR_ISP_NULL_PTR;

    *pstAdvAWBAttr = g_astAwbStubCtx[0].stAdvAwbAttr;

    /* Match the bit masks used by the original VREG-backed getter. */
    pstAdvAWBAttr->bAccuPrior &= 1;
    pstAdvAWBAttr->bGainNormEn &= 1;
    pstAdvAWBAttr->stInOrOut.bEnable &= 1;
    pstAdvAWBAttr->stInOrOut.enOpType =
        (ISP_OP_TYPE_E)((HI_U32)pstAdvAWBAttr->stInOrOut.enOpType & 1u);
    pstAdvAWBAttr->stInOrOut.bOutdoorStatus &= 1;
    pstAdvAWBAttr->stInOrOut.bGreenEnhanceEn &= 1;
    pstAdvAWBAttr->stCTLimit.bEnable &= 1;
    pstAdvAWBAttr->stCTLimit.enOpType =
        (ISP_OP_TYPE_E)((HI_U32)pstAdvAWBAttr->stCTLimit.enOpType & 1u);
    pstAdvAWBAttr->stCTLimit.u16HighRgLimit &= 0x0FFFu;
    pstAdvAWBAttr->stCTLimit.u16HighBgLimit &= 0x0FFFu;
    pstAdvAWBAttr->stCTLimit.u16LowRgLimit &= 0x0FFFu;
    pstAdvAWBAttr->stCTLimit.u16LowBgLimit &= 0x0FFFu;

    AWB_DEBUG_PRINT(
        "  result: accuPrior=%d, tolerance=%u, curve=[%u..%u], "
        "gainNorm=%d, inOutEnable=%d, ctLimitEnable=%d\n",
        pstAdvAWBAttr->bAccuPrior,
        pstAdvAWBAttr->u8Tolerance,
        pstAdvAWBAttr->u16CurveLLimit,
        pstAdvAWBAttr->u16CurveRLimit,
        pstAdvAWBAttr->bGainNormEn,
        pstAdvAWBAttr->stInOrOut.bEnable,
        pstAdvAWBAttr->stCTLimit.bEnable);

    return HI_SUCCESS;
}


HI_S32 HI_MPI_ISP_SetAWBAlgType(ISP_AWB_ALG_TYPE_E enALGType)
{
    AWB_DEBUG_PRINT(
        "Calling HI_MPI_ISP_SetAWBAlgType with params: enALGType=%d\n",
        enALGType);

    if ((HI_U32)enALGType >= (HI_U32)AWB_ALG_BUTT) {
        puts("Invalid AWB ALG type!");
        return HI_ERR_ISP_ILLEGAL_PARAM;
    }

    /* Logical replacement for the original byte VREG at 0x30164. */
    g_astAwbStubCtx[0].enAwbAlgType = enALGType;

    return HI_SUCCESS;
}


HI_S32 HI_MPI_ISP_GetAWBAlgType(ISP_AWB_ALG_TYPE_E *penALGType)
{
    AWB_DEBUG_PRINT(
        "Calling HI_MPI_ISP_GetAWBAlgType with params: penALGType=%p\n",
        (void *)penALGType);

    if (penALGType == HI_NULL)
        return HI_ERR_ISP_NULL_PTR;

    *penALGType = g_astAwbStubCtx[0].enAwbAlgType;

    AWB_DEBUG_PRINT("  AWB algorithm type=%d\n", *penALGType);

    return HI_SUCCESS;
}
