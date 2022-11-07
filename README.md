# Signaletic

This project is an early-stage effort to rewrite and redesign the core signal processing layers of [Flocking](https://flockingjs.org) in C. The goal is to support instruments that can be deployed without modification across different devices and platforms, with a particular emphasis on the Web and embedded platforms such as Eurorack.

## Goals

* *Multirepresentational*: Signaletic will provide the low-level infrastructure to support both visual patching alongside an interpreted and transpiled-to-C "signal expression language" (and to switch back and forth between representations on the fly).
* *Live repatching without compilation*: provide a highly mutable audio graph, so that instruments can be dynamically reconfigured while they are running (on platforms where this is appropriate).
* *Vastly portable, "embedded first"*: support deployment on resource-constrained embedded microcontroller-bsaed systems (such as Daisy), browser-based AudioWorklets (via Web Assembly), and desktop and mobile operating systems (macOS, Windows, Linux, iOS).
* *Fully externalized state*: Provide a means for [fully externalizing all state](http://openresearch.ocadu.ca/id/eprint/2059/1/Clark_sdr_2017_preprint.pdf) via a Nexus-like RESTful API, including:
    * Creating signals
    * Getting signal values/representations
    * Connecting/disconnecting signals or updating their values
    * Deleting signals
* Provide first-class support for using signal generators in non-audio environments such as video processing tools like [Bubbles](https://github.com/colinbdclark/bubbles).
* Make it easier to compose signal processing algorithms from smaller, self-contained pieces; avoid Flocking's (and SuperCollider's) formal distinction between unit generators and synths.
* Support variable sample block sizes that can be mixed together in the same graph, including single-sample graphs.
* Support cyclical graphs, multiplexing/demultiplexing of signals, and multiple channels
* Provide an architecture that is supportive of realtime programming techniques such as:
    * Static allocation where desired
    * Constant-time memory allocation where needed
    * Takes no locks
    * Communicates via a lock-free message queue and/or circular buffers

## Design and Approach

The design of this project is still in flux, and will more fully emerge as I become more familiar with C and the constraints of each of the core environments on which it will run (Web, desktop/mobile, and Daisy, in particular). However, there are a few abstractions that are beginning to crystallize:
* The core library, consisting of _Signals_ (which can be individual signal generators or compositions of them) and the _Evaluator_ (which draws samples from Signals), will be completely platform-agnostic and must be integrated with a particular audio API. It will be usable 1) directly in C/C++ applications, 2) in Audio Worklets by being compiled to Web Assembly with JavaScript API bindings, 3) within other languages that provide interoperability with the C ABI.
* A set of _Hosts_ will be developed, which will provide platform-specific logic for connecting to audio input and output devices, encoding/decoding audio files, and mapping hardware-specifc buses (e.g. GPIO, analog pins, or I2S) to Signals.


## Installing Dependencies

### macOS

#### libsignaletic
[Meson](https://mesonbuild.com/) and the XCode command line tools are required to build the Signaletic library:
1. ```brew install meson```
2. ```xcode-select --install```

#### libsignaletic Web Assembly
libsignaletic uses the [Emscripten compiler](https://emscripten.org/) for Web Assembly support. Docker is used to support cross-platform builds.
1. Install [Docker Desktop](https://www.docker.com/products/docker-desktop/).

#### Daisy Eurorack Examples
Docker is also used to cross-compile the Signaletic examples for embedded platforms such as the Daisy STM32 platform. If you want to build these directly on your host using gcc and make, the GCC ARM embedded toolkit and the Daisy Toolchain must be installed.
1. ```brew install gcc-arm-embedded```
2. Follow the [Daisy Toolchain](https://github.com/electro-smith/DaisyWiki/wiki/1.-Setting-Up-Your-Development-Environment#1-install-the-toolchain) installation instructions.

### Windows
1. Install Meson using the [Meson Installer](https://github.com/mesonbuild/meson/releases)
2. Install [Docker Desktop](https://www.docker.com/products/docker-desktop/).

## Building Signaletic

### macOS, Linux, and Windows

On Windows, use a VS Command Prompt or set the appropriate environment variables so that MSVC is the default compiler.

#### libsignaletic Native
1. ```cd libsignaletic```
2. ```meson setup build/native```
3. ```meson compile -C build/native```

To remove all previous build artifacts and rebuild, run ```rm -r build/native && meson setup build/native``` or run ```meson setup build/native --wipe```.

#### libsignaletic for Web Assembly
At the root of the Signaletic repository:
1. Build the Docker image: ```docker build . -t signaletic```
2. Run the cross-compilation script in Docker: ```docker run --platform=linux/amd64 -v `pwd`:/signaletic signaletic /signaletic/cross-build-all.sh```

#### Running the Unit Tests
1. Native: ```meson test -C build/native -v```
2. Node.js wasm: ```node build/wasm/run_tests.js```
3. Browser wasm: Open ```libsignaletic/tests/test-libsignaletic.html``` using VS Code's Live Server plugin or other web server.


#### Running the Examples

##### Console Example
1. Build libsignaletic
2. ```./build/native/libsignaletic-console-example```

#### Web Example
1. Build libsignaletic Web Assembly
2. Open ```hosts/web/examples/midi-to-freq/index.html``` using VS Code's Live Server plugin or other web server.

##### Daisy Bluemchen Examples
1. If you haven't already, build the Docker image ```docker build . -t signaletic```
2. ```docker run --platform=linux/amd64 -v `pwd`:/signaletic signaletic /signaletic/cross-build-all.sh```
3. Use the [Daisy Web Programmer](https://electro-smith.github.io/Programmer/) to flash the ```build/signaletic-bluemchen-looper.bin``` binary to the Daisy board, or run ```make program``` while connected to an ST-Link Mini debugger.


## Language and Compiler Support
Signaletic's core is written in C using the C99 standard (due to the use of C++ style comments, for loops with initial declarations, and float math functions like sinef and powf). It is currently compiled and tested on LLVM on macOS, GCC on Ubuntu Linux, and the Visual Studio C compiler on Windows.

On the Daisy platform, Signaletic is compiled using Daisy's own toolchain, which uses the gnu11 standard for C and gnu++14 for C++. Compiling the Daisy Host and examples is currently supported using GCC on macOS.

On the Web, Signaletic is compiled using the Emscripten compiler toolchain.

## Respecting Open Source Labour

Signaletic is offered as open source software for you to use and modify within a culture of reciprocity. It is built by volunteers, who take time out of their personal and family lives to share their work and enthusiasm with a community of peers. Please respect this labour by contributing your own time, resources, and energies in whatever form you can offer. This might be as simple as sharing artwork or projects you've made with Signaletic, by contributing code and documentation, or by respectfully filing bug reports and feature requests.

Many open source contributors have experienced burnout because of unrealistic or asymmetrical expectations by some community members, particularly those who are being paid to use the project but don't contribute back to it. While it's important and rewarding for us to help out with your projects, an immediate response to issues, problems, questions, or bugs often isn't feasible.

If you're using Signaletic in a commercial project, in a large grant-funded artwork, or as an essential component in your academic teaching or research, please consider sharing financial support or your own labour with the Signaletic community.

## Attribution

Signaletic is developed by Colin Clark and is licenced under the [MIT License](LICENSE).
