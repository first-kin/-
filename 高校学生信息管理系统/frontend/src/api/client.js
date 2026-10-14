import axios from 'axios'
export const http = axios.create({ baseURL: import.meta.env.VITE_API_BASE_URL || '/api', timeout: 10000 })
http.interceptors.request.use(config => {
  const token = localStorage.getItem('token')
  if (token) config.headers.Authorization = `Bearer ${token}`
  return config
})
http.interceptors.response.use(res => res.data, error => {
    if (error.response?.status === 401) {
    localStorage.removeItem('token')
localStorage.removeItem('role')
    if (window.location.pathname !== '/login') {
      window.location.replace('/login')
    }
  }
  const message = error?.response?.data?.msg || error.message || '请求失败'
  return Promise.reject(new Error(message))
})
