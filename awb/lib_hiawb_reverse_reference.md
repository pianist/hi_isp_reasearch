# `lib_hiawb.so` и AWB старого HiSilicon ISP: объединённая карта реверса

## 1. Назначение и статус документа

Это единый рабочий справочник по закрытой AWB-библиотеке старого 32-битного HiSilicon SDK. Он объединяет:

- фактический callback ABI `Init/Run/Ctrl/Exit`;
- раскладку statistics buffer и аппаратные источники статистики;
- восстановленный внутренний контекст `g_astAwbCtx`;
- карту AWB VREG;
- назначение изученных функций `lib_hiawb.so`;
- аппаратные MMIO-регистры ISP, обсуждавшиеся при реверсе;
- подтверждённые выводы, гипотезы и оставшиеся слепые места.

При конфликте источников принят следующий приоритет:

1. реальные ARM call sites и обращения по смещениям;
2. парные setter/getter и документация аппаратных регистров;
3. заголовки именно этой ветки SDK;
4. имена, восстановленные IDA;
5. физические гипотезы.

Метки достоверности:

- **подтверждено** — прямо следует из кода, ABI или документации регистра;
- **вероятно** — согласуется с несколькими независимыми участками кода;
- **неизвестно** — точный исходный смысл или единицы пока не доказаны.

Актуальная IDA-разметка хранится отдельно в `awb_ctx_ida.h`. Этот документ объясняет её смысл, но не заменяет проверку смещений через `_Static_assert`.

## 2. Граница системы

Исследуемый тракт выглядит так:

```text
sensor driver
  └─ cmos_get_awb_default()
       ├─ reference temperature
       ├─ static WB gain offsets
       ├─ six Planck/CCT coefficients
       ├─ three calibrated CCMs
       └─ saturation-by-ISO table

ISP hardware
  ├─ отбирает reference-white pixels
  ├─ считает global AWB ratios/count
  └─ считает 255 zonal ratios/count
             ↓
kernel ISP driver
  └─ копирует MMIO statistics в software ping-pong buffer
             ↓
ISP userspace framework
  └─ формирует ISP_AWB_INFO_S и вызывает lib_hiawb
             ↓
lib_hiawb.so
  ├─ оценивает illuminant и CCT
  ├─ выдаёт четыре WB gains
  ├─ интерполирует CCM
  └─ предлагает metering limits следующего кадра
             ↓
ISP framework / driver
  └─ программирует WB, CCM и AWB metering registers
```

Существенно: hardware уже выполнил первичный цветовой и яркостный отбор «белых» пикселей. `lib_hiawb.so` не получает Bayer-кадр и не ищет белые пиксели с нуля; она получает агрегированные ratios и population.

## 3. Целевая ABI-платформа

```text
CPU:        ARMv5TEJ
ABI:        32-bit ARM
endianness: little-endian
pointer:    4 bytes
HI_BOOL:    4 bytes
```

Базовые типы:

```c
typedef signed int     HI_S32;
typedef unsigned int   HI_U32;
typedef signed short   HI_S16;
typedef unsigned short HI_U16;
typedef unsigned char  HI_U8;
typedef HI_S32         HI_BOOL;
```

AWB handle — `ALG_LIB_S.s32Id`, а не ISP device number. Поддерживаются только `0` и `1`.

```text
handle 0 → AWB VREG 0x30000
handle 1 → AWB VREG 0x31000
```

## 4. Callback ABI

### 4.1 Экспортируемая таблица

```c
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
```

В `lib_hiawb.so` им соответствуют `AwbInit`, `AwbRun`, `AwbCtrl`, `AwbExit`.

```c
typedef struct {
    HI_S32  s32Id;
    HI_CHAR acLibName[20];
} ALG_LIB_S; /* 0x18 */

typedef struct {
    ISP_AWB_INIT_FN pfn_awb_init;
    ISP_AWB_RUN_FN  pfn_awb_run;
    ISP_AWB_CTRL_FN pfn_awb_ctrl;
    ISP_AWB_EXIT_FN pfn_awb_exit;
} ISP_AWB_EXP_FUNC_S; /* 0x10 */
```

### 4.2 `ISP_AWB_PARAM_S`: различие ревизий

Заголовок SDK объявляет:

```c
typedef struct {
    SENSOR_ID SensorId;
    HI_S32    s32Rsv;
} ISP_AWB_PARAM_S; /* 0x08 в заголовке */
```

Однако исследуемый `AwbInit()` читает только первое слово `SensorId`, а разобранный framework call site содержательно передаёт только его. Поэтому:

- первые 4 байта подтверждены как реальный контракт;
- второй `s32Rsv` допустим для source-совместимости с header;
- нельзя переносить размер структуры между ревизиями SDK без проверки конкретного caller.

### 4.3 Регистрация сенсора

```c
typedef struct {
    HI_S32 (*pfn_cmos_get_awb_default)(AWB_SENSOR_DEFAULT_S *pstDefault);
} AWB_SENSOR_EXP_FUNC_S;
```

`HI_MPI_AWB_SensorRegCallBack()`:

1. проверяет имя библиотеки `"hisi_awb_lib"`;
2. вызывает `pfn_cmos_get_awb_default(&ctx->stSnsDefault)`, если callback задан;
3. сохраняет `bSnsRegistered`, `SensorId` и указатель callback;
4. не хранит `AWB_SENSOR_DEFAULT_S` как отдельный глобальный объект — defaults являются частью каждого `AWB_CTX_S`.

## 5. Statistics buffer ISP

### 5.1 Выделение и очереди

Kernel driver выделяет один MMZ-блок:

```c
phy  = CMPI_MmzMalloc(NULL, "ISPStat", 0x2838);
virt = ioremap(phy, 0x2838);
```

Внутри находятся два software ping-pong buffer:

```text
buffer A: base + 0x0000
buffer B: base + 0x141C
0x2838 = 2 × 0x141C
```

Статистику заполняет сам driver чтением ISP MMIO; отдельный statistics DMA в этом тракте не используется.

Очереди:

```text
FREE → BUSY/READY → USER → FREE
```

Перед публикацией нового кадра старый непрочитанный `BUSY` buffer возвращается в `FREE`. Таким образом, сохраняется самая свежая статистика, а не бесконечная очередь кадров.

