import request from './request.js'

const demoMode = import.meta.env?.DEV &&
  import.meta.env?.VITE_MOCK_AUTH === 'true' &&
  import.meta.env?.VITE_USE_MOCK !== 'false'

const demoUsers = {
  'demo-admin': { role: 'admin', name: '演示管理员' },
  'demo-teacher': { role: 'teacher', name: '演示教师' },
  'demo-student': { role: 'student', name: '演示学生' },
}

export function login(username, password) {
  if (demoMode) {
    const user = demoUsers[username]
    if (!user || password !== '123456') {
      return Promise.reject(new Error('演示账号或密码错误'))
    }
    return Promise.resolve({
      token: `demo-${user.role}-${Date.now()}`,
      role: user.role,
      name: user.name,
      expiresIn: 86400,
    })
  }
  return request.post('/auth/login', { username, password })
}

export function logout(token) {
  if (demoMode) return Promise.resolve(null)
  return request.post('/auth/logout', null, {
    headers: token ? { Authorization: `Bearer ${token}` } : undefined,
    skipAuthRedirect: true,
  })
}
