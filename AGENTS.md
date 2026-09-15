# AGENTS.md — CVQKD Simulator

## 📌 Project Overview

**CVQKD Simulator** (Continuous Variable Quantum Key Distribution Simulator) — это C++17 симулятор протоколов квантового распределения ключей с непрерывными и дискретными переменными. Проект моделирует физические процессы передачи квантовых состояний, оценку параметров канала и процедуры постобработки ключа.

**Основная цель**: Исследование и симуляция CV-QKD протоколов (включая GG02 и родственные) с использованием гауссовых состояний, матриц ковариации и методов квантовой информации. А также сравнение с протоколами DV-QKD

---

## 🏗️ Architecture

### Структура проекта

```
cvqkd_sim/
├── main.cpp                    # Точка входа симулятора
── CMakeLists.txt              # Главный файл сборки CMake
├── src/                        # Исходники библиотеки cvqkd_lib
│   └── *.cpp
├── include/                    # Публичные заголовки библиотеки
│   └── *.hpp
├── tests/                      # Unit-тесты (GoogleTest)
│   └── *.cpp
├── extern/
│   ├── eigen/                  # Eigen3 (header-only, линейная алгебра)
│   └── nlohmann/               # nlohmann/json (header-only)
└── build/                      # Директория сборки (игнорируется git)
```

### Цели сборки (CMake targets)

| Target | Тип | Описание |
|--------|-----|----------|
| `cvqkd_lib` | STATIC library | Основная библиотека симулятора |
| `cvqkd_sim` | Executable | Исполняемый файл симулятора |
| `cvqkd_tests` | Executable | Набор unit-тестов |
| `gtest`, `gmock` | Libraries | GoogleTest/GoogleMock (FetchContent) |

### Зависимости

- **Eigen3** — линейная алгебра: матрицы, векторы, разложения (SVD, eigendecomposition), тензоры
- **nlohmann/json** — сериализация/десериализация JSON (конфиги, результаты)
- **GoogleTest 1.14.0** — фреймворк тестирования (подтягивается через FetchContent)

---

## 🔬 Domain Context: CV-QKD

### Ключевые концепции предметной области

При работе с кодом важно понимать физику CV-QKD:

1. **Гауссовы состояния** — описываются матрицей ковариации (covariance matrix) и вектором средних (displacement vector)
2. **Матрица ковариации** — симметричная положительно определённая матрица 2N×2N для N мод
3. **Квантовые каналы** — моделируются как гауссовы каналы с параметрами:
   - Transmittance (T) — коэффициент пропускания
   - Excess noise (ξ) — избыточный шум
4. **Протокол GG02** — Gaussian-modulated coherent-state protocol
5. **Parameter estimation** — оценка параметров канала по открытым данным
6. **Secret key rate** — скорость генерации секретного ключа (формула Холво)
7. **Reconciliation** — согласование ключа (прямое/обратное)
8. **Privacy amplification** — усиление приватности

### Математический аппарат

- **Симплектическая геометрия** — симплектическая форма Ω, симплектические собственные значения
- **Квантовая энтропия фон Неймана** — вычисляется через симплектические собственные значения
- **Holevo bound** — граница информации, доступной еavesdropper'у
- **Mutual information** — взаимная информация между Алисой и Бобом

---

## 💻 Coding Standards

### Общие правила

- **Стандарт**: C++17 (строго, без расширений компилятора)
- **Язык кода**: английский (имена переменных, функции, комментарии)
- **Стиль**: современный C++, RAII, умные указатели, range-based for
- **Исключения**: включены (`/EHsc` на MSVC)

### Именование

```cpp
// Классы — PascalCase
class CovarianceMatrix { ... };
class QuantumChannel { ... };

// Функции/методы — camelCase
double computeSecretKeyRate();
void applyChannelLoss(double transmittance);

// Переменные — camelCase
double transmittance;
Eigen::MatrixXd covarianceMatrix;

// Константы — UPPER_SNAKE_CASE
constexpr double PLANCK_CONSTANT = 1.054571817e-34;
constexpr double SHOT_NOISE_VARIANCE = 1.0;

// Приватные поля — m_ prefix или trailing _
class QuantumState {
    Eigen::MatrixXd m_covarianceMatrix;  // или covarianceMatrix_
    double m_excessNoise;
};
```