### 5.2 Итоговая раскладка одного buffer

| Offset | Size | Блок |
|---:|---:|---|
| `0x0000` | `0x00C` | global AE 5-range statistics |
| `0x000C` | `0x9FA` | AE zonal histogram, 255 zones |
| `0x0A06` | `0x200` | AE 256-bin histogram |
| `0x0C06` | `0x002` | alignment/reserved |
| `0x0C08` | `0x008` | global AWB statistics |
| `0x0C10` | `0x5FA` | zonal AWB statistics |
| `0x120A` | `0x208` | AF header + 255 zone metrics |
| `0x1412` | `0x002` | hot/defect pixel count |
| `0x1414` | `0x008` | four current hardware WB gains |
|  | `0x141C` | total |

```c
typedef struct {
    ISP_STAT_BUFFER_S astBuffer[2];
} ISP_STAT_MMZ_S;

_Static_assert(sizeof(ISP_STAT_BUFFER_S) == 0x141C, "");
_Static_assert(sizeof(ISP_STAT_MMZ_S)    == 0x2838, "");
```

### 5.3 Вход `AwbRun`

```c
typedef struct {
    HI_U32 u32FrameCnt;             /* +0x00 */
    ISP_AWB_STAT_1_S *pstAwbStat1;  /* +0x04 */
    ISP_AWB_STAT_2_S *pstAwbStat2;  /* +0x08 */
} ISP_AWB_INFO_S;                    /* 0x0C */
```

Framework передаёт:

```text
pstAwbStat1 = statistics buffer + 0x0C08
pstAwbStat2 = statistics buffer + 0x0C10
s32Reserved = 0
```

Global AWB:

```c
typedef struct {
    HI_U16 u16Ratio0;              /* +0x00, unsigned Q4.8 */
    HI_U16 u16Ratio1;              /* +0x02, unsigned Q4.8 */
    HI_U32 u32WhitePointCount;     /* +0x04 */
} ISP_AWB_STAT_1_S;                 /* 0x08 */
```

Zonal AWB:

```c
typedef struct {
    HI_U16 au16Ratio0[255];             /* +0x000 */
    HI_U16 au16Ratio1[255];             /* +0x1FE */
    HI_U16 au16WhitePointCount[255];    /* +0x3FC */
} ISP_AWB_STAT_2_S;                      /* 0x5FA */
```

Третье поле zonal statistics — **16-битное количество reference-white pixels**, а не `HI_U32 Sum[255]`.

### 5.4 Направление ratios

Аппаратная документация разных мест этой ревизии противоречит сама себе: встречаются `R/G` против `G/R`. Поэтому в точной ABI-разметке безопаснее оставлять нейтральные `Ratio0/Ratio1` либо исторические имена `MeteringAwbRg/Bg`.

Подтверждено:

- формат — unsigned Q4.8;
- `0x100 == 1.0`;
- первый и второй каналы соответствуют красно-зелёной и сине-зелёной координатам;
- hardware selection limits `Cr/Cb` задаются как `R/G` и `B/G`;
- дополнительные light-source points API задаёт в пространстве correction gains `G/R` и `G/B`.

Не подтверждено окончательно: отдаёт ли statistics register прямое отношение сенсорных каналов или уже reciprocal correction ratio. Это нужно доказывать экспериментом на известном illuminant либо локальными уравнениями `AwbWbMatrixCalculate`.

## 6. Выход AWB

```c
typedef struct {
    HI_BOOL bChange;                          /* +0x00 */
    HI_U16  u16MeteringWhiteLevelAwb;         /* +0x04 */
    HI_U16  u16MeteringBlackLevelAwb;         /* +0x06 */
    HI_U16  u16MeteringCrRefMaxAwb;           /* +0x08 */
    HI_U16  u16MeteringCbRefMaxAwb;           /* +0x0A */
    HI_U16  u16MeteringCrRefMinAwb;           /* +0x0C */
    HI_U16  u16MeteringCbRefMinAwb;           /* +0x0E */
} ISP_AWB_STAT_ATTR_S;                         /* 0x10 */

typedef struct {
    HI_U32 au32WhiteBalanceGain[4];           /* +0x00 */
    HI_U16 au16ColorMatrix[9];                /* +0x10 */
    HI_U16 u16Padding022;                     /* +0x22 */
    ISP_AWB_STAT_ATTR_S stStatAttr;           /* +0x24 */
} ISP_AWB_RESULT_S;                            /* 0x34 */
```

Важно: порядок четырёх color-limit полей в ABI — `CrMax, CbMax, CrMin, CbMin`. Это подтверждается `hi_comm_3a.h` и 32-битными копированиями в `AwbRun`. Он не совпадает с порядком физических адресов, где регистры сгруппированы как `CrMax, CrMin, CbMax, CbMin`.

### 6.1 White-balance gains

```text
[0] R
[1] Gr
[2] Gb
[3] B
```

Формат результата — Q16:

```text
0x00010000 = 1.0
```

Рабочие автоматические R/B gains внутри WB context в основном Q8. Перед возвратом они умножаются на Q8 sensor gain offsets, что даёт Q16.

### 6.2 CCM

Порядок 3×3 row-major:

```text
RR RG RB
GR GG GB
BR BG BB
```

Публичное представление коэффициента — 16-битный direct/sign-magnitude с 8 дробными битами; внутреннее представление — signed two's-complement Q8. Переход выполняют `AwbDirectToComplement` и `AwbComplementToDirect`.

### 6.3 Metering limits

```text
WhiteLevel: 10 bits
BlackLevel: 12 bits
Cr/Cb:      unsigned Q4.8
```

При `bChange == 0` framework сохраняет прежние hardware limits. При WDR framework переводит thresholds в square-root domain:

```c
White = round_sqrt(White << 10);
Black = round_sqrt(Black << 10);
CrCb  = round_sqrt(CrCb  << 8);
```

## 7. Sensor defaults

```c
typedef struct {
    HI_U16 u16WbRefTemp;          /* +0x00 */
    HI_U16 au16GainOffset[4];     /* +0x02 */
    HI_U8  padding[2];            /* +0x0A */
    HI_S32 as32WbPara[6];         /* +0x0C */
    AWB_AGC_TABLE_S stAgcTbl;     /* +0x24 */
    AWB_CCM_S stCcm;              /* +0x30 */
} AWB_SENSOR_DEFAULT_S;            /* 0x6C */
```

