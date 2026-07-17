# Реверс `lib_hiawb.so`: функции и архитектура AWB

## Статус документа

Этот документ описывает изученную реализацию HiSilicon AWB из старого SDK. Он основан на заголовках SDK, debug-символах, декомпиляции функций и восстановленном `awb_ctx_ida.h`.

Принятые обозначения:

- **Подтверждено** — непосредственно следует из декомпиляции, структуры ABI или документации регистра.
- **Вероятно** — хорошо согласуется с несколькими участками кода, но исходное имя или физический смысл не доказаны.
- **Неизвестно** — требуется дополнительная декомпиляция, caller/callee либо документация VREG.

Структуры, смещения и константы здесь не дублируются полностью: предполагается, что рядом используется актуальный `awb_ctx_ida.h`.

## Общая архитектура

Библиотека реализует стандартный callback-интерфейс 3A:

```text
SensorRegCallBack
        ↓
AwbInit
        ↓
AwbRun (каждый кадр, тяжёлый расчёт — на чётных кадрах)
        ↓
AwbCtrl (управляющие команды)
        ↓
AwbExit
```

Поддерживаются два AWB-контекста, handles `0` и `1`. Размер одного контекста — `0x464`, то есть 281 `DWORD`. VREG-области:

| Handle | VREG base | Размер |
|---:|---:|---:|
| 0 | `0x30000` | `0x1000` |
| 1 | `0x31000` | `0x1000` |

Основной поток данных:

```text
ISP global/zonal AWB statistics
             ↓
        AwbWbMatrixCalculate
             ↓
  R/B gains, mired Q8, Planck offset
             ↓
  AwbCoefClip + AwbWbMatrixNormalize
             ↓
      4 Bayer WB gains

color temperature + sensor CCM calibration
             ↓
      AwbCcMatrixCalculate
             ↓
  interpolated CCM × saturation matrix
             ↓
      output 3×3 CCM
```

## Fixed-point форматы

| Величина | Формат |
|---|---|
| Внутренние CCM | signed Q8, unity `0x100` |
| Публичные CCM | 16-bit direct/sign-magnitude, затем перевод в complement |
| Sensor gain offsets | Q8 |
| `ISP_AWB_RESULT_S` WB gains | Q16 |
| Рабочие R/B gains | Q8 |
| Скорость AWB | Q12 |
| Цветовая температура CM | Kelvin и Kelvin Q8 |
| Температурная координата WB | mired Q8 |
| `R/G`, `B/G` metering limits | unsigned Q4.8 |
| Saturation target | `0x80` означает единичный множитель |

---

## Жизненный цикл и управление

### `AwbInit`

**Назначение.** Инициализация одного AWB instance.

**Подтверждено:**

1. Проверяет `handle <= 1` и ненулевой `ISP_AWB_PARAM_S`.
2. Требует предварительную sensor registration: `bSnsRegistered != 0`.
3. Сравнивает `SensorId` ISP с сохранённым `SensorId` AWB.
4. Вызывает `VReg_Init(base, 0x1000)`.
5. Выполняет:

```c
AwbExtRegsDefault(handle);
AwbExtRegsInitialize(handle);
ctx->enRequestedWdrMode = 0;
ctx->enAppliedWdrMode = 0;
AwbWbInit(&ctx->stWb, &ctx->stSnsDefault);
AwbCmInit(&ctx->stCm, &ctx->stSnsDefault);
ctx->bInitialized = HI_TRUE;
```

Sensor callback и defaults находятся внутри общего контекста по смещениям `0x438` и `0x3cc`.

### `AwbExit`

**Назначение.** Завершение VREG instance.

**Подтверждено:**

- Проверяет handle.
- Если контекст не инициализирован, возвращает успех.
- Вызывает `VReg_Exit(base, 0x1000)`.
- Сбрасывает `bInitialized` только при успешном `VReg_Exit`.
- Не очищает остальной контекст и sensor registration.

**Замечание.** Функция явно не освобождает активный debug mapping. Это может быть особенностью ожидаемого порядка shutdown либо утечкой оригинальной реализации.

### `AwbRun`

**Назначение.** Главный callback обработки кадра.

**Подтверждено:**

