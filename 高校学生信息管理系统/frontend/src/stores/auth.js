import { computed, ref } from 'vue'
import { defineStore } from 'pinia'

const keys = ['token', 'role', 'name', 'expiresAt']
const homes = { admin: '/admin', teacher: '/teacher', student: '/student' }

function storage() {
  return typeof window === 'undefined' ? null : window.localStorage
}

export function homeForRole(role) {
  return homes[role] || '/login'
}

export const useAuthStore = defineStore('auth', () => {
  const token = ref('')
  const role = ref('')
  const name = ref('')
  const expiresAt = ref(0)

  const isAuthenticated = computed(() => Boolean(token.value && homes[role.value]))

  function clearSession() {
    token.value = ''
    role.value = ''
    name.value = ''
    expiresAt.value = 0
    keys.forEach((key) => storage()?.removeItem(key))
  }

  function hydrate() {
    const saved = storage()
    const hasSavedData = keys.some((key) => saved?.getItem(key) !== null)
    const savedExpiry = saved?.getItem('expiresAt')
    token.value = saved?.getItem('token') || ''
    role.value = saved?.getItem('role') || ''
    name.value = saved?.getItem('name') || ''
    expiresAt.value = Number(savedExpiry || 0)

    if (hasSavedData && (!isAuthenticated.value ||
        (savedExpiry && !Number.isFinite(expiresAt.value)) ||
        (expiresAt.value && expiresAt.value <= Date.now()) ||
        !homes[role.value])) {
      clearSession()
    }
    return isAuthenticated.value
  }

  function setSession(session) {
    if (typeof session?.token !== 'string' || !session.token.trim() || !homes[session.role]) {
      throw new Error('登录响应缺少有效的凭证或角色')
    }

    token.value = session.token
    role.value = session.role
    name.value = session.name || ''
    expiresAt.value = Number(session.expiresIn) > 0
      ? Date.now() + Number(session.expiresIn) * 1000
      : 0

    const saved = storage()
    saved?.setItem('token', token.value)
    saved?.setItem('role', role.value)
    if (name.value) saved?.setItem('name', name.value)
    else saved?.removeItem('name')
    if (expiresAt.value) saved?.setItem('expiresAt', String(expiresAt.value))
    else saved?.removeItem('expiresAt')
  }

  return { token, role, name, expiresAt, isAuthenticated, hydrate, setSession, clearSession }
})
