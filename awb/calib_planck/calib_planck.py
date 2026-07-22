#!/usr/bin/env python3
"""
Подбор коэффициентов HiSilicon AWB по измерениям нейтральной мишени.

Формат входного файла:

    4000 Rg=0x0ac Bg=0x162
    5000 Rg=0x100 Bg=0x100
    ?    Rg=0x0e7 Bg=0x13a

Также разрешены варианты ``T=4000K: ...`` и десятичные значения ratios.
Пустые строки и текст после символа ``#`` игнорируются.

Важно:

* Rg и Bg во входном файле — аппаратная статистика G/R и G/B в Q8.
* Во время измерений WB gains должны быть зафиксированы на sensor default
  GainOffset. Тогда дополнительная компенсация applied gains не нужна.
* Строка с неизвестной/сомнительной температурой обозначается ``?``. Такая
  точка участвует в подборе геометрии Planckian locus (p1/p2/q1), но не
  участвует в переводе координаты locus в Kelvin (a1/c1).

"""

import argparse
import math
import re
import sys


LINE_RE = re.compile(
    r"""
    ^\s*
    (?:T\s*=\s*)?
    (?P<temperature>\?+|unknown|\d+)
    \s*(?:K)?\s*:?[\s,]+
    Rg\s*=\s*(?P<rg>0[xX][0-9a-fA-F]+|\d+)
    [\s,]+
    Bg\s*=\s*(?P<bg>0[xX][0-9a-fA-F]+|\d+)
    \s*$
    """,
    re.IGNORECASE | re.VERBOSE,
)


def round_away_from_zero(value):
    """Округление к ближайшему целому без банковского правила round()."""

    if value >= 0.0:
        return int(math.floor(value + 0.5))
    return int(math.ceil(value - 0.5))


def c_div(numerator, denominator):
    """Целочисленное деление со знаком, как в C: округление к нулю."""

    if denominator == 0:
        raise ZeroDivisionError("division by zero")

    quotient = abs(numerator) // abs(denominator)
    if (numerator < 0) != (denominator < 0):
        return -quotient
    return quotient


def awb_sqrt32(value):
    """
    Целочисленный sqrt с тем же округлением, что AwbSqrt32().

    Оригинальная функция выбирает между floor(sqrt(x)) и следующим целым
    по границе root * (root + 1).
    """

    if value < 0:
        raise ValueError("sqrt of a negative value")

    root = math.isqrt(value)
    if value > root * (root + 1):
        root += 1
    return root


def parse_value(text):
    """Читает как 0xABC, так и обычное десятичное число."""

    return int(text, 0)


def parse_file(path):
    """Читает измерения и возвращает список словарей."""

    samples = []

    with open(path, "r", encoding="utf-8") as source:
        for line_number, original_line in enumerate(source, 1):
            line = original_line.split("#", 1)[0].strip()
            if not line:
                continue

            match = LINE_RE.match(line)
            if match is None:
                raise ValueError(
                    "{}:{}: неверный формат строки: {!r}".format(
                        path, line_number, original_line.rstrip("\n")
                    )
                )

            temperature_text = match.group("temperature").lower()
            if temperature_text.startswith("?") or temperature_text == "unknown":
                temperature = None
            else:
                temperature = int(temperature_text, 10)
                if temperature <= 0:
                    raise ValueError(
                        "{}:{}: температура должна быть положительной".format(
                            path, line_number
                        )
                    )

            stat_rg = parse_value(match.group("rg"))
            stat_bg = parse_value(match.group("bg"))

            if not (1 <= stat_rg <= 0xFFFF):
                raise ValueError(
                    "{}:{}: Rg вне диапазона 1..0xffff".format(
                        path, line_number
                    )
                )
            if not (1 <= stat_bg <= 0xFFFF):
                raise ValueError(
                    "{}:{}: Bg вне диапазона 1..0xffff".format(
                        path, line_number
                    )
                )

            # Оригинальная библиотека использует именно 0xffff / ratio.
            curve_rg = 0xFFFF // stat_rg
            curve_bg = 0xFFFF // stat_bg

            samples.append(
                {
                    "line": line_number,
                    "temperature": temperature,
                    "stat_rg": stat_rg,
                    "stat_bg": stat_bg,
                    "curve_rg": curve_rg,
                    "curve_bg": curve_bg,
                }
            )

    if len(samples) < 3:
        raise ValueError("для p1/p2/q1 нужны минимум три измерения")

    return samples