- Проверяет handle, `pstAwbInfo`, `pstAwbResult`, обе statistics pointers и `bInitialized`.
- Сохраняет `u32FrameCnt`.
- На frame `1` вызывает `AwbIspRegsInit`.
- Читает VREG через `AwbReadExtRegs`.
- При изменении WDR вызывает `AwbSetWdrMode` и устанавливает `stStatAttr.bChange`.
- Запускает debug begin/end.
- Тяжёлый WB/CCM расчёт выполняет только на чётных кадрах.
- На каждом кадре возвращает cached `ISP_AWB_RESULT_S`.

Основные вычисления:

```c
if ((frameCnt & 1) == 0) {
    AwbWbMatrixCalculate(handle, info, &ctx->stWb);
    AwbColorMatrixCalculate(handle, info, &ctx->stCm);
    /* CCM копируется в cached result */
    /* формируются 4 WB gains */
}
```

Есть два пути формирования WB gains:

- manual — четыре manual gains переводятся в Q16, при WDR могут возводиться в квадрат;
- auto — вычисленные R/G/B составляющие умножаются на sensor gain offsets.

В отдельной ветви учитывается аппаратный black level `WB_BLACK01` (`0x205A02D4`) и выполняется общая нормализация четырёх каналов.

### `AwbCtrl`

**Назначение.** Внешняя callback-обёртка управляющих команд.

```c
if (handle <= 1 && ctx->bInitialized)
    return AwbCtrlCmd(handle, cmd, value);
return HI_FAILURE;
```

Правильный третий аргумент — полиморфный `HI_VOID *`, а не `int *`.

### `AwbCtrlCmd`

**Назначение.** Диспетчер private AWB и common ISP control commands.

| Cmd | Команда | Действие |
|---:|---|---|
| 0 | `AWB_SATURATION_SET` | Запись low byte в VREG `+0x052` |
| 1 | `AWB_SATURATION_GET` | Чтение VREG `+0x052` |
| 2 | `AWB_DEBUG_ATTR_SET` | `AwbDbgSet` |
| 3 | `AWB_DEBUG_ATTR_GET` | `AwbDbgGet` |
| 8000 | `ISP_WDR_MODE_SET` | Запись requested WDR в bit 1 VREG `+0x000` |
| 8001 | `ISP_PROC_WRITE` | `AwbProcWrite` |
| 8002 | `ISP_AE_FPS_BASE_SET` | Игнорируется |
| 8003 | `ISP_AWB_ISO_SET` | Запись ISO в VREG `+0x160` |
| 8005 | undocumented | Запись `HI_U32` в VREG `+0x1ac` |

Неизвестные команды обычно возвращают успех. Смысл команды `8005` и `stWb.u32Unknown270` пока не установлен.

### `AwbSetWdrMode`

**Назначение.** Полная переинициализация sensor-dependent AWB state после изменения WDR.

Последовательность:

1. Повторно вызывает `pfn_cmos_get_awb_default`, если callback зарегистрирован.
2. Перезаписывает sensor defaults во VREG.
3. Повторно инициализирует WB и CM контексты.
4. Снова читает external registers.
5. Копирует requested WDR в applied WDR.

Это не простая смена одного регистра, а reload калибровки сенсора.

---

## VREG

### `AwbExtRegsDefault`

**Назначение.** Заполнение AWB VREG безопасными fallback defaults.

Ключевые defaults:

```text
AWB enable                 = 1
manual mode                = 0
WDR                         = 0
R response strength        = 128
B response strength        = 32
global sum threshold       = 4096
speed Q12                  = 128 = 1/32
reference/current CCT      = 5000 K
gain offsets/manual gains  = 0x100
ISO                         = 100
saturation target          = 0x80
```

Fallback metering/clip параметры и reference-zone настройки также заполняются. Четыре 8-байтных rule records получают `(256,256,256,0,8)`.

Fallback CCM одинакова для high/mid/low:

```text
 464  -336   128
 -80   416   -80
 -48  -192   496
```

Она signed Q8 и имеет сумму каждой строки `256`.

Fallback saturation table:

```c
{35, 44, 52, 62, 70, 78, 84, 84}
```

### `AwbExtRegsInitialize`

