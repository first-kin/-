import request from './request.js'
import { academicApi } from './academic.js'

const mock = import.meta.env?.VITE_USE_MOCK !== 'false'

let departments = [
  { id: 1, deptName: '计算机学院' },
  { id: 2, deptName: '数学学院' },
  { id: 3, deptName: '外国语学院' },
]

let majors = [
  { id: 1, majorName: '计算机科学与技术', deptId: 1, deptName: '计算机学院' },
  { id: 3, majorName: '软件工程', deptId: 1, deptName: '计算机学院' },
  { id: 5, majorName: '数学与应用数学', deptId: 2, deptName: '数学学院' },
]

let teachers = [
  { id: 1, teacherNo: 'T2019013', name: '王老师', gender: '男', title: '副教授', deptId: 1, deptName: '计算机学院', phone: '13700002013', email: 'wang@example.edu.cn' },
  { id: 2, teacherNo: 'T2020021', name: '李老师', gender: '女', title: '讲师', deptId: 1, deptName: '计算机学院', phone: '13800002021', email: 'li@example.edu.cn' },
  { id: 3, teacherNo: 'T2018018', name: '陈老师', gender: '男', title: '教授', deptId: 2, deptName: '数学学院', phone: '13900002018', email: 'chen@example.edu.cn' },
  { id: 4, teacherNo: 'T2021032', name: '赵老师', gender: '女', title: '助教', deptId: 3, deptName: '外国语学院', phone: '13600002032', email: 'zhao@example.edu.cn' },
]

let classes = [
  { id: 1, className: '软件2505班', majorId: 3, majorName: '软件工程', grade: 2025, headTeacherId: 1 },
  { id: 2, className: '计科2401班', majorId: 1, majorName: '计算机科学与技术', grade: 2024, headTeacherId: 2 },
  { id: 3, className: '数学2501班', majorId: 5, majorName: '数学与应用数学', grade: 2025, headTeacherId: 3 },
]

let students = [
  { id: 1, studentNo: '2025030227', name: '夏同学', gender: '男', birthDate: '2007-03-12', classId: 1, className: '软件2505班', phone: '13800002027', email: 'xia@example.edu.cn', address: '天津市' },
  { id: 2, studentNo: '2025030228', name: '李小明', gender: '男', birthDate: '2007-05-01', classId: 1, className: '软件2505班', phone: '13900001234', email: 'liming@example.edu.cn', address: '' },
  { id: 3, studentNo: '2024030101', name: '周同学', gender: '女', birthDate: '2006-08-19', classId: 2, className: '计科2401班', phone: '13700001001', email: 'zhou@example.edu.cn', address: '' },
  { id: 4, studentNo: '2025030501', name: '吴同学', gender: '女', birthDate: '2007-11-02', classId: 3, className: '数学2501班', phone: '13600005001', email: 'wu@example.edu.cn', address: '' },
]
const studentCourseSelections = new Map()

const clone = (value) => JSON.parse(JSON.stringify(value))
const listFrom = (data) => Array.isArray(data) ? data : (data?.list || [])
const assertUnique = (items, field, value, id, message) => {
  if (items.some((item) => item[field] === value && item.id !== id)) throw new Error(message)
}
const pageResult = (items, page = 1, size = 10) => ({
  list: clone(items.slice((page - 1) * size, page * size)),
  total: items.length,
})
const nextId = (items) => Math.max(0, ...items.map((item) => Number(item.id) || 0)) + 1
const includes = (value, keyword) => String(value || '').toLowerCase().includes(String(keyword || '').trim().toLowerCase())

