import assert from 'node:assert/strict'
import { after, test } from 'node:test'
import { createPinia, setActivePinia } from 'pinia'
import { homeForRole, useAuthStore } from '../src/stores/auth.js'

const originalWindow = globalThis.window
after(() => {
  if (originalWindow === undefined) delete globalThis.window
  else globalThis.window = originalWindow
})

function setup(initial = {}) {
  const data = new Map(Object.entries(initial))
  const localStorage = {
    getItem: (key) => data.has(key) ? data.get(key) : null,
    setItem: (key, value) => data.set(key, String(value)),
    removeItem: (key) => data.delete(key),
  }
  globalThis.window = { localStorage }
  setActivePinia(createPinia())
  return { localStorage, auth: useAuthStore() }
}

test('保存登录状态后可以从浏览器存储恢复', () => {
  const { auth, localStorage } = setup()
  auth.setSession({ token: 'test-token', role: 'teacher', name: '王老师', expiresIn: 60 })

  assert.equal(localStorage.getItem('token'), 'test-token')
  assert.equal(localStorage.getItem('role'), 'teacher')
  assert.equal(homeForRole('teacher'), '/teacher')

  setActivePinia(createPinia())
  const restored = useAuthStore()
  assert.equal(restored.hydrate(), true)
  assert.equal(restored.name, '王老师')
  assert.equal(restored.role, 'teacher')
})

test('过期的登录状态会被清除', () => {
  const { auth, localStorage } = setup({
    token: 'expired-token',
    role: 'student',
    expiresAt: String(Date.now() - 1000),
  })

  assert.equal(auth.hydrate(), false)
  assert.equal(auth.token, '')
  assert.equal(localStorage.getItem('token'), null)
  assert.equal(localStorage.getItem('expiresAt'), null)
})

test('无效角色和不完整的登录状态不会获得访问权限', () => {
  const { auth, localStorage } = setup({ role: 'admin' })

  assert.equal(auth.hydrate(), false)
  assert.equal(localStorage.getItem('role'), null)
  assert.throws(() => auth.setSession({ token: 'test-token', role: 'other' }), /凭证或角色/)
  assert.equal(auth.isAuthenticated, false)
})