**Назначение.** Запись sensor calibration в уже созданный VREG block.

Записывает:

- `u16WbRefTemp`;
- четыре sensor gain offsets;
- шесть `WbPara`;
- три температуры CCM и три матрицы;
- sensor AGC saturation table, если `bValid`.

High CCM дополнительно записывается как начальная текущая CCM. Public direct CCM переводится в аппаратный complement вызовом `AwbDirectToComplement`.

### `AwbReadExtRegs`

**Назначение.** Синхронизация VREG settings с `AWB_WB_CTX_S` и `AWB_CM_CTX_S`.

Это один из главных источников восстановленной раскладки контекста. Функция читает:

- enable/manual/WDR;
- gain strengths, thresholds, speed;
- reference temperature, gain offsets и WbPara;
- status/algorithm flags;
- ISO и runtime coefficients;
- CCM calibration и temperatures;
- saturation table;
- debug buffer configuration;
- четыре rule records и дополнительные thresholds.

При изменении отслеживаемого параметра устанавливает `bConfigChanged`. Некорректные CCM temperatures автоматически раздвигаются с минимальным интервалом около 500 K.

Сигнатура второго аргумента ещё требует проверки на уровне ARM call sites: декомпилятор показывает параметр, управляющий сбросом `bConfigChanged`, но часть вызовов выглядит одноаргументной.

### `AwbUpdateExtRegs`

**Назначение.** Экспорт текущего состояния обратно во VREG.

Подтверждённые outputs:

- девять текущих CCM coefficients в VREG `+0x05c..+0x07c`;
- текущая color temperature в `+0x012`;
- один status bit в `+0x1a6`.

Функция не программирует физические ISP-регистры — только виртуальную область AWB.

---

## Инициализация WB

### `AwbWbInit`

Композиция трёх операций:

```c
AwbWbMatrixInit(ctx);
AwbWbGainSet(ctx, &sensorDefault->au16GainOffset[0]);
AwbWbParaSet(ctx, &sensorDefault->as32WbPara[0]);
```

### `AwbWbGainSet`

Копирует четыре Q8 sensor gain offsets в WB context. Порядок: R, Gr, Gb, B.

### `AwbWbParaSet`

Копирует шесть signed calibration parameters:

```text
p2, p1, q1, a1, b1, c1
```

Если `a1 == 0`, сохраняет `1` для защиты последующих делений. Назначение `b1` в этой версии остаётся неясным: `AwbToMired` и `AwbToRgBg` его не используют.

### `AwbWbMatrixInit`

**Назначение.** Инициализация рабочего WB state и reference-zone cache.

Подтверждено:

- рабочие gains получают Q8 unity `0x100`;
- filtered R/B gains получают Q16 unity `0x10000`;
- reference/unnormalized gains получают `0x100`;
- 20 reference zone indices получают специальные пространственные индексы;
- 20 reference zone classes получают значения `0..4`;
- 20 cached zone records (`20 × 0x14`) обнуляются;
- часть counters/filters сбрасывается;
- одно поле инициализируется `17280` (`0x4380`).

Reference zone indices:

```text
0,1,15,16,17,18,32,33,127,128,
144,145,221,222,236,237,238,239,253,254
```

Это контрольные пространственные зоны сетки 15×17, используемые для scene-change detection.

### `AwbWbMatrixNormalize`

**Назначение.** Временная фильтрация и нормализация автоматических R/G/B gains.

Подтверждено:

- Обновление выполняется только при достаточном `u32GlobalSum`.
- R/B targets фильтруются с коэффициентом `u16Speed` Q12.
- Хранятся filtered R/B в Q16.
- Минимум из R, unity и B нормализуется к unity.
- Три выходных Q8 gain компонента ограничиваются `0x1000`.

Функцию полезно повторно декомпилировать с последней версией `awb_ctx_ida.h`: часть полей области `0x052..0x067` всё ещё не получила семантических имён.

---

## Основной WB-алгоритм

### `AwbWbMatrixCalculate`

**Назначение.** Ядро автоматического баланса белого.

Функция использует global statistics и 255 зон (`15×17`). Основные стадии:

