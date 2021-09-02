# Example 1 - "Hello World"

This - obviously - is the most basic example. We are calling a function in R
and receive its return value.

## Step 1

Let's pretend we wanted to remotely call this very simple R code:

```R
greet <- function(whom) {
  paste("Hello,", whom)
}
```

Then all we have to do is to load the `vrpc` library and add a single line:


*app.R*

```R
library("vrpc")

greet <- function(whom) {
  paste("Hello,", whom)
}

start_vrpc_agent(domain = "public.vrpc")
```

When you run this code, using e.g.

```bash
Rscript app.R
```

a VRPC agent will be created which connects to the vrpc.io cloud and
keeps listening for incoming instructions under the public and free domain
"public.vrpc".

## Step 2

Having started the agent, we can now call the code from any remote location
using any VRPC-supported programming language.

Let's see how that would work with Javascript (Node.js):

*index.js*

```js
const { VrpcRemote } = require('vrpc')

if (process.argv.length < 3) {
  console.log('Usage: node index.js <agentName>')
  process.exit(1)
}

// retrieve agent name from command line
const agent = process.argv[2]

async function main () {
  // create and connect VRPC client
  const client = new VrpcRemote({ agent, domain: 'public.vrpc' })
  await client.connect()

  // call the R function as static function of the Session class
  const ret = await client.callStatic({
    className: 'Session',
    functionName: 'greet',
    args: ['world!']
  })

  // print the result
  console.log(ret) // Hello, world!
}

main()
```

> **IMPORTANT**
>
> Any R code that is remotely available will be encapsulated within a `Session`
> class. You may create (remote-) instances of that class if you want need
> stateful interaction with the R code. Otherwise, you can simply call the
> functions statically.

To see how it works, you need to first install the required Node.js packages:

```bash
npm install
```

And then execute:


```bash
node index.js <yourAgent>
```

using the correct agent name as displayed after having started the R code.