Шесть `WbPara`:

```text
[0] p2
[1] p1
[2] q1
[3] a1
[4] b1
[5] c1
```

`p2/p1/q1` описывают Planckian curve в координатах сенсора. `a1/b1/c1` задают отображение положения на curve в color temperature/mired. В исследуемом коде `a1` используется как делитель и при нуле заменяется на `1`; `b1` калибровочный tool требует равным `128`, но его явное участие в известных `AwbToMired/AwbToRgBg` пока не найдено.

Пример калибровки сенсора с `u16WbRefTemp = 5000`, static gains и тремя CCM — не runtime-результат AWB, а model/tuning конкретного сенсора.

### 7.1 Физический смысл CCM

WB gains делают нейтральный объект нейтральным, компенсируя цвет источника света. CCM решает другую задачу: переводит спектральные отклики конкретного сенсора в целевое цветовое пространство и исправляет перекрёстную чувствительность каналов.

Три матрицы нужны потому, что спектральная ошибка сенсора зависит от illuminant. Библиотека интерполирует low/mid/high CCM по оценённой CCT.

### 7.2 Saturation AGC table

```c
typedef struct {
    HI_BOOL bValid;
    HI_U8   au8Saturation[8];
} AWB_AGC_TABLE_S;
```

Точки соответствуют ISO:

```text
100, 200, 400, 800, 1600, 3200, 6400, 12800
```

Типичная убывающая таблица снижает насыщенность при росте шума на высоком ISO.

## 8. Внутренний контекст `g_astAwbCtx`

Глобальный массив содержит два элемента:

```c
AWB_CTX_S g_astAwbCtx[2];
```

Размер одного элемента:

```text
0x464 bytes = 1124 bytes = 281 DWORD
```

Подтверждённые размеры:

| Структура | Размер |
|---|---:|
| `AWB_CTX_S` | `0x464` |
| `AWB_WB_CTX_S` | `0x2C8` |
| `AWB_CM_CTX_S` | `0x0A4` |
| `AWB_SENSOR_DEFAULT_S` | `0x06C` |
| `ISP_AWB_RESULT_S` | `0x034` |
| `AWB_DEBUG_CTX_S` | `0x028` |

Root layout:

| Offset | Поле | Смысл |
|---:|---|---|
| `0x000` | `bInitialized` | instance прошёл `AwbInit` |
| `0x004` | `u32Unknown004` | неизвестно |
| `0x008` | `bGainNormEn` | отдельная final-gain normalization |
| `0x00C` | `enAppliedWdrMode` | уже применённый WDR mode |
| `0x010` | `enRequestedWdrMode` | запрошенный WDR mode |
| `0x014` | `stWb` | WB estimator state, `0x2C8` |
| `0x2DC` | `stCm` | CCM/saturation state, `0x0A4` |
| `0x380` | `u32FrameCnt` | текущий frame number |
| `0x390` | `stResult` | cached `ISP_AWB_RESULT_S` |
| `0x3C4` | `bSnsRegistered` | sensor callback зарегистрирован |
| `0x3C8` | `SensorId` | зарегистрированный sensor ID |
| `0x3CC` | `stSnsDefault` | текущая sensor calibration |
| `0x438` | `stSnsRegister` | callback table |
| `0x43C` | `stDebug` | debug ring mapping/state |

Ключевые области WB context:

| Offset | Поле/область | Статус |
|---:|---|---|
| `0x000…0x007` | enable, changed, strengths, zone selection | в основном подтверждено |
| `0x008` | reference CCT | подтверждено |
| `0x00C` | global population threshold | подтверждено |
| `0x010/0x014` | high/low CCT limits, K/100 | подтверждено |
| `0x018` | four manual gains | подтверждено |
| `0x020` | four sensor gain offsets | подтверждено |
| `0x028` | six WbPara | подтверждено |
| `0x040` | reference-scene age | вероятно, хорошо подтверждено использованием |
| `0x044` | global white-point count | подтверждено debug output |
| `0x048/0x04A` | working R/B correction gains Q8 | подтверждено |
| `0x04C` | signed Planck offset | подтверждено |
| `0x04E` | current CCT | подтверждено |
| `0x050` | convergence speed Q12 | подтверждено |
| `0x068` | mired Q8 | подтверждено |
| `0x06C/0x070` | filtered R/B gains Q16 | подтверждено |
| `0x074/0x075` | status/algorithm flags | биты частично сопоставлены API |
| `0x094…0x25F` | 20 reference zones and cache | форма подтверждена, часть семантики нет |
| `0x270` | command-8005 runtime value | единицы неизвестны |
| `0x294` | four light-source records | подтверждено |

CM context:

| Offset | Поле | Смысл |
|---:|---|---|
| `0x00` | manual WB/CM mode byte | mode bits из VREG `+0x000` |
| `0x04` | manual saturation enable | подтверждено |
| `0x08/0x09` | target/current saturation | подтверждено |
| `0x0C/0x10` | current/new CCM region | подтверждено |
| `0x14` | current CCT | подтверждено |
| `0x18` | filtered CCT Q8 | подтверждено |
| `0x1C` | 8-byte saturation table | подтверждено |
| `0x24` | ISO | подтверждено |
| `0x28` | three calibration CCMs | подтверждено |
| `0x64` | output CCM | подтверждено |
| `0x76` | interpolated CCM | подтверждено |
| `0x88` | saturation matrix | подтверждено |

## 9. Жизненный цикл библиотеки

### 9.1 `AwbInit`

```c
if (handle > 1 || !param)
    return error;

ctx = &g_astAwbCtx[handle];

if (!ctx->bSnsRegistered)
    return error;

if (ctx->SensorId != param->SensorId)
    return error;

VReg_Init(AWB_VREG_BASE(handle), 0x1000);
AwbExtRegsDefault(handle);
AwbExtRegsInitialize(handle);

ctx->enAppliedWdrMode   = 0;
ctx->enRequestedWdrMode = 0;

AwbWbInit(&ctx->stWb, &ctx->stSnsDefault);
AwbCmInit(&ctx->stCm, &ctx->stSnsDefault);
ctx->bInitialized = HI_TRUE;
```