### Работа с Eigen

```cpp
// Всегда используй typedef/using для типов Eigen
using MatrixXd = Eigen::MatrixXd;
using VectorXd = Eigen::VectorXd;

// Для фиксированных размеров — используй фиксированные типы
using Matrix2d = Eigen::Matrix2d;  // 2x2
using Vector4d = Eigen::Vector4d;  // 4x1

// Передача в функции — по const reference
void processState(const Eigen::MatrixXd& covarianceMatrix);

// Возврат больших матриц — по значению (NRVO работает)
Eigen::MatrixXd computeCovarianceMatrix(...);

// Симплектическая форма Ω (стандартное обозначение)
// Ω = (i=1 to N) [0 1; -1 0]
Eigen::MatrixXd symplecticForm(int nModes) {
    Eigen::MatrixXd omega = Eigen::MatrixXd::Zero(2*nModes, 2*nModes);
    for (int i = 0; i < nModes; ++i) {
        omega(2*i, 2*i+1) = 1.0;
        omega(2*i+1, 2*i) = -1.0;
    }
    return omega;
}
```

### Работа с JSON

```cpp
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// Сериализация
json config;
config["transmittance"] = 0.5;
config["excessNoise"] = 0.01;
config["modulationVariance"] = 10.0;

// Десериализация
double T = config["transmittance"].get<double>();

// Сохранение в файл
std::ofstream file("config.json");
file << config.dump(4);  // 4 — отступ

// Загрузка из файла
std::ifstream file("config.json");
json config = json::parse(file);
```

### Обработка ошибок

```cpp
// Используй исключения для критических ошибок
class InvalidCovarianceMatrixException : public std::runtime_error {
    // ...
};

// Проверка положительной определённости матрицы ковариации
void validateCovarianceMatrix(const Eigen::MatrixXd& gamma) {
    // Физическое условие: γ + iΩ ≥ 0 (квантовое состояние)
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> solver(gamma);
    if (solver.eigenvalues().minCoeff() < 0) {
        throw InvalidCovarianceMatrixException(
            "Covariance matrix must be positive definite");
    }
}
```

---

## 🧪 Testing Guidelines

### Фреймворк: GoogleTest 1.14.0

```cpp
#include <gtest/gtest.h>

// Простой тест
TEST(CovarianceMatrixTest, IsPositiveDefinite) {
    Eigen::MatrixXd gamma = Eigen::MatrixXd::Identity(4, 4);
    EXPECT_TRUE(isPositiveDefinite(gamma));
}

// Тест с fixture (для переиспользования setup)
class QuantumChannelTest : public ::testing::Test {
protected:
    void SetUp() override {
        channel = std::make_unique<QuantumChannel>(0.5, 0.01);
    }
    std::unique_ptr<QuantumChannel> channel;
};

TEST_F(QuantumChannelTest, TransmittanceInRange) {
    EXPECT_GE(channel->transmittance(), 0.0);
    EXPECT_LE(channel->transmittance(), 1.0);
}

// Параметризованные тесты (для разных значений шума)
class ProtocolTest : public ::testing::TestWithParam<double> {};

TEST_P(ProtocolTest, SecretKeyRatePositive) {
    double excessNoise = GetParam();
    double keyRate = computeSecretKeyRate(0.5, excessNoise);
    EXPECT_GT(keyRate, 0.0) << "Failed for noise=" << excessNoise;
}

INSTANTIATE_TEST_SUITE_P(
    NoiseValues,
    ProtocolTest,
    ::testing::Values(0.001, 0.005, 0.01, 0.02)
);
```

### Запуск тестов

```bash
# Из папки build
cd build
cmake .. -DCVQKD_BUILD_TESTS=ON
cmake --build . --config Release
ctest --output-on-failure
```

---

## 🔨 Build Instructions

### Требования

- CMake 3.8+
- MSVC 2019+ (или GCC/Clang на Linux)
- Git (для FetchContent)
- C++17 compiler

### Сборка (Windows)

