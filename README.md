# VRPC R Agent

The VRPC R agent provides remote access to the entire R language functionality.

Using a [VRPC client](https://vrpc.io/technology/agent-and-client), you may call
your R code using any other language and from
any remote location, e.g. directly from the browser within an React web app.

## Technology

The `vrpc` R package implements a fully fledged MQTT client in pure
dependency-free C++ (thanks to the great work of
[Takatoshi Kondo](https://github.com/redboltz/mqtt_cpp)).
Using VRPC's asynchronous [RPC protocol](https://vrpc.io/docs/remote-protocol)
this client calls R functions in an asynchronous and isolated fashion.

## Example

*hello-world.R*

```R
library("vrpc")

greet <- function(whom) {
  paste0("Hello, ", whom)
}

vrpc_start_agent(agent = "hello-world")
```

*hello-world.js*

```js
const { VrpcRemote } = require('vrpc')

(async () => {
  const client = new VrpcRemote({ agent: 'hello-world' })
  await client.connect()
  const ret = await client.staticCall({ functionName: 'greet', args: ['world!'] })
  console.log(ret) // Hello, world!
})()
```

## Features

- lightning fast using plain C++-based MQTT with zero protocol overhead
- parallel execution of R code
- stateful R sessions mapped to VRPC instances
- support of graphical output (plots)
- error forwarding
- auto discovery of available agents and provided functionality
- uncomplicated, client-only network architecture that is easily manage-able
  even for large applications
