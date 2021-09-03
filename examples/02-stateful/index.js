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
  proxy.select_dataset('rock')
  proxy.get_table(1) // first line of rock dataset

  proxy.select_dataset('cars')
  proxy.get_table(2) // first two lines of cars dataset
  proxy.get_summary() // summary information of cats dataset
})()