```powershell
# Clone и настройка
git clone <repo>
cd cvqkd_sim
mkdir build && cd build

# Конфигурация
cmake .. -G "Visual Studio 17 2022" -A x64

# Сборка Release
cmake --build . --config Release

# Запуск симулятора
.\Release\cvqkd_sim.exe

# Запуск тестов
ctest -C Release --output-on-failure
```

### Ключевые CMake опции

| Опция | По умолчанию | Описание |
|-------|--------------|----------|
| `CVQKD_BUILD_TESTS` | ON | Собирать тесты |
| `CMAKE_BUILD_TYPE` | Release | Тип сборки |
| `CMAKE_CXX_STANDARD` | 17 | Стандарт C++ |

---

## 📐 Key Mathematical Formulas

При реализации алгоритмов используй эти формулы как reference:

### 1. Квантовое состояние (условие uncertaintity)

```
γ + i·ℏ·Ω ≥ 0
```

где γ — матрица ковариации, Ω — симплектическая форма.

### 2. Симплектические собственные значения

```
ν_k = |eigenvalues(i·Ω·γ)|
```

Все ν_k ≥ 1 для физических состояний.

### 3. Энтропия фон Неймана гауссова состояния

```
S(γ) = Σ_k g(ν_k)
где g(x) = (x+1)/2 · log((x+1)/2) - (x-1)/2 · log((x-1)/2)
```

### 4. Secret key rate (reverse reconciliation)

```
ΔI = β·I(A:B) - χ(E:B)
где:
  β — эффективность reconciliation (0.95-0.99)
  I(A:B) — взаимная информация Алисы и Боба
  χ(E:B) — Holevo bound (информация Eve)
```

### 5. Трансформация канала на матрице ковариации

```
γ_out = T·γ_in + (1-T)·γ_vac + ξ·I
где:
  T — transmittance
  γ_vac — матрица ковариации вакуума (единичная)
  ξ — excess noise
```

---

## 🎯 AI Agent Guidelines

### При работе с кодом:

1. **Всегда проверяй физическую корректность**:
   - Матрица ковариации должна быть симметричной и положительно определённой
   - Симплектические собственные значения ≥ 1
   - Transmittance ∈ [0, 1]
   - Secret key rate ≥ 0 (иначе протокол небезопасен)

2. **Используй Eigen эффективно**:
   - Избегай циклов там, где есть векторизованные операции
   - Используй `.noalias()` при умножении матриц для избежания временных объектов
   - Для симметричных матриц используй `SelfAdjointEigenSolver`

3. **Тестируй численную стабильность**:
   - Квантовые вычисления чувствительны к точности
   - Используй `double`, не `float`
   - Проверяй условия с эпсилоном: `EXPECT_NEAR(a, b, 1e-10)`

4. **Документируй физические параметры**:
   - Всегда указывай единицы измерения в комментариях
   - Ссылки на статьи (например, "Grosshans & Grangier 2002") приветствуются

### Типичные задачи и подходы:

| Задача | Подход |
|--------|--------|
| Диагонализация матрицы ковариации | `Eigen::SelfAdjointEigenSolver` |
| Вычисление симплектических собственных значений | Eigenvalues of `i·Ω·γ` |
| Williamson decomposition | Симплектическая диагонализация |
| Оптимизация secret key rate | Grid search или gradient-based |
| Монте-Карло симуляция | Параллелизм через `std::thread` или OpenMP |

---

##  References

- Grosshans, F. & Grangier, P. (2002). "Continuous Variable Quantum Cryptography Using Coherent States"
- Weedbrook et al. (2012). "Quantum cryptography with Gaussian states"
- Leverrier, A. et al. (2010). "Composable security proof for continuous-variable quantum key distribution"
- [Eigen Documentation](https://eigen.tuxfamily.org/dox/)
- [nlohmann/json Documentation](https://json.nlohmann.me/)

---

##  Quick Start для новых разработчиков

1. Изучи структуру: `src/`, `include/`, `tests/`
2. Запусти тесты: `cd build && ctest --output-on-failure`
3. Прочитай `main.cpp` — точка входа симуляции
4. Для добавления нового модуля:
   - Создай `.hpp` в `include/`
   - Создай `.cpp` в `src/`
   - Добавь тест в `tests/`
   - CMake автоматически подхватит файлы (GLOB_RECURSE)

---