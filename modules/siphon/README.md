# Siphon Module

Siphon is a _router_ module that fetches data directly from client drivers by
loading their respective dll/so libraries and calling the pre-defined
interfaces.

## Load Siphon Module into Router

### R2D2

```
connection load --router <path-to-siphon-module-dynamic-lib>
```

### Radeon Developer Service (RDS)

TODO: Find out how to load modules into RDS. Or it has be built into RDS executable?

## Settings

Currently, Siphon module, upon loading, collects Settings YAML text blobs from
client drivers. It implements a set of [PRC services](siphon-rpc-services) for
serving these blobs to clients.

