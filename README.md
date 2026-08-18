# DevDriver

This repo houses code owned by the DevDriver team.

## Communication APIs

### ddNet

Provides a way for applications to acquire a `DDNetConnection` handle for use with other APIs. A handle is produced by connecting to a `ddRouter` owned network.

### ddSocket

Low-level networking functionality used to send and receive byte-streams. Requires a `DDNetConnection`.

### ddRouter

Functionality for managing user-mode routers. Routers are responsible for moving messages between clients on their network. Applications and drivers can connect to this network via `ddNet`.

### ddRpc

High-level abstraction over `ddSocket` that exposes a remote procedure call interface. Includes a utility that generates boilerplate client/server code. Requires a `DDNetConnection`.

### ddEvent

High-level abstraction over `ddSocket` that exposes an asynchronous event notification interface. Allows applications to remotely discover and configure events supported by the target. Requires a `DDNetConnection`.

### ddTool

Foundation for module based tooling. Supports dynamically loading tool modules and provides streamlined access to the network for module implementations.

## Getting Started

CMake is the official build system for DevDriver.

### Include DevDriver in an existing Project

The DevDriver libraries are designed to be used from CMake. To include it in an existing project, add the folder that contains this repo. We highly recommend making this path override-able,  so that you can point it to in-development builds of DevDriver easily.

```cmake
### Configure DevDriver Api targets
set(DEVDRIVER_LIB_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/my-path-to-devdriver
    CACHE STRING
    "Overridable Path to DevDriver library"
)
message(STATUS "Using devdriver from \"${DEVDRIVER_LIB_PATH}\"")
add_subdirectory(${DEVDRIVER_LIB_PATH} devdriver)
```

### Build DevDriver

#### Windows

Install Visual Studio (when selecting workloads, choose "Desktop Development with C++").

To generate the CMake project, we'll use Visual Studio 2022 as an example.

Under your `devdriver` root directory:

```sh
$ cmake -S . -B _vs -G "Visual Studio 17 2022" -D DD_MSVC_CODE_ANALYZE=OFF
$ cmake --build _vs --parallel
```

When everything works correctly, we will see some CMake output and a message like this:

```txt
-- Build files have been written to: C:/Code/devdriver/_vs
```

Now there should be a solution file `_vs\DevDriver.sln` generated. We can open
it with `cmake --open _vs` and build from Visual Studio.

We can also build from the command line using CMake like this:

```sh
$ cmake --build _vs --config Release --parallel 16
```

If you have Ninja installed, you can also use Ninja for generation. Ninja is
faster on Windows, but it doesn't generate Visual Studio solutions.

```sh
$ cmake -S . -B _ninja -G Ninja
```

#### Linux

For Linux development, the libdrm-dev package is required to compile listener
core.  This can be installed using your package manager, e.g.

```sh
$ sudo apt-get install libdrm-dev
```

Configure and build devdriver:

```sh
$ cmake -S . -B _ninja -G Ninja
$ cmake --build _ninja
```
