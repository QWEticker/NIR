This project is a sophisticated simulation framework for Quantum Key Distribution (QKD), modeling various physical layers, signal processing techniques, attacks, and post-processing steps. The architecture is highly modular, utilizing C++ best practices with clear separation of concerns across different modules.

### 🛠️ Code Review Summary

The codebase demonstrates an excellent level of abstraction suitable for research simulation. Key strengths include:
1.  **Modularity**: Components (QPSK Modulation, Quantum Channel, Parameter Estimation, Attacks) are isolated into distinct classes and headers, promoting maintainability.
2.  **Configuration Management**: Using `ExperimentConfig` to centralize all physical parameters (T, $\xi$, $\alpha$, etc.) allows for easy parameter sweeping without altering core logic.
3.  **Simulation Flow**: The `main.cpp` structure correctly implements a simulation loop: Configuration $\rightarrow$ Signal Generation $\rightarrow$ Channel Transmission $\rightarrow$ Detection/Measurement $\rightarrow$ Post-processing (Estimation, Reconciliation) $\rightarrow$ Attack Simulation.

**Potential Areas for Improvement:**

1.  **Error Handling in `CMakeLists.txt`**: The use of `message(FATAL_ERROR ...)` is good but could be wrapped in a more user-friendly check or provide clearer installation instructions if the dependencies are missing.
2.  **Attack Base Class**: Since `include/attacks/attack_base.hpp` was empty, ensure that all derived attack classes (like `SaturationAttack`) implement a consistent interface method (e.g., `apply(const std::vector<Complex>& signal)`) and handle any necessary state management or randomness internally to prevent duplication of boilerplate code.
3.  **Type Consistency**: While `core/types.hpp` defines clear type aliases (`VectorCd`, `MatrixCd`), ensure that all modules consistently use these types instead of raw Eigen types (e.g., `Eigen::VectorXcd`) to maintain strict adherence to the project's defined interface.
4.  **Testing Coverage**: The existence of dedicated test files is excellent. Ensure that tests for complex interactions, such as running an attack *after* parameter estimation but *before* reconciliation, are explicitly covered.

***

# Quantum Key Distribution Simulation Framework (cvqkd)

This repository provides a comprehensive C++ simulation framework for studying various aspects of Quantum Key Distribution (QKD). It models physical communication channels, signal processing techniques (e.g., heterodyne detection), security attacks, and post-processing protocols necessary for secure key generation.

## 🚀 Getting Started
The project is built using CMake and requires the following dependencies:
*   Eigen (for linear algebra operations)
*   nlohmann/json (for configuration loading)
*   Google Test (for unit testing)

**Build Instructions:**
```bash
cmake -B build -DCVQKD_BUILD_TESTS=ON
cmake --build build
```

**Usage:**
The simulation is executed via the `cvqkd_sim` executable. Configuration and output paths can be specified at runtime:
```bash
./bin/cvqkd_sim --config <path/to/config.json> --out <output/results.csv>
```

## 📁 Project Structure & Modules

### 1. Core Utilities (`include/core`)
These files provide fundamental data types, configuration handling, and random number generation used across all modules.

*   **`types.hpp`**: Defines essential complex type aliases (e.g., `VectorCd`, `MatrixCd`) for consistent use of complex-valued vector and matrix operations throughout the simulation.
*   **`config.hpp` / `core/config.cpp`**: Manages all physical parameters of the simulated environment, including channel transmissivity ($\text{T}$), excess noise ($\xi$), detector efficiency ($\eta$), and saturation levels. It centralizes experiment setup via `ExperimentConfig`.
*   **`rng.hpp` / `core/rng.cpp`**: Provides a thread-safe, deterministic Random Number Generator (`RNG`) using Mersenne Twister (MT19937_64) for reproducible simulations across different runs.

### 2. Protocol Modules (`include/protocol`)
These modules model the physical transmission and detection processes in QKD.

*   **`qpsk_modulator.hpp` / `core/qpsk_modulator.cpp`**: Responsible for generating Alice's transmitted quantum states using Quadrature Phase-Shift Keying (QPSK). It converts bit keys into complex amplitude vectors ($\alpha \cdot i^k$).
*   **`quantum_channel.hpp` / `core/quantum_channel.cpp`**: Models the lossy quantum channel between Alice and Bob. It simulates signal attenuation ($T$) and introduces excess noise ($\xi$), transforming ideal signals into realistic received amplitudes.
*   **`heterodyne_detector.hpp` / `core/heterodyne_detector.cpp`**: Simulates the detection process at Bob's receiver, which measures both amplitude and phase quadratures (X and P) of the incoming signal.
*   **`phase_discretizer.hpp` / `core/phase_discretizer.cpp`**: Handles the quantization or discretization of continuous physical measurements into discrete digital values used for key generation.

### 3. Post-Processing & Security (`include/postprocessing`)
These modules implement classical post-processing steps required to distill a secure key from raw measurement data.

*   **`parameter_estimator.hpp` / `core/parameter_estimator.cpp`**: Estimates unknown channel parameters (e.g., $\hat{T}$ and $\hat{\xi}$) using Maximum Likelihood Estimation (MLE) based on observed signal pairs, crucial for accounting for real-world channel variations.
*   **`security_analyzer.hpp` / `core/security_analyzer.cpp`**: Performs security checks (e.g., privacy amplification analysis) to quantify the amount of information an eavesdropper could have gained about the key.
*   **`reconciliation.hpp` / `core/reconciliation.cpp`**: Implements error correction protocols (e.g., Cascade or LDPC) necessary to correct discrepancies between Alice's and Bob's raw keys, minimizing the final Quantum Bit Error Rate (QBER).

### 4. Attack Simulation (`include/attacks`)
This module defines various attack vectors against the QKD system.

*   **`attack_base.hpp`**: Defines the abstract base class for all attacks, enforcing a standard interface that derived classes must implement to interact with the simulated signal stream.
*   **`saturation_attack.hpp` / `core/saturation_attack.cpp`**: Simulates an eavesdropping attack where the detector is intentionally saturated, limiting measurable information and potentially introducing biases.
*   **`lo_manipulation_attack.hpp`**: Models a Line-of-Sight (LoS) manipulation attack, simulating an attacker who intercepts and modifies signals directly in the channel path.

### 5. Execution (`main.cpp`)
The entry point orchestrates the entire simulation pipeline:
1.  Reads configuration from JSON.
2.  Initializes `ExperimentRunner`.
3.  Attaches specified attacks to the runner.
4.  Executes the full cycle: QPSK $\rightarrow$ Quantum Channel $\rightarrow$ Detection $\rightarrow$ Attack $\rightarrow$ Parameter Estimation $\rightarrow$ Reconciliation $\rightarrow$ Key Output.

