<h1 align="center">
    <img src=".readme/Logo.png" alt="StockDory" width=600>
</h1>

<h3 align="center">
    Strong Neural Network Chess Engine
</h3>

<p align="center">
<a href="https://www.runpod.io/">
<img
        alt="Runpod Logo"
        src="https://img.shields.io/badge/RUNPOD-Honorable%20Sponsor-blue?logo=data:image/png;base64,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&logoWidth=20&style=for-the-badge&labelColor=black"
>
</a>
</p>

### 🌐 Overview

- 🧠 Exceptional Tactical Analysis
- 🚀 Blazing Performance
- 📦 Lightweight and Scalable
- 💻 Cross Platform
- 🔄 UCI Compatible
- 👌 Free and Open Source

StockDory is a modern C++ chess engine combining neural-network evaluation with a deeply optimized tree search. Capable 
of analyzing millions of positions every second with remarkable accuracy, StockDory plays chess at a level far beyond 
human ability.

### 🎮 Building & Using

For the quickest way to get started, it is recommended to use one of the
[official release binaries](https://github.com/TheBlackPlague/StockDory/releases). These builds target the three major
desktop operating systems and common CPU architecture levels, making them suitable for most systems likely to run
StockDory.

> [!NOTE]
> StockDory is a command-line program that communicates through the **standard input and output streams** using the
> **Universal Chess Interface (UCI)** protocol. For normal use, it is recommended to pair StockDory with a
> **UCI-compatible** graphical interface such as [En Croissant](https://encroissant.org/).

For maximum performance, however, the best StockDory build is generally one compiled specifically for your native
hardware. The official releases cannot reasonably cover every processor or architecture combination, particularly for
new, uncommon, or recently introduced hardware. Compiling StockDory locally is therefore recommended when no suitable
release binary exists, when targeting a specialized platform, or when you simply want the best possible performance
from your system.

StockDory currently targets **C++23** and officially supports the **LLVM toolchain**. The recommended build environment
is:

* 👽 Git >= 2.30
* 🏗️ CMake >= 3.21 (recommended: 4.0.x)
* 🥷 Ninja >= 1.10 (recommended: 1.12)
* 🐉 LLVM >= 22 (recommended: 22.1.8)

> [!CAUTION]
> Other toolchains or build-system substitutions may work, but they are not officially supported and may produce builds
> with different behavior or performance characteristics. Maintaining equivalent support across multiple compiler
> toolchains is impractical due to differences in compiler behavior, optimization capabilities, and platform 
> integration.

**Compilation Steps:**
```shell
# Run these commands from a directory that does not already contain a
# directory named "StockDory"

# Clone the StockDory repository.
git clone https://github.com/TheBlackPlague/StockDory.git
cd StockDory

# Ensure the required toolchain is available through your system PATH
# before continuing...

# Choose one of the configurations below:

# OPTION A ---
# Native Build
#
# Recommended for local use. This build targets the capabilities of the
# current system and will generally provide the best performance on the
# machine on which it is compiled. 
# 
# The resulting binary may not be portable to systems with different CPU 
# capabilities.
cmake -B Build -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_C_COMPILER=clang \
      -DCMAKE_CXX_COMPILER=clang++

# OPTION B ---
# Architecture-Targeted Build
#
# Recommended when building for redistribution. This disables native CPU
# targeting and instead generates a binary for the explicitly selected
# architecture or architecture level, such as x86-64-v3.
#
# Such a build is more portable across compatible systems, although it
# may not achieve the maximum possible performance on any one machine.
cmake -B Build -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_C_COMPILER=clang \
      -DCMAKE_CXX_COMPILER=clang++ \
      -DBUILD_NATIVE=OFF \
      -DARCHITECTURE=<architecture>

# After running one of the above options, build StockDory using the 
# selected configuration
cmake --build Build

# The resulting executable will be available under:
# StockDory/Build

# It is recommended to test that it is working by running the bench command
cd Build
./StockDory bench
```

### 📑 Terms of Use

StockDory is licensed under [GNU AGPL v3.0](LICENSE).

The StockDory logo is licensed separately under the
[CC BY-NC-ND 4.0](https://creativecommons.org/licenses/by-nc-nd/4.0/) license.