### 9.2 `AwbRun`

`AwbRun` вызывается каждый кадр, но тяжёлый WB/CCM расчёт выполняется только на чётных frame numbers. На нечётных кадрах возвращается cached result.

```c
validate(handle, info, result, info->pstAwbStat1, info->pstAwbStat2);

ctx->u32FrameCnt = info->u32FrameCnt;

if (frame == 1)
    AwbIspRegsInit(handle);       // формирует cached result, не пишет MMIO

AwbReadExtRegs(handle);

if (appliedWdr != requestedWdr) {
    AwbSetWdrMode(handle);
    ctx->stResult.stStatAttr.bChange = HI_TRUE;
} else {
    ctx->stResult.stStatAttr.bChange = HI_FALSE;
}

AwbDbgRunBgn(handle);

if ((frame & 1) == 0) {
    AwbWbMatrixCalculate(handle, info, &ctx->stWb);
    AwbColorMatrixCalculate(handle, info, &ctx->stCm);
    build_four_output_gains(ctx);
    copy_output_ccm_to_cached_result(ctx);
}

AwbUpdateExtRegs(handle);
AwbDbgRunEnd(handle, info);
*result = ctx->stResult;
```

Manual path использует четыре Q8 manual gains. В WDR branch они могут возводиться в квадрат; в linear branch сдвигаются на 8, получая Q16.

Auto path умножает рабочие gains на sensor gain offsets. При `bGainNormEn` выполняется общая амплитудная нормализация четырёх каналов с учётом `WB_BLACK01`; относительные цветовые отношения сохраняются.

### 9.3 `AwbSetWdrMode`

Это полная перезагрузка sensor-dependent state:

```c
if (ctx->stSnsRegister.pfn_cmos_get_awb_default)
    callback(&ctx->stSnsDefault);

AwbExtRegsInitialize(handle);
AwbWbInit(&ctx->stWb, &ctx->stSnsDefault);
AwbCmInit(&ctx->stCm, &ctx->stSnsDefault);
AwbReadExtRegs(handle);
ctx->enAppliedWdrMode = ctx->enRequestedWdrMode;
```

### 9.4 `AwbExit`

Вызывает `VReg_Exit(base, 0x1000)` и сбрасывает `bInitialized` только при успехе. Sensor registration и остальные bytes контекста не очищаются.

## 10. Команды `AwbCtrl`

| Cmd | Payload | Действие |
|---:|---|---|
| `0` | scalar pointer | saturation target set, VREG `+0x052` |
| `1` | scalar pointer | saturation target get |
| `2` | `AWB_DEBUG_INFO_S *` | debug set |
| `3` | `AWB_DEBUG_INFO_S *` | debug get |
| `8000` | `HI_BOOL *` | requested WDR mode |
| `8001` | proc buffer descriptor | `AwbProcWrite` |
| `8002` | framework value | игнорируется этой AWB library |
| `8003` | `HI_U32 *` | ISO → VREG `+0x160` |
| `8005` | `HI_U32 *` | runtime value → VREG `+0x1AC` |

`AwbCtrl` проверяет handle и `bInitialized`; неизвестные commands внутренний dispatcher в большинстве случаев молча принимает.

Цепочка `8005`:

```text
AE Ctrl command 8004
  → ISP context runtime field
  → AWB Ctrl command 8005
  → VREG +0x1AC
  → WB context +0x270
```

Default `0x100`. Значение участвует в выражении вида `ISO * value / 100`, поэтому название `ISP digital gain Q8` остаётся гипотезой: масштаб деления не согласован с обычным Q8 без дополнительного преобразования.

## 11. AWB VREG

### 11.1 Адресация

```c
#define AWB_VREG_BASE(handle) (((handle) + 48) << 12)
```

Адреса `0x30000…0x30FFF` и `0x31000…0x31FFF` — virtual registers библиотеки. Они не являются физическими ISP MMIO `0x205A....`.

Жизненный цикл:

```text
AwbExtRegsDefault      writes library fallback defaults
AwbExtRegsInitialize   overlays sensor calibration
MPI setters / Ctrl     update tuning/runtime inputs
AwbReadExtRegs         VREG → context
AwbUpdateExtRegs       context outputs → VREG
```

### 11.2 Core, calibration и manual controls

| Off | Width | Поле | Писатель | Читатель | Физический смысл |
|---:|---:|---|---|---|---|
| `000` | U16 bit 1 | requested WDR | `AwbCtrlCmd(8000)` | `AwbReadExtRegs` | linear/WDR request |
| `000` | U16 bit 3 | auto AWB enable | `HI_MPI_ISP_SetWBType` | `AwbReadExtRegs` | auto estimator enable |
| `000` | U16 bits 5:4 | WB op type | `HI_MPI_ISP_SetWBType` | `AwbReadExtRegs` | AUTO/MANUAL |
| `002` | U8 | zone selection | `HI_MPI_ISP_SetAWBAttr` | WB calculation | global/zonal policy; exact enum revision-dependent |
| `004` | U16 | global population threshold | default/private | WB calculation | минимум white points для global update |
| `008` | U8 | R response strength | `HI_MPI_ISP_SetAWBAttr` | WB calculation | scales deviation from unity, `128` nominal |
| `009` | U8 | B response strength | тот же | WB calculation | аналогично для B |
| `00A` | U8 | low CCT / 100 K | тот же | WB calculation | default `25 = 2500 K` |
| `00B` | U8 | high CCT / 100 K | тот же | WB calculation | default `100 = 10000 K` |
| `00D` | U8 | private | default/private | WB calculation | неизвестно |
| `00E` | U8 | geometry scale | default/private | WB calculation | Planckian/zonal metric, точное имя неизвестно |
| `010` | U16 | reference CCT | sensor/API | `AwbReadExtRegs` | sensor calibration temperature |
| `012` | U16 | current CCT | `AwbUpdateExtRegs` | `HI_MPI_ISP_GetColorTemp` | runtime output, K |
| `014…01A` | U16×4 | static WB offsets | sensor/API | WB init/read | R,Gr,Gb,B Q8 |
| `02C…032` | U16×4 | manual WB gains | manual-WB setter | `AwbReadExtRegs`/`AwbRun` | R,Gr,Gb,B Q8 |
| `034…048` | S32×6 | p2,p1,q1,a1,b1,c1 | sensor/API | Planck/CCT transforms | calibrated curve model |
| `04C` | U16 | speed | `HI_MPI_ISP_SetAWBAttr` | normalization | Q12 convergence coefficient |

