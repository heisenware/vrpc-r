const { VrpcRemote } = require('vrpc')

if (process.argv.length < 3) {
  console.log('Usage: node index.js <agentName>')
  process.exit(1)
}

(async () => {
  const client = new VrpcRemote({ domain: 'public.vrpc', agent: process.argv[2] })
  await client.connect()
  console.log(await client.callStatic({
    className: 'Session',
    functionName: 'greet',
    args: ['world!']
  }))
})()
