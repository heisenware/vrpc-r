# Example 2 - "Stateful"

This example shows, how VRPC handles stateful interactions with R.

The example here takes up the ideas as shown in the
[shiny reactivity example](https://github.com/rstudio/shiny-examples/tree/master/003-reactivity),
to demonstrate how these concepts map to VRPC.

## Step 1: Stateful R code to be called remotely

The fundamental idea behind the shiny example is to show that input parameters
can be explicitly wired, such that only specific functions
are re-evaluated and not simply everything. Shiny calls this concept "reactivity"
and technically it does (at least) two things:

1. Building up dependency chains (a polytree) for functions required
   to evaluate on a given input change
2. Caching of last-values-changed to avoid unnecessary re-evaluations

As VRPC does not implement the reactivity wiring in R, but in the client application
this leads to the following effect:

1. Individual tasks can be implemented using individual functions
2. Caching is naturally done by sharing state between the functions (using
   variables)

This leads to very simple and clean code:

```R
library("vrpc")

dataset <- NULL

select_dataset <- function(name) {
  dataset <<- switch(name,
    "rock" = rock,
    "pressure" = pressure,
    "cars" = cars
  )
  return(TRUE)
}

get_summary <- function() {
  summary(dataset)
}

get_table <- function(obs) {
  head(dataset, n = obs)
}

start_vrpc_agent(domain = "public.vrpc")
```

Note, that the `caption` output from the shiny example is entirely skipped here
as there is no computation but only re-rendering involved. In VRPC the caption
update is accomplished solely on client-side. Shiny needs a roundtrip
as rendering is done server-side.

## Step 2: Call R code and manage state

Existing R code is mapped into the `Session` class on the remote client-side.
All functions preceding the `start_vrpc_agent` line are available as static
functions of the `Session` class and as **member functions** of `Session` class
instances.

> **NOTE**
>
> Additionally, the generic `call` function is injected (both statically and as
> member) which allows calling any other R code.

Within a `Session` instance all state is maintained, allowing client code like
this:

```js
const { VrpcRemote } = require('vrpc')

if (process.argv.length < 3) {
  console.log('Usage: node index.js <agentName>')
  process.exit(1)
}

(async () => {
  const client = new VrpcRemote({
    domain: 'public.vrpc',
    agent: process.argv[2]
  })
  await client.connect()
  const proxy = await client.create({
    className: 'Session',
    instance: 'session1'
  })
  await proxy.select_dataset('rock')
  await proxy.get_table(1) // first line of rock dataset

  await proxy.select_dataset('cars')
  await proxy.get_table(2) // first two lines of cars dataset
  await proxy.get_summary() // summary information of cats dataset
})()
```
