# ddRouter

## Overview

`ddRouter` is API for starting a Router, the entry point for starting a network. It is what creates a network and routes messages between clients.

A Router can be run locally on a remote machine for the tool to connect to, or can run on the same machine, or even in the same process as a tool. Most tools have some capacity to start their own router.

## Basic Usage

The `ddRouter` API is described in [`ddRouter.h`](./inc/ddRouter.h) and requires the `ddRouter` shared library to run. The API provides a `DDRouterContext` object that can be created to start a network.

See [simple-router](../rust/simple-router/src/main.rs) for an introductory example of a Router project.

## Extending a Router with Modules

Sometimes a Tool Module needs code executing on the target machine but outside of the driver. For example, the DirectX Runtime produces ETW events related to drivers that a tool may wish to consume. In order to capture that data and make it available to tools we need a way to support code execution on the target machine. Enter Modules.

These Modules allow other AMD teams to hook ddSocket-based tooling into the network from the target machine. This includes [`ddRpc`](../ddRpc), [`ddEvent`](../ddEvent), as well as raw [`ddSocket`](../ddSocket) protocols. The core mechanic here is that the Router will manage loading, connecting (to the network), and disconnecting these protocol implementations. Each implementation is in charge of handling its own client connections.