1. Корректирует global `R/G`, `B/G` с учётом текущих gains и sensor offsets.
2. Для WDR применяет квадратичное преобразование отношений.
3. Проверяет минимальный global population.
4. Сравнивает текущую global point с предыдущей и ведёт stable-frame counter.
5. Читает 255 zonal `Rg/Bg/Sum` records; зоны с `Sum <= 64` отвергаются.
6. Для валидных зон вычисляет mired/Planck offset и несколько confidence/penalty coefficients.
7. Использует 20 reference zones и их cache для обнаружения смены сцены.
8. Формирует четыре light candidates размером `0x24`.
9. Сортирует их по убыванию веса через `qsort(..., AwbCompareWeight)`.
10. Выбирает лучший candidate либо агрегирует несколько, в зависимости от flags.
11. Применяет раздельную силу R/B correction.
12. Вызывает `AwbToMired`, `AwbCoefClip`, сохраняет unnormalized gains и вызывает `AwbWbMatrixNormalize`.

Подтверждённые внутренние массивы:

```c
AWB_ZONE_WORK_S zones[255];          /* 0x14 каждый */
AWB_LIGHT_CANDIDATE_S candidate[4];  /* 0x24 каждый */
```

Полная структура light candidate и точное значение части zone fields пока не восстановлены. Это главное слепое место алгоритма.

### `AwbGlobalRelateCoef`

**Назначение.** Нормированная мера различия двух точек в `R/G–B/G` пространстве.

Вычисляет точку на reference ray:

```text
projectedBg = rg * referenceBg / referenceRg
```

Затем:

```text
error = (|referenceRg-rg| + |projectedBg-bg|) * 256
        / sqrt(referenceRg² + referenceBg²)
```

Результат насыщается до 255. `0` означает совпадение, большое значение — различие. Несмотря на имя `RelateCoef`, это скорее error/dissimilarity.

### `AwbCompareWeight`

Comparator для `qsort`. Сравнивает `HI_U32` по offset `+0x0c` и сортирует записи по убыванию веса:

```c
if (left->weight > right->weight) return -1;
if (left->weight < right->weight) return 1;
return 0;
```

Не вызывается прямым `BL`, потому что передаётся как function pointer.

### `AwbCoefClip`

**Назначение.** Ограничение оценённой белой точки по CCT и Planck offset с плавной коррекцией gains.

Использует внутреннюю пару:

```text
mired Q8 + signed Planck offset
```

Algorithm flags:

| Bit | Подтверждённое действие |
|---:|---|
| 0 | CCT clipping |
| 1 | Использовать настроенные target clip gains |
| 2 | Planck-offset clipping |

CCT limits defaults соответствуют примерно 2500–10000 K. При выходе за диапазон R/B gains плавно смешиваются с configured либо вычисленными boundary gains. Planck-offset correction смешивается ещё мягче, с ограниченным Q4-весом.

### `AwbToMired`

**Назначение.** Преобразование измеренной точки `(R/G,B/G)` в `(mired Q8, signed Planck offset)`.

1. Вычисляет наклон луча `k = Bg/Rg` в Q8.
2. Аналитически находит пересечение этого луча с откалиброванной Planckian curve.
3. Преобразует координату пересечения в mired Q8 через `a1/c1`.
4. Вычисляет радиальное расстояние между наблюдаемой точкой и точкой кривой.
5. Назначает знак offset по относительному положению `Rg`.

Это не классическое перпендикулярное `Duv`, а знаковое расстояние вдоль лучевой геометрии алгоритма.

### `AwbToRgBg`

**Назначение.** Обратное преобразование `(mired Q8, Planck offset) → (R/G,B/G)`.

Планковская модель имеет рациональный вид:

```text
x = (miredQ8 - c1) * 256 / a1

y = (p1*65536 + p2*x²) / (q1 + x²/256)
```

Затем точка сдвигается на заданный signed offset. Функция вместе с `AwbToMired` образует прямое/обратное преобразование координат AWB.

### `AwbSqrt32`

Целочисленный квадратный корень с округлением к ближайшему целому, не floor:

```c
n = isqrt_floor(x);
return x > n*(n+1) ? n+1 : n;
```