### 11.3 Debug, saturation и CCM

| Off | Width | Поле | Писатель | Читатель |
|---:|---:|---|---|---|
| `01E` | U16 | debug enable | `AwbDbgSet` | `AwbDbgGet`, debug run |
| `020` | U32 | debug physical address | `AwbDbgSet` | debug mapping |
| `024` | U32 | mapped size | `AwbDbgSet` | debug mapping |
| `028` | U32 | debug depth | `AwbDbgSet` | debug ring |
| `050` | U8 bit0 | manual saturation enable | `HI_MPI_ISP_SetSaturationAttr` | saturation calculation |
| `052` | U8 | saturation target | setter / Ctrl 0 | getter / saturation calculation |
| `05C…07C` | U16×9 stride4 | current/output CCM | init/update | status/readback |
| `080…100` | U16×9 stride4 | mid CCM | sensor/`HI_MPI_ISP_SetCCM` | CCM interpolation |
| `104…124` | U16×9 stride4 | low CCM | тот же | CCM interpolation |
| `128` | U16 | low CCM CCT | тот же | CCM interpolation |
| `12C` | U16 | mid CCM CCT | тот же | CCM interpolation |
| `130` | U16 | high CCM CCT | тот же | CCM interpolation |
| `134…154` | U16×9 stride4 | high CCM | тот же | CCM interpolation |
| `158…15F` | U8×8 | saturation ISO table | sensor/`SetSaturationAttr` | saturation calculation |

CCM VREG хранит internal signed/complement Q8. Public MPI setter/getter конвертирует representation.

### 11.4 Runtime и advanced AWB

| Off | Width | Поле | Писатель | Читатель/действие |
|---:|---:|---|---|---|
| `160` | U32 | ISO | `AwbCtrlCmd(8003)` | WB and saturation |
| `164` | U8 | algorithm type | `HI_MPI_ISP_SetAWBAlgType` | default/advanced aggregation |
| `165` | U8 | `bAccuPrior` | `SetAdvAWBAttr` | candidate preference |
| `166` | U8 | candidate population threshold | private/default | candidate blending |
| `167` | U8 | adaptive threshold | private/default | точный смысл неизвестен |
| `168` | U16 | curve left limit | `SetAdvAWBAttr` | Planck filtering |
| `16A` | U16 | curve right limit | `SetAdvAWBAttr` | Planck filtering |
| `16C` | U8 | reference-zone cache enable | private/default | scene stability |
| `16D` | U8 | tolerance | `SetAdvAWBAttr` | reference comparisons |
| `16E` | U8 | reference refresh interval | private/default | cache age limit |
| `16F` | U8 | additional lights enable | `SetLightSource` | light-source rules |
| `190` | U16 | fallback R clip gain | private/default | CT fallback |
| `192` | U16 | fallback companion gain | private/default | CT fallback |
| `194` | U8 | global stability enable | private/default | stability counter |
| `195` | U8 | stable-frame threshold | private/default | zonal/reference gate |
| `196` | U8 | reference relate threshold | private/default | `AwbGlobalRelateCoef` |
| `197` | U8 | private candidate normalization | private/default | algorithm flag `0x08` |
| `198` | U8 | CT-limit enable | `SetAdvAWBAttr` | clipping |
| `199` | U8 | CT-limit op type | `SetAdvAWBAttr` | AUTO/MANUAL clipping |
| `19A…1A0` | U16×4 | CT-limit correction gains | `SetAdvAWBAttr` | high/low R/B boundaries |
| `1A2` | U8 | Planck-offset clipping enable | private/default | `AwbCoefClip` |
| `1A3` | U8 | Planck-offset limit | private/default | `AwbCoefClip` |
| `1A4` | U8 | in/out enable | `SetAdvAWBAttr` | indoor/outdoor state machine |
| `1A5` | U8 | in/out op type | `SetAdvAWBAttr` | AUTO/MANUAL |
| `1A6` | U8 | outdoor status | API or `AwbUpdateExtRegs` | input/status |
| `1A7` | U8 | ISO/CCT candidate weighting | private/default | exact API name unknown |
| `1A8` | U8 | `bGainNormEn` | `SetAdvAWBAttr` | final gain normalization |
| `1A9` | U8 | green enhance | `SetAdvAWBAttr` | outdoor correction |
| `1AC` | U32 | command-8005 runtime value | framework | units unknown |
| `1B0` | U32 | outdoor threshold | `SetAdvAWBAttr` | in/out state machine |
| `1B4…1BA` | U16×4 | low/high start/stop | `SetAdvAWBAttr` | CCT transition intervals |

`+0x164` default равен `AWB_ALG_ADVANCE`. `+0x1A8` — отдельный root-context Boolean и не должен объединяться с private algorithm flag `+0x197`.

### 11.5 Status и algorithm flag packing

`AwbReadExtRegs()` собирает internal `u8StatusFlags`:

| Bit | VREG | Смысл |
|---:|---:|---|
| `01` | `164` | advanced algorithm |
| `02` | `165` | accuracy priority |
| `04` | `194` | global stability |
| `08` | `16C` | reference-zone cache |
| `10` | `16F` | additional lights |
| `20` | `1A4` | indoor/outdoor enable |
| `40` | `1A5` | indoor/outdoor manual |
| `80` | `1A6` | outdoor state |

Internal `u8AlgorithmFlags`:

| Bit | VREG | Смысл |
|---:|---:|---|
| `01` | `198` | CT-limit enable |
| `02` | `199` | CT-limit manual |
| `04` | `1A2` | Planck/deviation clipping |
| `08` | `197` | private candidate normalization |
| `10` | `1A7` | ISO/CCT candidate weighting |
| `20` | `1A9` | green enhancement |

### 11.6 Additional light-source records

Четыре записи по 8 байт:

```c
typedef struct {
    HI_U16 u16WhiteRgain;  /* correction gain G/R */
    HI_U16 u16WhiteBgain;  /* correction gain G/B */
    HI_U16 u16ExpQuant;
    HI_U8  u8LightStatus;  /* 0 idle, 1 add, 2 delete */
    HI_U8  u8Radius;
} ISP_AWB_LIGHTSOURCE_INFO_S;
```

