import assert from 'node:assert/strict'
import { after, test } from 'node:test'
import { createPinia, setActivePinia } from 'pinia'
import request from '../src/api/request.js'
import { useAuthStore } from '../src/stores/auth.js'

const originalWindow = globalThis.window
after(() => {
  if (originalWindow === undefined) delete globalThis.window
  else globalThis.window = originalWindow
})

function setup() {
  const data = new Map()
  const redirects = []
  globalThis.window = {
    localStorage: {
      getItem: (key) => data.get(key) ?? null,
      setItem: (key, value) => data.set(key, String(value)),
      removeItem: (key) => data.delete(key),
    },
    location: { pathname: '/student', replace: (path) => redirects.push(path) },
  }
  setActivePinia(createPinia())
  const auth = useAuthStore()
  auth.setSession({ token: 'test-token', role: 'student' })
  return { auth, data, redirects }
}

function reply(code, data = null) {
  return async (config) => ({
    data: { code, msg: code === 0 ? '成功' : '未登录', data },
    status: 200,
    statusText: 'OK',
    headers: {},
    config,
  })
}

test('请求携带 Token，成功响应只返回业务数据', async () => {
  setup()
  let authorization = ''
  const data = await request.get('/courses', {
    adapter: async (config) => {
      authorization = config.headers.Authorization
      return reply(0, { list: [1] })(config)
    },
  })

  assert.equal(authorization, 'Bearer test-token')
  assert.deepEqual(data, { list: [1] })
})

for (const code of [401, 1003]) {
  test(`业务未登录错误 ${code} 会清除 Token 并跳转登录页`, async () => {
    const { auth, data, redirects } = setup()
    await assert.rejects(request.get('/courses', { adapter: reply(code) }), /未登录/)

    assert.equal(auth.isAuthenticated, false)
    assert.equal(data.has('token'), false)
    assert.deepEqual(redirects, ['/login'])
  })
}