Выходной диапазон для `HI_U32`: `0..65536`. Для битовой совместимости нельзя бездумно заменять на `(HI_U32)sqrt(x)`.

---

## CCM и насыщенность

### `AwbCmInit`

Инициализирует `as16InterpolatedCCM` из sensor High CCM, переводя public direct coefficients во внутренний complement. Начальная температура:

```text
u16ColorTemp   = 6500 K
u32ColorTempQ8 = 6500 << 8
```

Region state намеренно инициализируется несимметрично: previous/current state требует дополнительной сверки с `AwbCcMatrixCalculate`.

Функция не заполняет явно `au16OutputCCM`, что создаёт вопрос о CCM первого кадра.

### `AwbCcMatrixCalculate`

**Назначение.** Выбор и интерполяция sensor CCM по текущей color temperature.

Подтверждено ранее по декомпиляции:

- Использует три calibration matrices: low, mid, high.
- Имеет пятисостоянийный hysteresis machine:

```text
LOW → LOW_MID → MID → MID_HIGH → HIGH
```

- Интерполяция выполняется в signed Q8, вес — Q11 (`2048` unity).
- Сглаживает CCT Q8: при малом скачке коэффициент около `1/32`, при большом — около `1/16`.
- Обновляет interpolated CCM только при достаточной global population (`> 0x4000` в изученной ветви).

**Недостаток материала.** Полная декомпиляция этой функции не была повторно приведена в текущем документе; для окончательных source-like имён region fields желательно сохранить её отдельно.

### `AwbColorMatrixCalculate`

**Назначение.** Построение финальной CCM с учётом saturation и extreme-CCT correction.

Стадии:

1. `AwbSaturationCalculate` получает текущую saturation по ISO.
2. Строится стандартная RGB saturation matrix с luma weights `76,150,29`.
3. Выполняется `saturationMatrix × interpolatedCCM`.
4. При CCT вне примерно 2500–8000 K CCM плавно смешивается с identity.
5. Output CCM переводится из complement в public direct representation.
6. Обновляется CCT state и вызывается `AwbCcMatrixCalculate` для следующего состояния.

При saturation `0x80` saturation matrix является identity; при `0` изображение переводится к luma weights.

### `AwbSaturationCalculate`

**Назначение.** Выбор saturation по ISO.

Manual mode:

```c
output = u8SatTarget;
```

Auto mode интерполирует `au8SaturationTable[8]` по ISO nodes:

```text
100, 200, 400, 800, 1600, 3200, 6400, 12800
```

После интерполяции:

```text
output = tableValue * u8SatTarget / 128
```

с округлением и насыщением до 255.

### `AwbSaturationOffsetCalculate`

Вычисляет округлённый абсолютный interpolation offset между соседними saturation nodes:

```text
offset = round(
    (currentIso-lowerIso) * |next-prev|
    / (upperIso-lowerIso))
```

Caller отдельно прибавляет или вычитает offset в зависимости от направления таблицы.

---

## Матричные helpers

### `AwbMatrixIdentity`

Заполняет квадратную row-major signed Q8 матрицу:

```c
matrix[row*N + col] = row == col ? 0x100 : 0;
```

### `AwbMatrixCopy`

Копирует `rows * columns` 16-битных элементов. Порядок аргументов: source, destination, rows, columns.

### `AwbMatrixMultiply`

Умножает signed Q8 матрицы через 32-битный аккумулятор:

```text
Cij = (Σ Aik*Bkj + 128) >> 8
```

Затем для каждой строки вычисляет сумму и добавляет `256-rowSum` в диагональный элемент. Это гарантирует сумму строки `0x100` и сохранение neutral gray axis. Явного saturation результата к `S16` нет.

### `AwbDirectToComplement`

**Назначение.** Перевод публичного CCM coefficient из direct/sign-magnitude в signed two's-complement bit pattern для внутренних вычислений или аппаратного регистра.

Подтверждено по использованию, но точная декомпиляция функции в текущем наборе не приведена. Для положительных значений результат неизменен; для значений с sign bit `0x8000` magnitude должен стать отрицательным complement.

**Нужно декомпилировать**, если требуется битовая совместимость для corner cases: negative zero, максимальная magnitude и маска допустимых битов.

### `AwbComplementToDirect`

