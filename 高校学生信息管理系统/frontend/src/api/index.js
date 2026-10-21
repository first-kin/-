import request from './request.js'
import { useAuthStore } from '../stores/auth.js'

const mock = import.meta.env?.VITE_USE_MOCK !== 'false'

let courses = [
  { id: 1, code: 'SE201', name: '数据结构', teacher: '王老师', credit: 4, term: '2026-2027-1', time: '周一 1-2 节', room: '教 2-301', status: '开课中' },
  { id: 2, code: 'SE202', name: '面向对象程序设计', teacher: '李老师', credit: 3.5, term: '2026-2027-1', time: '周二 3-4 节', room: '实验楼 A204', status: '开课中' },
  { id: 3, code: 'MA103', name: '高等数学（上）', teacher: '陈老师', credit: 5, term: '2026-2027-1', time: '周三 1-2 节', room: '教 1-105', status: '开课中' },
  { id: 4, code: 'EN101', name: '大学英语', teacher: '赵老师', credit: 2, term: '2026-2027-1', time: '周五 5-6 节', room: '教 3-210', status: '已结课' },
]

let scores = [
  { id: 1, courseCode: 'SE201', courseName: '数据结构', teacher: '王老师', term: '2026-2027-1', usual: 92, exam: 88, total: 90, credit: 4, status: '已发布' },
  { id: 2, courseCode: 'SE202', courseName: '面向对象程序设计', teacher: '李老师', term: '2026-2027-1', usual: 90, exam: 94, total: 92, credit: 3.5, status: '已发布' },
  { id: 3, courseCode: 'MA103', courseName: '高等数学（上）', teacher: '陈老师', term: '2026-2027-1', usual: 82, exam: 79, total: 80, credit: 5, status: '已发布' },
  { id: 4, courseCode: 'EN101', courseName: '大学英语', teacher: '赵老师', term: '2026-2027-1', usual: 95, exam: 91, total: 93, credit: 2, status: '已发布' },
  { id: 5, courseCode: 'PH105', courseName: '大学物理', teacher: '孙老师', term: '2026-2027-1', usual: 78, exam: 72, total: 74, credit: 3, status: '已发布' },
]

let profile = {
  name: '夏同学', studentNo: '2025030227', gender: '男', department: '计算机学院',
  major: '软件工程', className: '软件2505班', grade: '2025级', phone: '138****2027',
  email: 'student@example.edu.cn', address: '天津市', username: '2025030227',
}

const clone = (value) => JSON.parse(JSON.stringify(value))
const wait = (data) => Promise.resolve(clone(data))
const listFrom = (data) => Array.isArray(data) ? data : (data?.list || [])

// 当前学期（与页面顶部的学年学期标签一致）
const CURRENT_TERM = '2026-2027-1'

// 后端课程表没有状态字段，按学期推算：当前学期=开课中，早于当前=已结课，晚于当前=未开课
function courseStatus(term, rawStatus) {
  if (rawStatus && rawStatus !== '未知') return rawStatus
  if (!term) return '未排课'
  if (term === CURRENT_TERM) return '开课中'
  return term < CURRENT_TERM ? '已结课' : '未开课'
}

function normalizeCourse(item) {
  const term = item.term || item.semester || ''
  return {
    ...item,
    code: item.code || item.courseCode || '',
    name: item.name || item.courseName || '',
    teacher: item.teacher || item.teacherName || '',
    term,
    time: item.time || item.courseTime || '',
    room: item.room || '',
    status: courseStatus(term, item.status),
  }
}

function normalizeScore(item) {
  const total = item.total ?? item.totalScore ?? item.score ?? 0
  const status = typeof item.status === 'number'
    ? ({ 0: '已发布', 1: '缺考', 2: '缓考' }[item.status] || '未录入')
    : (item.status || '已发布')

  return {
    ...item,
    courseCode: item.courseCode || '',
    courseName: item.courseName || '',
    teacher: item.teacher || item.teacherName || '',
    term: item.term || item.semester || '',
    usual: item.usual ?? item.usualScore ?? null,
    exam: item.exam ?? item.examScore ?? item.score ?? null,
    total,
    credit: Number(item.credit || 0),
    status,
  }
}

function coursePayload(data) {
  return {
    courseName: data.name ?? data.courseName,
    credit: data.credit,
    teacherId: data.teacherId ?? null,
    semester: data.term ?? data.semester,
    courseTime: data.time ?? data.courseTime ?? '',
    room: data.room ?? '',
  }
}

export const api = {
  courses: async (params = {}) => {
    if (mock) {
      return wait(courses
        .filter((course) => !params.keyword ||
          `${course.name}${course.code}${course.teacher}`.toLowerCase().includes(params.keyword.toLowerCase()))
        .map(normalizeCourse))
    }
    const result = await request.get('/courses', { params })
    return listFrom(result).map(normalizeCourse)
  },

  addCourse: async (data) => {
    if (!mock) return request.post('/courses', coursePayload(data))
    const item = { ...data, id: Date.now(), status: '开课中' }
    courses.unshift(item)
    return clone(normalizeCourse(item))
  },

  updateCourse: async (data) => {
    if (!mock) return request.put(`/courses/${data.id}`, coursePayload(data))
    courses = courses.map((course) => course.id === data.id ? { ...course, ...data } : course)
    return clone(data)
  },

  deleteCourse: async (id) => {
    if (!mock) return request.delete(`/courses/${id}`)
    courses = courses.filter((course) => course.id !== id)
    return true
  },

  scores: async (params = {}) => {
    if (mock) {
      return wait(scores
        .filter((score) => (!params.keyword || `${score.courseName}${score.courseCode}`.toLowerCase().includes(params.keyword.toLowerCase())) &&
          (!params.term || score.term === params.term))
        .map(normalizeScore))
    }

    const role = useAuthStore().role
    const result = role === 'student'
      ? await request.get('/scores/my', { params: { semester: params.semester || params.term } })
      : await request.get('/scores', { params })
    return listFrom(result).map(normalizeScore)
  },

  profile: async () => mock ? wait(profile) : request.get('/users/me'),

  updateProfile: async (data) => {
    if (mock) {
      profile = { ...profile, ...data }
      return clone(profile)
    }
    await request.put('/users/me', {
      phone: data.phone,
      email: data.email,
      address: data.address,
    })
    return { ...data }
  },

  changePassword: async (data) => {
    if (!mock) return request.post('/users/me/password', {
      oldPassword: data.oldPassword,
      newPassword: data.newPassword,
    })
    if (data.oldPassword !== '123456') throw new Error('原密码不正确（演示账号原密码为 123456）')
    if (data.newPassword.length < 6) throw new Error('新密码至少 6 位')
    return true
  },
}