def solve_square_system(matrix, vector):
    """Решает небольшую квадратную систему методом Гаусса с pivoting."""

    size = len(vector)
    augmented = [
        [float(value) for value in matrix[row]] + [float(vector[row])]
        for row in range(size)
    ]

    for column in range(size):
        pivot = max(range(column, size), key=lambda row: abs(augmented[row][column]))
        if abs(augmented[pivot][column]) < 1.0e-12:
            raise ValueError("система вырождена: нужны более разные источники света")

        augmented[column], augmented[pivot] = augmented[pivot], augmented[column]

        pivot_value = augmented[column][column]
        for item in range(column, size + 1):
            augmented[column][item] /= pivot_value

        for row in range(size):
            if row == column:
                continue

            factor = augmented[row][column]
            for item in range(column, size + 1):
                augmented[row][item] -= factor * augmented[column][item]

    return [augmented[row][size] for row in range(size)]


def least_squares(rows, values):
    """
    Решает переопределённую систему методом наименьших квадратов.

    Для маленькой задачи 3x3 нормальные уравнения достаточно удобны и
    позволяют не требовать NumPy.
    """

    if not rows:
        raise ValueError("пустая система")

    columns = len(rows[0])
    normal_matrix = [[0.0 for _ in range(columns)] for _ in range(columns)]
    normal_vector = [0.0 for _ in range(columns)]

    for row, value in zip(rows, values):
        for i in range(columns):
            normal_vector[i] += row[i] * value
            for j in range(columns):
                normal_matrix[i][j] += row[i] * row[j]

    return solve_square_system(normal_matrix, normal_vector)


def fit_planck_curve(samples):
    r"""
    Подбирает p1, p2, q1 из уравнения:

        B = (p2 * R + 256 * p1) / (R + q1)

    После переноса членов каждая точка даёт линейное уравнение:

        256*p1 + R*p2 - B*q1 = R*B

    Порядок неизвестных в системе: [p1, p2, q1].
    """

    rows = []
    values = []

    for sample in samples:
        curve_rg = sample["curve_rg"]
        curve_bg = sample["curve_bg"]
        rows.append([256.0, float(curve_rg), float(-curve_bg)])
        values.append(float(curve_rg * curve_bg))

    p1_float, p2_float, q1_float = least_squares(rows, values)

    return {
        "p1_float": p1_float,
        "p2_float": p2_float,
        "q1_float": q1_float,
        "p1": round_away_from_zero(p1_float),
        "p2": round_away_from_zero(p2_float),
        "q1": round_away_from_zero(q1_float),
    }


def project_to_planck_locus(curve_rg, curve_bg, p2, p1, q1):
    """
    Повторяет геометрическую часть AwbToMired().

    Возвращает R/G и B/G точки на fitted locus, а также signed расстояние
    от измеренной точки. a1/b1/c1 здесь не участвуют.
    """

    reference_rg = curve_rg if curve_rg != 0 else 1
    if curve_rg == 0:
        curve_rg = 1
        half_rg = 1
    else:
        half_rg = curve_rg >> 1

    slope_q8 = (half_rg + (curve_bg << 8)) // curve_rg
    if slope_q8 == 0:
        slope_q8 = 1
        twice_slope_q8 = 2
        curve_term = 0
    else:
        twice_slope_q8 = 2 * slope_q8
        slope_square_q8 = (slope_q8 * slope_q8 + 128) >> 8
        inner = (q1 * slope_square_q8 + 128) >> 8
        curve_term = q1 * inner

    radicand = (
        curve_term
        + p2 * p2
        + slope_q8 * 4 * p1
        - q1 * ((p2 * twice_slope_q8 + 128) >> 8)
    )

    # Для корректной калибровки radicand неотрицателен. Clamp совпадает с
    # нашей безопасной реконструкцией AwbToMired().
    if radicand < 0:
        radicand = 0

    locus_rg = (
        c_div(p2 * 256, twice_slope_q8)
        - c_div(q1, 2)
        + c_div(awb_sqrt32(radicand) * 128, slope_q8)
    )
    if locus_rg < 0:
        locus_rg = 0

    locus_bg = (locus_rg * slope_q8 + 128) >> 8

    delta_rg = locus_rg - reference_rg
    delta_bg = locus_bg - curve_bg
    distance = awb_sqrt32(delta_rg * delta_rg + delta_bg * delta_bg)
    if locus_rg > reference_rg:
        distance = -distance

    return locus_rg, locus_bg, distance