Обратное преобразование internal/apparatus two's complement → public direct/sign-magnitude. Используется в `HI_MPI_ISP_GetCCM` и перед возвратом output CCM.

**Нужно декомпилировать** вместе с `AwbDirectToComplement` для точных corner cases.

---

## Начальный ISP result

### `AwbIspRegsInit`

Несмотря на имя, функция не пишет физические ISP-регистры. Она формирует initial cached `ISP_AWB_RESULT_S` на frame 1:

- sensor gain offsets Q8 переводятся в result gains Q16;
- текущая output CCM копируется в result;
- metering defaults:

```text
WhiteLevel = 940
BlackLevel = 64
CrRefMin/Max = 128/512 = 0.5/2.0
CbRefMin/Max = 128/512 = 0.5/2.0
bChange = 1
```

Физическую запись, вероятно, выполняет ISP framework после callback.

---

## Debug и PROC

### `AwbDbgSet`

Использует фактический 12-байтный payload:

```c
typedef struct {
    HI_BOOL bEnable;
    HI_U32 u32PhyAddr;
    HI_U32 u32Depth;
} AWB_DEBUG_INFO_S;
```

При enable проверяет ненулевые physical address и depth. Вычисляет:

```text
mapSize = 88 + depth * 7188
```

и записывает configuration во VREG. При disable сбрасывает только enable.

Это ABI не совпадает с большой публичной `AWB_DBG_ATTR_S` из приложенного заголовка, что указывает на различие ревизий SDK.

### `AwbDbgGet`

Возвращает из VREG три поля фактического `AWB_DEBUG_INFO_S`: enable, physical address, depth. Calculated map size не возвращается.

### `AwbDbgRunBgn`

**Назначение.** Управление mapping debug ring buffer и начало записи кадра.

- Сравнивает текущую и предыдущую debug configuration.
- При изменении освобождает старый mapping.
- Делает `HI_MPI_SYS_Mmap(phyAddr, mapSize)`.
- Заполняет 88-байтный global header.
- Устанавливает `pstStatus = mappedBase + 88`.
- Выбирает record `frameCnt % depth` и пишет `u32FrmNumBgn`.

Global header содержит AWB/VREG configuration и часть аппаратных metering registers, включая white/black levels и `Cr/Cb` limits.

### `AwbDbgRunEnd`

Завершает текущую кадровую запись:

- global sum, Rg, Bg;
- color temperature Kelvin;
- result R/G/B gains в Q8;
- девять CCM coefficients;
- `u32FrmNumEnd`.

Пара `FrmNumBgn/FrmNumEnd` позволяет читателю обнаружить частично записанный record без mutex.

Zonal debug entries заполняются внутри `AwbWbMatrixCalculate`.

### `AwbProcWrite`

Печатает краткое состояние AWB в supplied PROC buffer:

- четыре WB gains;
- color temperature;
- текущую CCM;
- manual enable;
- manual saturation enable;
- current/target saturation;
- поле с label `Zones`;
- AWB speed.

Функция помогла подтвердить several context fields, но label `Zones` не совпадает с обнаруженным математическим использованием поля как Bgain response strength.

---

## Оставшиеся неизвестности и необходимые декомпиляции

Все функции из исходного списка представлены выше. Для завершения именно битовой, а не архитектурной реконструкции не хватает:

1. Полной декомпиляции `AwbDirectToComplement`.
2. Полной декомпиляции `AwbComplementToDirect`.
3. Повторной декомпиляции `AwbWbMatrixNormalize` с текущими типами.
4. Полной source-like фиксации `AwbCcMatrixCalculate`.
5. Разметки `AWB_LIGHT_CANDIDATE_S` внутри `AwbWbMatrixCalculate`.
6. Назначения flags bits и четырёх `AWB_WB_RULE_S`.
7. Caller команды `8005` в ISP framework.
8. Проверки применения `ISP_AWB_RESULT_S` на первом кадре, особенно output CCM.

Следующий логичный этап — ISP/framework: найти формирование `ISP_AWB_INFO_S`, непрямой вызов `pfn_awb_run` и применение `ISP_AWB_RESULT_S` к физическим WB/CCM/metering registers.
