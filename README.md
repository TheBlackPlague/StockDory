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
- 🧠 **Powerful and Deep Analysis**
- 🚀 **High Performance**
- 📦 **Lightweight**
- 💻 **Cross Platform**
- 🔄 **UCI Compatible**
- 👌 **Free and Open Source**

StockDory, the mark of strength and efficiency, emerges as a proficient chess engine reengineered
in C++ from its popular and mighty C# predecessor, StockNemo.
Striving to ascend the peaks of excellence, StockDory serves as a testament to the spirit of 
continuous improvement, eliminating bugs and performance issues that blemished its otherwise
remarkable predecessor.

Download the latest release and try it out for yourself! 
For maximum performance, compile the engine specifically for your hardware using the
instructions below.

### 🛠️ Compiling
StockDory is written in C++ and uses CMake as its build system.

**Requirements**:
- 🏭 CMake >= 3.15
- 🐉 Clang (LLVM) >= 16.0.0
- 🥷 Ninja >= 1.10.2

**Steps**:
- 💾 Clone the repository
```bash
git clone https://github.com/TheBlackPlague/StockDory.git
```
- 🔦 Setup Build Process
```bash
cd StockDory
cmake -B Build -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -G Ninja
```
- 🪛 Compile
```bash
cmake --build Build --config Release
```
- 🏃‍♂️ Run
```bash
./Build/StockDory
```

### 🤝 Contributing
🖥 **Hardware Contributions:**

StockDory requires a lot of computational power to be tested and improved.
As such, the [FindingChess Testing Framework](http://tests.findingchess.com/)
was created.
This framework allows anyone to contribute to the development of StockDory
by donating their computational power.
It works by letting you run a Python script on your computer that will
automatically download and test StockDory on your hardware, relaying the
results back to the framework.

If you would like to contribute hardware for the development of StockDory,
please download the Client Worker from the
[FindingChess Testing Framework](http://tests.findingchess.com/) and run it.

📝 **Pull Requests and Bug Reports:**

StockDory is a community project, and as such, we welcome any and all contributors
looking to improve the codebase or report bugs.

If you would like to report a bug, please open an issue on this GitHub repository.

If you would like to contribute to the codebase, please fork this repository,
create a new branch (naming it appropriately for the changes you are making),
make your changes.
Then, create an account on the [FindingChess Testing Framework](http://tests.findingchess.com/).
Once your account is approved, appropriately set the source repository on your profile, and
create a test for your branch.

StockDory requires two tests to pass before a pull request can be merged: STC and LTC.

Once your STC test has passed, only then can you request a LTC test.
Given that the LTC test also passes, you may then open a pull request on this repository.

Please understand that at times your pull request may require some changes before it can be merged,
and that this is not a reflection of your work, but rather a reflection of the high standards
that StockDory is held to.

### 📑 Terms of Use
🚂 **StockDory Engine:**

The StockDory engine is licensed under the [LGPL-3.0](LICENSE). 
This is a very permissive license that allows you to use the engine
in almost any way you want.
This includes being able to send the engine to your friends, and
even being able to use the engine in your own projects.

The only requirement is that you must make the source code of your
project available to the public, and that StockDory's License is
included in your project.
This is to ensure that the engine, all the improvements made to it,
and all the works derived from it, remain free, open-source, and for
the benefit of the public.

StockNemo or StockDory would've never existed if it wasn't for the
generosity of the open-source community, and it's now our turn to
carry on that tradition.

🎀 **StockDory Logo:**
The StockDory Logo is licensed under the 
[CC BY-NC-ND 4.0 License](https://creativecommons.org/licenses/by-nc-nd/4.0/).

The Logo must be used in its entirety, and cannot be modified in any way.