```text
record n: VREG +0x170 + 8*n, n=0…3
```

## 12. Аппаратные ISP-регистры

Все адреса в этом разделе — реальный MMIO с базой `0x205A0000`, не AWB VREG.

### 12.1 Геометрия и служебная статистика

| Address | Name/role | Описание |
|---:|---|---|
| `0x205A0010` | `ACTIVE_WIDTH` | активная ширина кадра |
| `0x205A0014` | `ACTIVE_HEIGHT` | активная высота кадра |
| `0x205A01EC` | hot/defect count | младшие 10 bits; statistics bit 6 |

### 12.2 Hardware WB

| Address | Поле | Формат/роль |
|---:|---|---|
| `0x205A02C0` | WB gain R | 12-bit unsigned Q4.8 |
| `0x205A02C4` | WB gain Gr | 12-bit unsigned Q4.8 |
| `0x205A02C8` | WB gain Gb | 12-bit unsigned Q4.8 |
| `0x205A02CC` | WB gain B | 12-bit unsigned Q4.8 |
| `0x205A02D0` | `WB_BLACK00`, R offset | 16-bit |
| `0x205A02D4` | `WB_BLACK01`, Gr offset | 16-bit; используется gain normalization |
| `0x205A02D8` | `WB_BLACK10`, Gb offset | 16-bit |
| `0x205A02DC` | `WB_BLACK11`, B offset | 16-bit |

Четыре gain registers копируются driver в конец statistics buffer как statistics bit 7.

### 12.3 CCM и неидентифицированные WB registers

| Address | Роль |
|---:|---|
| `0x205A0480…0x205A04A0` | девять hardware CCM coefficients |
| `0x205A04A8` | неизвестный WB/metering gain, init `0x100` |
| `0x205A04AC` | неизвестный WB/metering gain, init `0x100` |
| `0x205A04B0` | неизвестный WB/metering gain, init `0x100` |

### 12.4 AE histogram

| Address | Роль |
|---:|---|
| `0x205A0600` | `METERING_HIST_THRESH01` |
| `0x205A0604` | `METERING_HIST_THRESH12` |
| `0x205A0608` | `METERING_HIST_THRESH34` |
| `0x205A060C` | `METERING_HIST_THRESH45` |
| `0x205A0620` | outer global histogram 0 |
| `0x205A0624` | outer global histogram 1 |
| `0x205A0628` | outer global histogram 2 |
| `0x205A062C` | outer global histogram 3 |

Пятый эффективный диапазон восстанавливается из total/остатка. Zonal AE histogram нормирована к `0xFFFF`.

### 12.5 AWB reference-white selection

| Address | Register | Field |
|---:|---|---|
| `0x205A0640` | `METERING_AWB_WHITE_LEVEL` | upper brightness limit, 10 bits |
| `0x205A0644` | AWB black level | lower brightness limit, 12 bits |
| `0x205A0648` | `METERING_AWB_CR_REF_MAX` | max R/G, Q4.8 |
| `0x205A064C` | `METERING_AWB_CR_REF_MIN` | min R/G, Q4.8 |
| `0x205A0650` | `METERING_AWB_CB_REF_MAX` | max B/G, Q4.8 |
| `0x205A0654` | `METERING_AWB_CB_REF_MIN` | min B/G, Q4.8 |

Initial result:

```text
WhiteLevel = 940
BlackLevel = 64
R/G = 0.5…2.0
B/G = 0.5…2.0
```

### 12.6 Global и zonal AWB statistics

| Address | Роль |
|---:|---|
| `0x205A0658` | global red/green ratio, Q4.8; направление требует проверки |
| `0x205A065C` | global blue/green ratio, Q4.8; направление требует проверки |
| `0x205A0660` | global reference-white count, U32 |
| `0x205A0B00…0x205A0BFE` | 255 hardware zone weights, 4 bits each |
| `0x205A2800 + zone*8` | packed zonal ratios, low and high 12-bit halves |
| `0x205A2804 + zone*8` | zonal reference-white count, U16 |

### 12.7 AF statistics

| Address | Register | Описание |
|---:|---|---|
| `0x205A0680` | `METERING_AF_METRICS` | integrated normalized contrast metric, U16 |
| `0x205A0684` | `METERING_AF_THRESHOLD_WRITE` | preset threshold; `0` uses previous-frame threshold, U16 |
| `0x205A0688` | `METERING_AF_THRESHOLD_READ` | low16 calculated threshold; high16 calculated intensity |
| `0x205A068A` | upper halfword of `0x0688` | calculated AF intensity при 16-bit read, little-endian |
| `0x205A068C` | unknown AF field | driver использует младшие 4 bits |
| `0x205A0690` | `AF_NODES_USED` | low8 horizontal zones, high8 vertical zones |
| `0x205A0694` | `AF_NP_OFFSET` | AF noise-profile offset, unsigned Q4.4 |
| `0x205A3000 + zone*4` | zonal AF metric | low16, 255 zones |

Восстановленная AF payload:

```c
typedef struct {
    HI_U16 u16GlobalMetrics;             /* +0x000 */
    HI_U16 u16ThresholdWrite;            /* +0x002 */
    HI_U16 u16CalculatedThreshold;       /* +0x004 */
    HI_U16 u16CalculatedIntensity;       /* +0x006 */
    HI_U8  u8Unknown068C;                /* +0x008, low 4 bits */
    HI_U8  u8NoiseProfileOffsetQ4_4;     /* +0x009 */
    HI_U16 au16ZoneMetric[255];          /* +0x00A */
} ISP_AF_STAT_S;                          /* 0x208 */
```

`AF_NODES_USED` настраивает активную сетку, но в показанную payload driver его не копирует.

### 12.8 Statistics arrays

| Address pattern | Назначение | Статус имени |
|---:|---|---|
| `0x205A1800 + bin*4` | AE 256-bin histogram, читается U16 | физический смысл подтверждён, symbol неизвестен |
| `0x205A2000 + zone*8` | AE zonal outer bins 0/1 | symbol неизвестен |
| `0x205A2004 + zone*8` | AE zonal outer bins 4/5 | symbol неизвестен |
| `0x205A2800 + zone*8` | AWB zonal ratios | symbol неизвестен |
| `0x205A2804 + zone*8` | AWB zonal population | symbol неизвестен |
| `0x205A3000 + zone*4` | AF zonal contrast metric | точный symbol неизвестен |

