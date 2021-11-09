# VRPC - Variadic Remote Procedure Calls

[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/heisenware/vrpc-r/master/LICENSE)
[![Semver](https://img.shields.io/badge/semver-2.0.0-blue)](https://semver.org/spec/v2.0.0.html)
[![GitHub Releases](https://img.shields.io/github/tag/heisenware/vrpc-r.svg)](https://github.com/heisenware/vrpc-r/tag)
[![GitHub Issues](https://img.shields.io/github/issues/heisenware/vrpc-r.svg)](http://github.com/heisenware/vrpc-r/issues)
![ci](https://github.com/heisenware/vrpc-r/actions/workflows/ci.yml/badge.svg)

## Visit our website: [vrpc.io](https://vrpc.io)

---

## What is VRPC?

VRPC - Variadic Remote Procedure Calls - is an enhancement of the old RPC
(remote procedure calls) idea. Like RPC, it allows to directly call functions
written in any programming language by functions written in any other (or the
same) programming language. Unlike RPC, VRPC furthermore supports:

- non-intrusive adaption of existing code, making it remotely callable
- remote function calls on many distributed receivers at the same time (one
  client - multiple agents)
- service discovery
- outbound-connection-only network architecture (using MQTT technology)
- isolated (multi-tenant) and shared access modes to remotely available
  resources
- asynchronous language constructs (callbacks, promises, event-loops)
- OOP (classes, objects, member functions) and functional (lambda) patterns
- exception forwarding

VRPC is available for an entire spectrum of programming technologies including
embedded (arduino, header-only C++, etc.), data-science (python, R,
etc.), and web (javascript, react, etc.) technologies.

As a robust and highly performing communication system it can build the
foundation of complex digitization projects in the area of (I)IoT or
Cloud-Computing.

## This is VRPC for R

The VRPC R agent provides remote access to the entire R language functionality.

Using a [VRPC client](https://vrpc.io/technology/agent-and-client), existing R
code can be called by any other (VRPC-supported) programming language and
from any remote location - e.g. directly from the browser within a
[React](https://reactjs.org) web application.

### Technology

The VRPC R package implements a fully fledged MQTT client in pure
dependency-free C++ (thanks to the great work of
[Takatoshi Kondo](https://github.com/redboltz/mqtt_cpp)).
Based on VRPC's asynchronous [RPC protocol](https://vrpc.io/docs/remote-protocol),
the MQTT client calls R functions in an asynchronous and isolated fashion.

### Teaser

Agent code: *hello-world.R*

```R
library(vrpc)

greet <- function(whom) {
  paste("Hello,", whom)
}

start_vrpc_agent(domain = "public.vrpc")
```

Run in terminal:

```bash
Rscript hello-world.R # displays generated agent name
```

Client code: *hello-world.js*

```js
const { VrpcRemote } = require('vrpc')

;(async () => {
  const client = new VrpcRemote({ domain: 'public.vrpc', agent: process.argv[2] })
  await client.connect()
  console.log(await client.callStatic({
    className: 'Session',
    functionName: 'greet',
    args: ['world!']
  }))
})()
```

Run in another terminal:

```bash
node index.js <agentName> # will print: "Hello, world!"
```

> **DISCLAIMER**
>
> At this very early time of this brand-new repository the VRPC package is not
> (yet) available via CRAN. You may still run the example utilizing the
> all-inclusive docker image `heisenware/vrpc-r` (see shipped Dockerfile)
> or by building the vrpc package your own (`R CMD INSTALL --preclean vrpc`).

### Features

- lightning fast using plain C++-based MQTT with zero protocol overhead
- inherently parallel execution of R code
- support for stateless calls (mapped to VRPC static functions)
- support for stateful calls (R sessions mapped to VRPC instances)
- forwarding of graphical R output (plots) as high-quality SVGs
- forwarding of R errors as regular exceptions
- auto discovery of all available R functionality
- uncomplicated, client-only network architecture with constant management
  overhead even for large applications

### Differences to OpenCPU

The [OpenCPU](https://github.com/opencpu/opencpu) project is another very nice
solution that provides generic remote access to existing R code.

Depending on your use case, this might be the better choice for you. In order to
understand the differences, here is a small and very high-level comparison:

| VRPC  | OpenCPU  |
|-------|----------|
| transport protocol is MQTT   | transport protocol is HTTP   |
| no API but language specific clients (SDK-like)  | RESTful API  |
| establishes a single permanent connection as TCP client  | establishes a connection per request as TPC server  |
| provides access to code within the same file and packages | provides access to code within packages |
| allows stateful interaction in form of `Session` class instances (*OOP style*) | allows stateful interaction using session keys (*functional state style*)|

### Differences to Shiny

[Shiny](https://shiny.rstudio.com/) allows building interactive web apps
straight from R and solely using R. In effect, reactive web-apps are realized
using server-side rendering and server-side business logic for wiring input
element updates (such as button clicks or slider moves) to output element
updates (such as re-rendered tables or plots). Hence, the client (= browser)
receives fully pre-rendered HTML pages for display.

VRPC in contrast, does not deal with HTML nor CSS. It focuses on transporting
data in the most performing and simple way. Visualization and reactivity can
still be implemented but those concepts are separated and left to technology
that is much better suited to do so than R would ever be.

See for example [VRPC Live](https://live.vrpc.io) which allows all the
reactivity-wiring and visualization using client-side code based on
[React](https://reactjs.org). It calls the R functions (and if needed any other
technology's functions) to retrieve the relevant data when needed.

Or implement your own React application and use
[react-vrpc](https://www.npmjs.com/package/react-vrpc) for backend connectivity
and state management.

### Examples

- [Hello World Example](examples/01-hello-world/README.md)
- [State Management Example](examples/02-stateful/README.md)
- [Plotting Example](examples/03-svg/README.md)

## Maintenance

This open-source project is professionally managed and supported by
[Heisenware GmbH](https://heisenware.com).
