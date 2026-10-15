import axios from 'axios'
import { useAuthStore } from '../stores/auth.js'

const request = axios.create({
  baseURL: import.meta.env?.VITE_API_BASE_URL || '/api',
  timeout: 10000,
  headers: { 'Content-Type': 'application/json' },
})

function authStoreOrNull() {
  try {
    return useAuthStore()
  } catch {
    return null
  }
}

function clearAuthentication() {
  authStoreOrNull()?.clearSession()
  if (typeof window !== 'undefined') {
    for (const key of ['token', 'role', 'name', 'expiresAt']) {
      window.localStorage.removeItem(key)
    }
  }
}

function redirectToLogin() {
  if (typeof window !== 'undefined' && window.location.pathname !== '/login') {
    window.location.replace('/login')
  }
}

function businessError(result) {
  const error = new Error(result?.msg || '请求失败')
  error.businessCode = result?.code
  error.data = result?.data
  return error
}

request.interceptors.request.use((config) => {
  const token = authStoreOrNull()?.token ||
    (typeof window !== 'undefined' ? window.localStorage.getItem('token') : '')
  if (token) config.headers.Authorization = `Bearer ${token}`
  return config
})

request.interceptors.response.use((response) => {
  const result = response.data
  if (response.config.responseType === 'blob' || response.config.responseType === 'arraybuffer') {
    return result
  }
  if (!result || typeof result.code !== 'number') {
    return Promise.reject(new Error('接口响应格式不正确'))
  }

  if (result.code !== 0) {
    if ((result.code === 1003 || result.code === 401) && !response.config.skipAuthRedirect) {
      clearAuthentication()
      redirectToLogin()
    }
    return Promise.reject(businessError(result))
  }
  return result.data
}, (error) => {
  if (!error.config?.skipAuthRedirect &&
      (error.response?.status === 401 ||
       error.response?.data?.code === 1003 ||
       error.response?.data?.code === 401)) {
    clearAuthentication()
    redirectToLogin()
  }
  error.message = error.response?.data?.msg || error.message || '请求失败'
  return Promise.reject(error)
})

export default request