## 13. Алгоритм AWB

### 13.1 Что происходит без AWB

Без компенсации источник света окрашивает весь кадр: лампа накаливания даёт тёплый красно-жёлтый cast, холодное небо — синий. Нейтральные объекты перестают быть нейтральными, а CCM сама по себе не обязана устранять этот global illuminant cast.

### 13.2 Общая последовательность

```c
hardware_select_reference_pixels();
stats = hardware_compute_global_and_zonal_ratios();

if (global_population_is_reliable(stats))
    global_candidate = derive_correction_gain(stats.global);

zones = build_255_zone_candidates(stats.zonal);
zones = reject_low_population_and_implausible_points(zones);

for each zone:
    convert_gain_point_to_mired_and_planck_offset();
    compute_confidence_and_scene_relation();

update_reference_zone_cache_and_stability();
build_four_candidate_groups();
sort_candidates_by_weight();
select_or_blend_candidates();

apply_R_and_B_strength();
clip_by_CCT_and_planck_offset();
temporally_filter_and_normalize();

color_temperature = mired_to_kelvin();
ccm = interpolate_sensor_ccm(color_temperature);
saturation = interpolate_saturation_by_iso();
output_ccm = saturation_matrix * ccm;
```

Если сцена почти целиком зелёная и настоящих neutral surfaces нет, pure gray-world оценка ошиблась бы. Здесь защита многоуровневая: hardware white-region window, population thresholds, Planckian distance, temporal stability, cached reference zones, candidate classes, optional indoor/outdoor logic и green enhancement. Абсолютной гарантии нет: при отсутствии нейтральной информации AWB вынуждена опираться на priors и историю.

### 13.3 Planckian curve и CCT

По curve не просто «сразу считается температура». Сначала наблюдаемая chromaticity point проектируется/сопоставляется с calibrated Planckian model, получаются:

- положение вдоль curve — mired Q8, затем CCT;
- signed offset от curve — мера непохожести на типичный black-body/daylight illuminant.

`AwbToMired` делает прямое преобразование gain/ratio point → `(miredQ8, signedOffset)`. `AwbToRgBg` делает обратное преобразование для clipping boundaries.

После оценки температуры:

1. выбираются WB correction gains;
2. gains ограничиваются по CCT/offset;
3. выполняются temporal filtering и normalization;
4. по CCT выбирается/interpolates sensor CCM;
5. saturation корректируется по ISO;
6. WB gains и CCM возвращаются framework.

### 13.4 CCM и saturation

`AwbColorMatrixCalculate` строит стандартную saturation matrix с luma weights `76,150,29`, умножает её на interpolated sensor CCM и при экстремальной CCT плавно смешивает результат с identity.

При saturation `0x80` матрица saturation — identity. При saturation `0` цветовые различия сводятся к luminance.

## 14. Каталог изученных функций

| Функция | Назначение | Статус/замечание |
|---|---|---|
| `AwbCcMatrixCalculate` | hysteretic low/mid/high CCM selection и interpolation по CCT | архитектура подтверждена; source-like полевая разметка ещё улучшаема |
| `AwbCmInit` | initial high CCM, state и 6500 K | подтверждено |
| `AwbCoefClip` | smooth CCT и Planck-offset clipping gains | подтверждено; часть private flags без API-имён |
| `AwbColorMatrixCalculate` | saturation matrix × interpolated CCM, extreme-CCT fade | подтверждено |
| `AwbCompareWeight` | `qsort` comparator по U32 weight at candidate `+0x0C`, descending | подтверждено |
| `AwbComplementToDirect` | internal two's-complement → public direct/sign-magnitude | роль подтверждена; corner cases требуют точного тела |
| `AwbCtrl` | handle/init guard вокруг dispatcher | подтверждено |
| `AwbCtrlCmd` | commands 0…3, 8000…8005 | подтверждено |
| `AwbDbgGet` | читает enable/phy/depth | подтверждено |
| `AwbDbgRunBgn` | map/re-map debug ring, global header, frame begin | подтверждено |
| `AwbDbgRunEnd` | global ratios/count, CCT, gains, CCM, frame end | подтверждено |
| `AwbDbgSet` | проверяет debug config, вычисляет `88 + 7188*depth` | подтверждено |
| `AwbDirectToComplement` | public direct/sign-magnitude → internal complement | роль подтверждена; corner cases требуют точного тела |
| `AwbExit` | `VReg_Exit`, сброс initialized | подтверждено |
| `AwbExtRegsDefault` | fallback VREG defaults | подтверждено |
| `AwbExtRegsInitialize` | sensor defaults → VREG | подтверждено |
| `AwbGlobalRelateCoef` | normalized dissimilarity двух gain points, saturates 255 | подтверждено |
| `AwbInit` | VREG/context initialization | подтверждено |
| `AwbIspRegsInit` | initial cached result на frame 1; MMIO не пишет | подтверждено |
| `AwbMatrixCopy` | copy 16-bit matrix | подтверждено |
| `AwbMatrixIdentity` | Q8 identity matrix | подтверждено |
| `AwbMatrixMultiply` | Q8 multiply и коррекция row sum к `0x100` | подтверждено |
| `AwbProcWrite` | PROC summary: gains, CCT, CCM, saturation, zones, speed | подтверждено; label `Zones` неоднозначен |
| `AwbReadExtRegs` | VREG → WB/CM/debug/root contexts | ключевая функция, большая часть размечена |
| `AwbRun` | per-frame orchestrator | подтверждено |
| `AwbSaturationCalculate` | manual либо ISO-table interpolation | подтверждено |
| `AwbSaturationOffsetCalculate` | absolute interpolation delta между ISO nodes | подтверждено |
| `AwbSetWdrMode` | reload sensor defaults и WB/CM state | подтверждено |
| `AwbSqrt32` | rounded integer square root | подтверждено |
| `AwbToMired` | gain point → mired Q8 + signed curve offset | подтверждено математически |
| `AwbToRgBg` | inverse Planck model | подтверждено математически |
| `AwbUpdateExtRegs` | current CCT/CCM/outdoor status → VREG | подтверждено |
| `AwbWbGainSet` | four sensor gain offsets → WB context | подтверждено |
| `AwbWbInit` | matrix init + gain set + WbPara set | подтверждено |
| `AwbWbMatrixCalculate` | основное global/zonal estimator ядро | общий pipeline восстановлен; candidate internals частично слепы |
| `AwbWbMatrixInit` | initializes gains, filters, 20-zone reference cache | подтверждено |
| `AwbWbMatrixNormalize` | temporal filtering и Q8 normalization | назначение подтверждено; некоторые intermediate fields ещё private |
| `AwbWbParaSet` | copy six calibrated curve coefficients, protects `a1` | подтверждено |

