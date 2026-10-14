import axios from 'axios'

const request = axios.create({
  baseURL: '/api',
  timeout: 10000,
})
request.interceptors.request.use((config) => {
  const token = localStorage.getItem('token')
  if (token) {
    config.headers.Authorization = `Bearer ${token}`
  }
  return config
})
request.interceptors.response.use((response) => {
  const result = response.data

  if (result.code !== 0) {
    return Promise.reject(new Error(result.msg || '请求失败'))
  }
  return result.data
}, (error) => {
  if (error.response?.status === 401) {
    localStorage.removeItem('token')

    if (window.location.pathname !== '/login') {
      window.location.replace('/login')
    }
  }

  return Promise.reject(error)
})
export default request
