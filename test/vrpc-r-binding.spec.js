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
  describe('Auto Discovery', () => {
    it('should correctly produce all discovery information', async () => {
      const agentInfo = new Promise(resolve =>
        client.on('agent', ({ status, hostname, version }) => {
          assert.strictEqual(status, 'online')
          assert.strictEqual(hostname, 'agent1')
          assert.strictEqual(version, '')
          resolve()
        })
      )
      const classInfo = new Promise(resolve =>
        client.on(
          'class',
          ({ className, staticFunctions, memberFunctions }) => {
            if (className !== 'Session') return
            for (const x of [staticFunctions, memberFunctions]) {
              assert(x.includes('select_dataset'))
              assert(x.includes('get_table'))
              assert(x.includes('test_sys_sleep'))
              assert(x.includes('test_foreign_package'))
              assert(x.includes('test_plot'))
              assert(x.includes('call'))
            }
            resolve()
          }
        )
      )
      await client.connect()
      await agentInfo
      await classInfo
    })
  })
  describe('Static calls', () => {
    it('should report error on invalid call', async () => {
      await assert.rejects(
        async () =>
          client.callStatic({
            className: 'Session',
            functionName: 'call',
            args: ['does_not_exist']
          }),
        err => {
          assert.strictEqual(
            err.message,
            'could not find function "does_not_exist"'
          )
          return true
        }
      )
    })
    it('should report error on invalid arguments', async () => {
      await assert.rejects(
        async () =>
          client.callStatic({
            className: 'Session',
            functionName: 'call',
            args: ['rnorm']
          }),
        err => {
          assert.strictEqual(
            err.message,
            'argument "n" is missing, with no default'
          )
          return true
        }
      )
    })
    it('should execute a generic (namespaced) call to stats::rnorm(10)', async () => {
      const ret = await client.callStatic({
        className: 'Session',
        functionName: 'call',
        args: ['stats::rnorm', 10]
      })
      assert(Array.isArray(ret))
      assert(ret.length === 10)
      assert(typeof ret[0] === 'number')
    })
    it('should execute a generic (non-namespaced) call to rnorm(10)', async () => {
      const ret = await client.callStatic({
        className: 'Session',
        functionName: 'call',
        args: ['rnorm', 10]
      })
      assert(Array.isArray(ret))
      assert(ret.length === 10)
      assert(typeof ret[0] === 'number')
    })
    it('should execute a generic call to plot(c(1,2), c(3,4))', async () => {
      const ret = await client.callStatic({
        className: 'Session',
        functionName: 'call',
        args: ['plot', [1, 2], [3, 4]]
      })
      assert.strictEqual(typeof ret, 'string')
      const svg = Buffer.from(ret, 'base64').toString()
      assert(svg.includes("<svg xmlns='http://www.w3.org/2000/svg'"))
    })
    it('should execute specific calls to registered functions', async () => {
      const ret = await client.callStatic({
        className: 'Session',
        functionName: 'test_sys_sleep',
        args: []
      })
      assert.strictEqual(ret, 1)
    })
    it('should execute calls in parallel', async () => {
      const call1 = client.callStatic({
        className: 'Session',
        functionName: 'test_sys_sleep',
        args: [0.8]
      })
      const call2 = client.callStatic({
        className: 'Session',
        functionName: 'test_sys_sleep',
        args: [0.7]
      })
      const start = Date.now()
      const ret = await Promise.all([call1, call2])
      const duration = Date.now() - start
      assert(duration < 1000)
      assert.deepStrictEqual(ret, [0.8, 0.7])
    })
    it('should support calling of external package functionality', async () => {
      const ret = await client.callStatic({
        className: 'Session',
        functionName: 'test_foreign_package',
        args: []
      })
      assert.strictEqual(typeof ret, 'object')
      assert.strictEqual(
        ret['$schema'],
        'https://vega.github.io/schema/vega-lite/v4.json'
      )
    })
  })
  describe('Member function calls', () => {
    let proxy1
    let proxy2
    it('should allow to create named instances', async () => {
      proxy1 = await client.create({
        className: 'Session',
        instance: 'session1'
      })
      proxy2 = await client.create({
        className: 'Session',
        instance: 'session2'
      })
      assert.strictEqual(typeof proxy1, 'object')
      assert.strictEqual(typeof proxy2, 'object')
    })
    it('should allow generic calls on that instance', async () => {
      const ret = await proxy1.call('c', -2, -1, 0, 2, 6)
      assert(Array.isArray(ret))
      assert.strictEqual(ret.length, 5)
      assert.strictEqual(ret[0], -2)
    })
    it('should allow re-using the returned object', async () => {
      const mean = await proxy1.call('mean', '$c')
      assert.strictEqual(mean, 1)
      const sum = await proxy1.call('sum', '$c')
      assert.strictEqual(sum, 5)
      const vec = await proxy1.call('c', '$mean', '$sum')
      assert.deepStrictEqual(vec, [1, 5])
    })
    it('should keep state of global variables', async () => {
      assert(await proxy1.select_dataset('rock'))
      const rockRow = await proxy1.get_table(1)
      assert.deepStrictEqual(rockRow, [
        { area: 4990, peri: 2791.9, perm: 6.3, shape: 0.0903 }
      ])
      assert(await proxy1.select_dataset('cars'))
      const carRows = await proxy1.get_table(2)
      assert.deepStrictEqual(carRows, [
        { dist: 2, speed: 4 },
        { dist: 10, speed: 4 }
      ])
    })
    it('should cleanly separate state of different instances', async () => {
      assert(await proxy1.select_dataset('pressure'))
      const pressureRow = await proxy1.get_table(1)
      assert(await proxy2.select_dataset('cars'))
      const carRow = await proxy2.get_table(1)
      assert.deepStrictEqual(pressureRow, [
        { pressure: 0.0002, temperature: 0 }
      ])
      assert.deepStrictEqual(carRow, [{ dist: 2, speed: 4 }])
    })
    it('should delete proxies', async () => {
      const ret = await client.delete('session1')
      assert.strictEqual(ret, true)
      assert.rejects(async () => await proxy1.get_table(1))
    })
  })
})
