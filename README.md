# Starlings

This project is an early-stage effort to rewrite and redesign the core signal processing layers of [Flocking](https://flockingjs.org) in C. The goal is to support instruments that can be deployed without modification across different devices and platforms, with a particular emphasis on the Web and embedded platforms such as Eurorack.

## Goals

* *Live repatching without compilation*: provide a highly mutable audio graph, so that instruments can be dynamically reconfigured while they are running (on platforms where this is appropriate).
* *Vastly portable*: support deployment on the Web (via Web Assembly), desktop and mobile operating systems (macOS, Windows, Linux, iOS), specialized audio OS-based environments (Bela), and microcontroller-based systems (Daisy).
* *Multirepresentational and openly authorable*: serve as a foundation for supporting [open authorial practices](https://github.com/amb26/papers/blob/master/onward-2016/onward-2016.pdf) while offering a very low-level signal graph data representation and API upon which more supportive abstractions can be layered.
* *Fully externalized state*: Provide a means for [fully externalizing all state](http://openresearch.ocadu.ca/id/eprint/2059/1/Clark_sdr_2017_preprint.pdf) via a Nexus-like RESTful API, including:
    * Creating signals
    * Getting signal values/representations
    * Connecting/disconnecting signals or updating their values
    * Deleting signals
* Provide first-class support for using signal generators in non-audio environments such as video processing tools like [Bubbles](https://github.com/colinbdclark/bubbles).
* Make it easier to compose signal processing algorithms from smaller, self-contained pieces; avoid Flocking's (and SuperCollider's) formal distinction between unit generators and synths.
* Support variable sample block sizes that can be mixed together in the same graph, including single-sample graphs.
* Support cyclical graphs, multiplexing/demultiplexing of signals, and multiple channels
* Provide an architecture that is supportive of multiple processes, including a realtime process that:
    * Allocates no memory
    * Takes no locks
    * Communicates via a lock-free message queue and/or circular buffers

## Design and Approach

The design of this project is still in flux, and will more fully emerge as I become more familiar with C and the constraints of each of the core environments on which it will run (Web, desktop/mobile, and Daisy, in particular). However, there are a few abstractions that are beginning to crystallize:
* The core library, consisting of _Signals_ (which can be individual signal generators or compositions of them) and the _Evaluator_ (which draws samples from Signals), will be completely platform-agnostic and must be integrated with a particular audio API. It will be usable 1) directly in C/C++ applications, 2) in Audio Worklets by being compiled to Web Assembly with JavaScript API bindings, 3) within other languages that provide interoperability with the C ABI.
* A set of _Hosts_ will be developed, which will provide platform-specific logic for connecting to audio input and output devices, encoding/decoding audio files, and mapping hardware-specifc buses (e.g. GPIO, analog pins, or I2S) to Signals.

## Dependencies
To build Starlings itself:
1. Meson ```brew install meson```
2.

To compile to wasm:
1. LLVM ```brew install llvm```

To compile to Daisy-based Eurorack modules:
1. XCode command line tools ```xcode-select --install```
2. GCC ARM Embedded ```brew install gcc-arm-embedded```
3. The [Daisy Toolchain](https://github.com/electro-smith/DaisyWiki/wiki/1.-Setting-Up-Your-Development-Environment#1-install-the-toolchain)

## Building Starlings

### libstar
1. ```meson setup build```
2. ```meson compile -C build```

### Examples

#### Web Host Example
1. ```cd hosts/web/examples/print-silence```
2. ```./build.sh```
3. Open ```index.html``` using VS Code's LiveServer or other webserver

#### Bluemchen Example
1. ```cd hosts/daisy/vendor/libDaisy```
2. ```make```
3. ```cd ../../examples/bluemchen```
2. ```make```
3. Use the [Daisy Web Programmer](https://electro-smith.github.io/Programmer/) to flash the ```build/libflock-bluemchen-example.bin``` binary to the Daisy board

## Attribution

Starlings is developed by Colin Clark and is licenced under the [MIT License](LICENSE).