export const managementApi = {
  students: async (params = {}) => {
    if (!mock) {
      const data = await request.get('/students', { params })
      return { list: listFrom(data), total: data?.total ?? data?.list?.length ?? 0 }
    }
    const filtered = students.filter((item) =>
      (!params.keyword || [item.studentNo, item.name, item.className].some((value) => includes(value, params.keyword))) &&
      (!params.classId || Number(item.classId) === Number(params.classId)))
    return pageResult(filtered, Number(params.page) || 1, Number(params.size) || 10)
  },

  student: async (id) => {
    if (!mock) return request.get(`/students/${id}`)
    const item = students.find((entry) => entry.id === id)
    if (!item) throw new Error('学生记录不存在')
    return clone(item)
  },

  studentCourses: async (id) => {
    if (!mock) return request.get(`/students/${id}/courses`)
    if (!students.some((entry) => Number(entry.id) === Number(id))) throw new Error('学生记录不存在')
    return clone(studentCourseSelections.get(Number(id)) || [])
  },

  setStudentCourses: async (id, courseIds = []) => {
    if (!mock) return request.put(`/students/${id}/courses`, { courseIds })
    if (!students.some((entry) => Number(entry.id) === Number(id))) throw new Error('学生记录不存在')
    const ids = [...new Set(courseIds.map(Number))]
    const allCourses = (await academicApi.courses({ page: 1, size: 100 })).list || []
    if (ids.some((courseId) => !allCourses.some((course) => Number(course.id) === courseId))) throw new Error('所选课程不存在')
    studentCourseSelections.set(Number(id), ids)
    return { studentId: Number(id), courseIds: clone(ids) }
  },

  addStudent: async (data) => {
    if (!mock) return request.post('/students', data)
    assertUnique(students, 'studentNo', data.studentNo, null, '学号已存在')
    const item = { ...data, id: nextId(students), className: classes.find((entry) => entry.id === Number(data.classId))?.className || '' }
    students.unshift(item)
    return clone(item)
  },

  updateStudent: async (id, data) => {
    if (!mock) return request.put(`/students/${id}`, data)
    assertUnique(students, 'studentNo', data.studentNo, id, '学号已存在')
    const item = { ...students.find((entry) => entry.id === id), ...data, id }
    item.className = classes.find((entry) => entry.id === Number(item.classId))?.className || item.className
    students = students.map((entry) => entry.id === id ? item : entry)
    return clone(item)
  },

  deleteStudent: async (id) => {
    if (!mock) return request.delete(`/students/${id}`)
    students = students.filter((entry) => entry.id !== id)
  },

  exportStudents: async (params = {}) => {
    if (!mock) return request.get('/students/export', { params, responseType: 'blob' })
    const rows = students.filter((item) =>
      (!params.keyword || [item.studentNo, item.name, item.className].some((value) => includes(value, params.keyword))) &&
      (!params.classId || Number(item.classId) === Number(params.classId)))
    const columns = ['学号', '姓名', '性别', '班级', '联系电话', '邮箱']
    const csv = [columns, ...rows.map((item) => [
      item.studentNo, item.name, item.gender, item.className, item.phone, item.email,
    ])].map((row) => row.map((cell) => `"${String(cell ?? '').replaceAll('"', '""')}"`).join(',')).join('\r\n')
    return new Blob(['\uFEFF', csv], { type: 'text/csv;charset=utf-8' })
  },

  teachers: async (params = {}) => {
    if (!mock) {
      const data = await request.get('/teachers', { params })
      return { list: listFrom(data), total: data?.total ?? data?.list?.length ?? 0 }
    }
    const filtered = teachers.filter((item) =>
      (!params.keyword || [item.teacherNo, item.name, item.deptName].some((value) => includes(value, params.keyword))) &&
      (!params.deptId || Number(item.deptId) === Number(params.deptId)))
    return pageResult(filtered, Number(params.page) || 1, Number(params.size) || 10)
  },

  addTeacher: async (data) => {
    if (!mock) return request.post('/teachers', data)
    assertUnique(teachers, 'teacherNo', data.teacherNo, null, '工号已存在')
    const item = { ...data, id: nextId(teachers), deptName: departments.find((entry) => entry.id === Number(data.deptId))?.deptName || '' }
    teachers.unshift(item)
    return clone(item)
  },

  updateTeacher: async (id, data) => {
    if (!mock) return request.put(`/teachers/${id}`, data)
    assertUnique(teachers, 'teacherNo', data.teacherNo, id, '工号已存在')
    const item = { ...teachers.find((entry) => entry.id === id), ...data, id }
    item.deptName = departments.find((entry) => entry.id === Number(item.deptId))?.deptName || item.deptName
    teachers = teachers.map((entry) => entry.id === id ? item : entry)
    return clone(item)
  },

  deleteTeacher: async (id) => {
    if (!mock) return request.delete(`/teachers/${id}`)
    teachers = teachers.filter((entry) => entry.id !== id)
  },

  departments: async () => {
    if (!mock) return listFrom(await request.get('/departments'))
    return clone(departments)
  },

  majors: async () => {
    if (!mock) return listFrom(await request.get('/majors'))
    return clone(majors)
  },

  addDepartment: async (data) => {
    if (!mock) return request.post('/departments', data)
    assertUnique(departments, 'deptName', data.deptName, null, '院系名称已存在')
    const item = { ...data, id: nextId(departments) }
    departments.push(item)
    return clone(item)
  },

  updateDepartment: async (id, data) => {
    if (!mock) return request.put(`/departments/${id}`, data)
    assertUnique(departments, 'deptName', data.deptName, id, '院系名称已存在')
    departments = departments.map((entry) => entry.id === id ? { ...entry, ...data, id } : entry)
  },

  deleteDepartment: async (id) => {
    if (!mock) return request.delete(`/departments/${id}`)
    departments = departments.filter((entry) => entry.id !== id)
  },

  classes: async () => {
    if (!mock) return listFrom(await request.get('/classes'))
    return clone(classes)
  },

  addClass: async (data) => {
    if (!mock) return request.post('/classes', data)
    if (classes.some((item) => item.className === data.className && Number(item.grade) === Number(data.grade))) {
      throw new Error('同年级班级名称已存在')
    }
    const item = { ...data, id: nextId(classes) }
    classes.push(item)
    return clone(item)
  },

  updateClass: async (id, data) => {
    if (!mock) return request.put(`/classes/${id}`, data)
    if (classes.some((item) => item.className === data.className && Number(item.grade) === Number(data.grade) && item.id !== id)) {
      throw new Error('同年级班级名称已存在')
    }
    classes = classes.map((entry) => entry.id === id ? { ...entry, ...data, id } : entry)
  },

  deleteClass: async (id) => {
    if (!mock) return request.delete(`/classes/${id}`)
    classes = classes.filter((entry) => entry.id !== id)
  },
}