def runtime_mired_q8(a1, c1, sqrt_locus_q8):
    """Повторяет целочисленную формулу температуры AwbToMired()."""

    value = c1 + c_div(a1 * sqrt_locus_q8 + 128, 256)
    return abs(value)


def fit_temperature_curve(samples, planck):
    r"""
    Подбирает a1 и c1 по известным температурам.

        miredQ8 = c1 + a1 * sqrt(R_locus / 256)

    В оригинальном fixed-point коде это записано как:

        miredQ8 = c1 + a1 * sqrt(R_locus << 8) / 256

    b1 равен 128 и соответствует фиксированной степени 1/2.
    """

    rows = []
    values = []
    temperature_samples = []

    for sample in samples:
        if sample["temperature"] is None:
            continue

        locus_rg, locus_bg, offset = project_to_planck_locus(
            sample["curve_rg"],
            sample["curve_bg"],
            planck["p2"],
            planck["p1"],
            planck["q1"],
        )
        sqrt_locus_q8 = awb_sqrt32(locus_rg << 8)
        target_mired_q8 = 256000000.0 / sample["temperature"]

        rows.append([sqrt_locus_q8 / 256.0, 1.0])
        values.append(target_mired_q8)
        temperature_samples.append(
            {
                "sample": sample,
                "locus_rg": locus_rg,
                "locus_bg": locus_bg,
                "offset": offset,
                "sqrt_locus_q8": sqrt_locus_q8,
                "target_mired_q8": target_mired_q8,
            }
        )

    if len(temperature_samples) < 2:
        return None

    a1_float, c1_float = least_squares(rows, values)
    a1_initial = round_away_from_zero(a1_float)
    c1_initial = round_away_from_zero(c1_float)

    # Небольшой поиск рядом с округлённым решением компенсирует целочисленное
    # округление внутри настоящего AwbToMired().
    best = None
    for a1 in range(a1_initial - 8, a1_initial + 9):
        for c1 in range(c1_initial - 8, c1_initial + 9):
            error = 0.0
            for item in temperature_samples:
                predicted = runtime_mired_q8(
                    a1, c1, item["sqrt_locus_q8"]
                )
                delta = predicted - item["target_mired_q8"]
                error += delta * delta

            # Несколько соседних целочисленных пар могут давать одинаковый
            # результат. В этом случае оставляем ближайшую к вещественному
            # решению МНК, а не первую попавшуюся пару из диапазона поиска.
            distance_from_float = (
                abs(a1 - a1_float) + abs(c1 - c1_float)
            )
            candidate = (error, distance_from_float, a1, c1)
            if best is None or candidate < best:
                best = candidate

    return {
        "a1_float": a1_float,
        "c1_float": c1_float,
        "a1": best[2],
        "b1": 128,
        "c1": best[3],
        "samples": temperature_samples,
    }


