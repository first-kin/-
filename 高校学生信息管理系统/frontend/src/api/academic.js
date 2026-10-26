import request from './request.js'
import { useAuthStore } from '../stores/auth.js'
import { getActivePinia } from 'pinia'

const mock = import.meta.env?.VITE_USE_MOCK !== 'false'

let courses = [
  { id: 1, courseName: '数据结构', credit: 4, teacherId: 1, teacherName: '王老师', semester: '2026-2027-1', courseTime: '周一 1-2 节' },
  { id: 2, courseName: '面向对象程序设计', credit: 3.5, teacherId: 2, teacherName: '李老师', semester: '2026-2027-1', courseTime: '周二 3-4 节' },
  { id: 3, courseName: '高等数学（上）', credit: 5, teacherId: 3, teacherName: '陈老师', semester: '2026-2027-1', courseTime: '周三 1-2 节' },
]
let scores = [
  { id: 1, courseId: 1, courseName: '数据结构', studentId: 6, studentNo: '2025030227', studentName: '夏同学', score: 90, status: 0, examTime: '2026-10-12' },
  { id: 2, courseId: 1, courseName: '数据结构', studentId: 7, studentNo: '2025030228', studentName: '李小明', score: 82, status: 0, examTime: '2026-10-12' },
  { id: 3, courseId: 2, courseName: '面向对象程序设计', studentId: 6, studentNo: '2025030227', studentName: '夏同学', score: 92, status: 0, examTime: '2026-10-13' },
]
let profile = { id: 6, username: '2025030227', role: 'student', name: '夏同学', studentNo: '2025030227', department: '计算机学院', major: '软件工程', className: '软件2505班', grade: '2025级', phone: '13800002027', email: 'student@example.edu.cn', address: '天津市' }

const clone = (value) => JSON.parse(JSON.stringify(value))
const listFrom = (value) => Array.isArray(value) ? value : (value?.list || [])
const pageOf = (items, params = {}) => {
  const page = Math.max(1, Number(params.page) || 1)
  const size = Math.min(100, Math.max(1, Number(params.size) || 10))
  return { list: clone(items.slice((page - 1) * size, page * size)), total: items.length, page, size }
}
const match = (item, keyword, fields) => !keyword || fields.some((field) => String(item[field] || '').toLowerCase().includes(String(keyword).toLowerCase()))
const activeRole = () => getActivePinia() ? useAuthStore().role : ''
const teacherOwnsCourse = (courseId) => {
  const role = activeRole()
  if (!role || role === 'admin') return true
  return role === 'teacher' && courses.some((course) => course.id === Number(courseId) && course.teacherName === useAuthStore().name)
}

