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