def estimated_temperature(mired_q8):
    """Переводит miredQ8 обратно в Kelvin с округлением."""

    if mired_q8 <= 0:
        return None
    return (256000000 + mired_q8 // 2) // mired_q8


def print_report(samples, planck, temperature_curve):
    """Печатает параметры и диагностическую таблицу."""

    print("Преобразованные точки (вход AwbToMired):")
    print(" line    T[K]   stat G/R stat G/B   curve R/G curve B/G")
    for sample in samples:
        temperature_text = (
            str(sample["temperature"])
            if sample["temperature"] is not None
            else "?"
        )
        print(
            " {:4d} {:>7s}     0x{:03x}    0x{:03x}       0x{:03x}     0x{:03x}".format(
                sample["line"],
                temperature_text,
                sample["stat_rg"],
                sample["stat_bg"],
                sample["curve_rg"],
                sample["curve_bg"],
            )
        )

    print("\nPlanckian locus, решение МНК:")
    print("  p1 = {: .6f} -> {}".format(planck["p1_float"], planck["p1"]))
    print("  p2 = {: .6f} -> {}".format(planck["p2_float"], planck["p2"]))
    print("  q1 = {: .6f} -> {}".format(planck["q1_float"], planck["q1"]))

    if temperature_curve is None:
        print("\nДля a1/c1 нужны минимум две строки с известной температурой.")
    else:
        print("\nТемпературная зависимость, решение МНК:")
        print(
            "  a1 = {: .6f} -> {}".format(
                temperature_curve["a1_float"], temperature_curve["a1"]
            )
        )
        print("  b1 = 128 (фиксировано библиотекой)")
        print(
            "  c1 = {: .6f} -> {}".format(
                temperature_curve["c1_float"], temperature_curve["c1"]
            )
        )

    print("\nДиагностика fitted locus:")
    print(" line    T input  locus R/G locus B/G  offset    CCT fitted")

    for sample in samples:
        locus_rg, locus_bg, offset = project_to_planck_locus(
            sample["curve_rg"],
            sample["curve_bg"],
            planck["p2"],
            planck["p1"],
            planck["q1"],
        )

        input_temperature = (
            str(sample["temperature"])
            if sample["temperature"] is not None
            else "?"
        )

        if temperature_curve is None:
            fitted_temperature = "-"
        else:
            sqrt_locus_q8 = awb_sqrt32(locus_rg << 8)
            mired_q8 = runtime_mired_q8(
                temperature_curve["a1"],
                temperature_curve["c1"],
                sqrt_locus_q8,
            )
            fitted_temperature = str(estimated_temperature(mired_q8))

        print(
            " {:4d} {:>10s}      0x{:03x}     0x{:03x}  {:+6d} {:>13s}".format(
                sample["line"],
                input_temperature,
                locus_rg,
                locus_bg,
                offset,
                fitted_temperature,
            )
        )

    print("\nЗначения для AWB_SENSOR_DEFAULT_S:")
    print("  as32WbPara[0] = {:7d}; /* p2 */".format(planck["p2"]))
    print("  as32WbPara[1] = {:7d}; /* p1 */".format(planck["p1"]))
    print("  as32WbPara[2] = {:7d}; /* q1 */".format(planck["q1"]))

    if temperature_curve is not None:
        print(
            "  as32WbPara[3] = {:7d}; /* a1 */".format(
                temperature_curve["a1"]
            )
        )
        print("  as32WbPara[4] =     128; /* b1 */")
        print(
            "  as32WbPara[5] = {:7d}; /* c1 */".format(
                temperature_curve["c1"]
            )
        )


def main():
    parser = argparse.ArgumentParser(
        description=(
            "Подбор p1/p2/q1 и, при наличии известных температур, "
            "a1/b1/c1 для HiSilicon AWB"
        )
    )
    parser.add_argument("data_file", help="текстовый файл с измерениями")
    arguments = parser.parse_args()

    try:
        samples = parse_file(arguments.data_file)
        planck = fit_planck_curve(samples)
        temperature_curve = fit_temperature_curve(samples, planck)
        print_report(samples, planck, temperature_curve)
    except (OSError, ValueError, ZeroDivisionError) as error:
        print("Ошибка: {}".format(error), file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