export const academicApi = {
  courses: async (params = {}) => {
    if (!mock) return request.get('/courses', { params })
    const role = activeRole()
    const filtered = courses.filter((item) => (role !== 'teacher' || teacherOwnsCourse(item.id)) && match(item, params.keyword, ['courseName', 'teacherName']) && (!params.semester || item.semester === params.semester))
    return pageOf(filtered, params)
  },
  createCourse: async (data) => {
    if (!mock) return request.post('/courses', data)
    const item = { ...data, id: Math.max(0, ...courses.map((course) => course.id)) + 1, teacherName: ({ 1: '王老师', 2: '李老师', 3: '陈老师' })[data.teacherId] || '' }
    courses = [item, ...courses]
    return clone(item)
  },
  updateCourse: async (id, data) => {
    if (!mock) return request.put(`/courses/${id}`, data)
    courses = courses.map((item) => item.id === Number(id) ? { ...item, ...data, id: Number(id) } : item)
    return clone(courses.find((item) => item.id === Number(id)))
  },
  deleteCourse: async (id) => {
    if (!mock) return request.delete(`/courses/${id}`)
    if (scores.some((item) => item.courseId === Number(id))) throw new Error('该课程已有成绩记录，不能删除')
    courses = courses.filter((item) => item.id !== Number(id))
  },
  scores: async (params = {}) => {
    const role = activeRole()
    if (role && !['admin', 'teacher'].includes(role)) throw new Error('无权查看成绩')
    if (!mock) {
      const query = { ...params, semester: params.semester || undefined }
      const result = role === 'student' ? await request.get('/scores/my', { params: query }) : await request.get('/scores', { params: query })
      return Array.isArray(result) ? { list: result, total: result.length } : result
    }
    const filtered = scores.filter((item) => (role !== 'teacher' || teacherOwnsCourse(item.courseId)) && (!params.courseId || item.courseId === Number(params.courseId)) && (!params.studentId || item.studentId === Number(params.studentId)) && (!params.semester || courses.find((course) => course.id === item.courseId)?.semester === params.semester) && match(item, params.keyword, ['courseName', 'studentName', 'studentNo']))
    return pageOf(filtered, params)
  },
  saveScore: async (data) => {
    const role = activeRole()
    if (role && !['admin', 'teacher'].includes(role)) throw new Error('无权录入成绩')
    if (!mock) return request.post('/scores', data)
    if (Number(data.status) === 0 && (data.score == null || Number(data.score) < 0 || Number(data.score) > 100)) throw new Error('成绩需在 0~100 之间')
    const course = courses.find((item) => item.id === Number(data.courseId))
    if (!course || !teacherOwnsCourse(data.courseId)) throw new Error('无权维护该课程的成绩')
    const existing = scores.find((score) => score.courseId === Number(data.courseId) && score.studentId === Number(data.studentId))
    const item = { ...data, id: existing?.id || Math.max(0, ...scores.map((score) => score.id)) + 1, courseId: Number(data.courseId), courseName: course.courseName, studentId: Number(data.studentId), score: Number(data.status) === 0 ? Number(data.score) : (data.score ?? null) }
    scores = existing
      ? scores.map((score) => score.id === existing.id ? item : score)
      : [item, ...scores]
    return clone(item)
  },
  deleteScore: async (id) => {
    const role = activeRole()
    if (role && !['admin', 'teacher'].includes(role)) throw new Error('无权删除成绩')
    if (!mock) return request.delete(`/scores/${id}`)
    const item = scores.find((score) => score.id === Number(id))
    if (!item) throw new Error('成绩记录不存在')
    if (!teacherOwnsCourse(item.courseId)) throw new Error('无权删除该课程的成绩')
    scores = scores.filter((score) => score.id !== Number(id))
  },
  statistics: async (courseId) => {
    const role = activeRole()
    if (role && !['admin', 'teacher'].includes(role)) throw new Error('无权查看成绩统计')
    if (!mock) return request.get('/scores/statistics', { params: { courseId } })
    if (!teacherOwnsCourse(courseId)) throw new Error('无权查看该课程成绩统计')
    const values = scores.filter((item) => item.courseId === Number(courseId) && item.status === 0 && item.score != null).map((item) => Number(item.score))
    const count = (test) => values.filter(test).length
    return { courseId: Number(courseId), courseName: courses.find((item) => item.id === Number(courseId))?.courseName || '', avgScore: values.length ? values.reduce((a, b) => a + b, 0) / values.length : 0, maxScore: values.length ? Math.max(...values) : 0, minScore: values.length ? Math.min(...values) : 0, passRate: values.length ? count((score) => score >= 60) / values.length : 0, distribution: [{ segment: '90-100', count: count((score) => score >= 90) }, { segment: '80-89', count: count((score) => score >= 80 && score < 90) }, { segment: '70-79', count: count((score) => score >= 70 && score < 80) }, { segment: '60-69', count: count((score) => score >= 60 && score < 70) }, { segment: '<60', count: count((score) => score < 60) }] }
  },
  profile: async () => mock ? clone(profile) : request.get('/users/me'),
  updateProfile: async (data) => {
    if (!mock) return request.put('/users/me', { phone: data.phone, email: data.email, address: data.address })
    profile = { ...profile, phone: data.phone, email: data.email, address: data.address }
    return clone(profile)
  },
  changePassword: async (data) => {
    if (!mock) return request.post('/users/me/password', { oldPassword: data.oldPassword, newPassword: data.newPassword })
    if (data.oldPassword !== '123456') throw new Error('原密码不正确（演示账号原密码为 123456）')
    if (String(data.newPassword).length < 6) throw new Error('新密码至少 6 位')
    return true
  },
}

export const academicPageOf = pageOf
export const academicListFrom = listFrom
