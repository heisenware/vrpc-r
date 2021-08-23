'use strict'

/* global describe, context, before, after, it */
const { VrpcRemote } = require('vrpc')
const assert = require('assert')

describe('VRPC R-Agent', () => {
  let client
  before(async () => {
    client = new VrpcRemote({
      broker: 'mqtt://broker:1883',
      domain: 'test',
      agent: 'agent1'
    })
  })
  describe('Availability Listings', () => {
    it('should show all registered R functionality', async () => {
      const expectedClasses = new Set(['Session'])
      // Give some time to accumulate class info messages
      client.on('class', ({ className, staticFunctions, memberFunctions }) => {
        console.log(className, staticFunctions, memberFunctions)
        if (expectedClasses.has(className)) {
          expectedClasses.delete(className)
          if (className === 'Session') {
            assert(memberFunctions.includes('test_sys_sleep'))
          }
        }
      })
      await client.connect()
      while (expectedClasses.size > 0) {
        await new Promise(resolve => setTimeout(resolve, 100))
      }
    })
  })
  describe('Static calls', () => {
    it('should execute a call to stats::rnorm(10)', async () => {
      const ret = await client.callStatic({
        className: 'Session',
        functionName: 'call',
        args: ['stats::rnorm', 10]
      })
      assert(Array.isArray(ret))
      assert(ret.length === 10)
      assert(typeof ret[0] === 'number')
    })
    it.skip('should execute a call to app::run_long()', async () => {
      const promise = client.callStatic({
        className: 'app',
        functionName: 'run_long',
        args: [{ s: 2 }]
      })
      const ret2 = await client.callStatic({
        className: 'stats',
        functionName: 'rnorm',
        args: [{ n: 100 }]
      })
      assert(Array.isArray(ret2))
      assert(ret2.length === 100)
      const ret1 = await promise
      assert.deepStrictEqual(ret1, [2])
    })
  })
})