### 14.1 Математические helpers

`AwbGlobalRelateCoef` фактически возвращает error/dissimilarity, а не similarity:

```text
projectedB = Rgain * referenceB / referenceR
error = (|referenceR-Rgain| + |projectedB-Bgain|) * 256
        / sqrt(referenceR² + referenceB²)
error = min(error, 255)
```

`AwbMatrixMultiply`:

```text
Cij = (sum(Aik * Bkj) + 128) >> 8
Cii += 256 - sum(row_i)
```

Вторая операция сохраняет neutral gray axis: сумма каждой строки становится `0x100`.

`AwbSqrt32` округляет к ближайшему integer, а не всегда вниз:

```c
n = isqrt_floor(x);
return x > n * (n + 1) ? n + 1 : n;
```

## 15. Debug ABI

Фактический payload commands 2/3:

```c
typedef struct {
    HI_BOOL bEnable;
    HI_U32  u32PhyAddr;
    HI_U32  u32Depth;
} AWB_DEBUG_INFO_S; /* 0x0C */
```

Он может отличаться от большой public debug structure другой ревизии SDK.

Debug mapping:

```text
global header = 88 bytes
one record    = 7188 bytes = 0x1C14
map size      = 88 + depth * 7188
```

Каждая запись содержит begin/end frame numbers, global count/ratios, CCT, gains, CCM и 255 zonal debug entries. Begin/end позволяют читателю заметить частично записанный slot без mutex.

## 16. Fallback defaults

Ключевые defaults `AwbExtRegsDefault`:

```text
auto AWB                  enabled
manual WB                disabled
WDR                       disabled
zone selection            32
R/B response strength     128 / 128
global population limit   4096
low/high CCT              2500 / 10000 K
speed                     128 Q12
reference/current CCT     5000 K
manual/static gains       0x100
ISO                       100
saturation target         0x80
runtime value 8005        0x100
```

Fallback CCM, signed Q8:

```text
 464  -336   128
 -80   416   -80
 -48  -192   496
```

Сумма каждой строки равна `256`.

Fallback saturation table:

```c
{35, 44, 52, 62, 70, 78, 84, 84}
```

Sensor defaults затем перекрывают reference CCT, gain offsets, WbPara, CCM и, если `bValid`, saturation table.

## 17. Подтверждённые слепые места

### 17.1 ABI и framework

- размер фактического `ISP_AWB_PARAM_S` между близкими SDK revisions: library использует только первые 4 bytes, header объявляет 8;
- точные единицы command `8005` / WB context `+0x270`;
- политика framework при ошибке `AwbRun`: ранее восстановлено применение zero/cached result, но это следует ещё раз зафиксировать на конкретной ветке call site.

### 17.2 Основной estimator

- окончательное направление raw global/zonal ratio: direct sensor ratio или reciprocal correction gain;
- точная семантика `AWB_ZONE_WORK_S` offsets `+0x08`, `+0x0A`, `+0x12`;
- точная раскладка 0x24-byte candidate record;
- физический смысл четырёх candidate classes и их веса;
- обработка `count == 0` и low-count zones в source-like именах;
- точная связь hardware white-region selection с software Planckian rejection.

### 17.3 Context и private tuning

- root `AWB_CTX_S +0x004`;
- часть WB matrix fields `+0x052…+0x067` и tail `+0x280…`;
- CM tail `+0x09A…+0x0A3`;
- public/private имена VREG `+00D`, `+00E`, `+166`, `+167`, `+16E`, `+190`, `+192`, `+194…197`, `+1A2`, `+1A3`, `+1A7`;
- точные corner cases `AwbDirectToComplement` и `AwbComplementToDirect`.

### 17.4 Hardware

- имя/семантика `0x205A068C`;
- точные symbols массивов `0x1800`, `0x2000`, `0x2800`, `0x3000`;
- имена и назначение `0x205A04A8/04AC/04B0`;
- экспериментальное подтверждение направления AWB ratios.

## 18. Практический порядок следующего реверса

1. В `AwbWbMatrixCalculate` разметить 255 `AWB_ZONE_WORK_S` от места загрузки `WhitePointCount` до формирования четырёх candidate records.
2. Типизировать 0x24-byte candidate и подтвердить comparator field `+0x0C`.
3. Найти все writers private VREG offsets, особенно `+0x194`, `+0x197`, `+0x1A2`, `+0x1A7`.
4. Проследить AE command `8004` до AWB command `8005` и вывести единицы из producer, а не из default `0x100`.
5. Проверить raw ratio direction на сцене с известными channel means или по одному полностью разобранному global branch.
6. Зафиксировать тела direct/complement conversion для битовой совместимости CCM.
7. В ISP/framework проверить первый кадр, error path callback и точное программирование result fields в hardware registers.

## 19. Итог

Архитектура уже восстановлена достаточно, чтобы написать ABI-совместимую AWB-библиотеку поверх закрытого ISP framework:

- входные structures, размеры и offsets известны;
- statistics transport и hardware preselection известны;
- выходные gains, CCM и metering attributes известны;
- registration и `Init/Run/Ctrl/Exit` lifecycle известны;
- VREG interface в основном картирован.

Для функциональной совместимости необязательно побитово повторять оригинальный estimator. Для качественной замены всё ещё нужны аккуратные решения по confidence, mixed illuminants, green-dominated scenes, temporal stability и tuning конкретного сенсора. Для битовой совместимости требуется завершить candidate internals, private VREG и fixed-point corner cases.
