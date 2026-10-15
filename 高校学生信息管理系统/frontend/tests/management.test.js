import assert from 'node:assert/strict'
import test from 'node:test'
import { managementApi } from '../src/api/management.js'

test('学生管理支持筛选、分页和完整 CRUD', async () => {
  const initial = await managementApi.students({ page: 1, size: 100 })
  const studentNo = `209901${String(Date.now()).slice(-6)}`
  const payload = {
    studentNo,
    name: '管理测试学生',
    gender: '女',
    classId: 1,
    phone: '13800000000',
    email: 'management-test@example.edu.cn',
  }

  await managementApi.addStudent(payload)
  let result = await managementApi.students({ keyword: studentNo, page: 1, size: 10 })
  assert.equal(result.total, 1)
  assert.equal(result.list[0].className, '软件2505班')
  await assert.rejects(managementApi.addStudent(payload), /学号已存在/)

  const student = result.list[0]
  assert.equal((await managementApi.student(student.id)).classId, 1)
  await managementApi.updateStudent(student.id, { ...payload, name: '更新后的测试学生' })
  result = await managementApi.students({ keyword: studentNo, page: 1, size: 10 })
  assert.equal(result.list[0].name, '更新后的测试学生')

  const page = await managementApi.students({ page: 1, size: 1 })
  assert.equal(page.list.length, 1)
  assert.ok(page.total >= initial.total + 1)

  await managementApi.deleteStudent(student.id)
  result = await managementApi.students({ keyword: studentNo, page: 1, size: 10 })
  assert.equal(result.total, 0)
})

test('教师、院系和班级管理支持新增、查询、编辑及删除', async () => {
  const dept = await managementApi.addDepartment({ deptName: `测试院系${Date.now()}` })
  await assert.rejects(managementApi.addDepartment({ deptName: dept.deptName }), /院系名称已存在/)
  let deptList = await managementApi.departments()
  assert.ok(deptList.some((item) => item.id === dept.id))
  await managementApi.updateDepartment(dept.id, { deptName: '已更新测试院系' })
  deptList = await managementApi.departments()
  assert.equal(deptList.find((item) => item.id === dept.id).deptName, '已更新测试院系')

  const teacher = await managementApi.addTeacher({
    teacherNo: `T${Date.now()}`,
    name: '测试教师',
    gender: '男',
    title: '讲师',
    deptId: dept.id,
  })
  await assert.rejects(managementApi.addTeacher({
    teacherNo: teacher.teacherNo,
    name: '重复工号教师',
    gender: '女',
    deptId: dept.id,
  }), /工号已存在/)
  let teacherList = await managementApi.teachers({ keyword: teacher.teacherNo, page: 1, size: 10 })
  assert.equal(teacherList.total, 1)
  assert.equal(teacherList.list[0].deptName, '已更新测试院系')
  await managementApi.updateTeacher(teacher.id, { ...teacher, name: '更新后的测试教师' })
  teacherList = await managementApi.teachers({ keyword: teacher.teacherNo, page: 1, size: 10 })
  assert.equal(teacherList.list[0].name, '更新后的测试教师')

  const classRecord = await managementApi.addClass({
    className: '测试班级',
    majorId: 3,
    grade: 2025,
    headTeacherId: teacher.id,
  })
  await assert.rejects(managementApi.addClass({
    className: classRecord.className,
    majorId: 3,
    grade: classRecord.grade,
  }), /同年级班级名称已存在/)
  assert.ok((await managementApi.classes()).some((item) => item.id === classRecord.id))
  await managementApi.updateClass(classRecord.id, { ...classRecord, className: '更新后的测试班级' })
  assert.equal((await managementApi.classes()).find((item) => item.id === classRecord.id).className, '更新后的测试班级')

  await managementApi.deleteClass(classRecord.id)
  await managementApi.deleteTeacher(teacher.id)
  await managementApi.deleteDepartment(dept.id)
  assert.ok(!(await managementApi.classes()).some((item) => item.id === classRecord.id))
  assert.ok(!(await managementApi.teachers({ keyword: teacher.teacherNo, page: 1, size: 10 })).total)
  assert.ok(!(await managementApi.departments()).some((item) => item.id === dept.id))
})

test('学生管理支持组合筛选、跨页查询、空结果及 Excel 可读导出', async () => {
  const prefix = `2088${String(Date.now()).slice(-8)}`
  const created = []

  for (let index = 0; index < 12; index += 1) {
    created.push(await managementApi.addStudent({
      studentNo: `${prefix}${String(index).padStart(2, '0')}`,
      name: index % 2 === 0 ? `分页案例${index}` : `筛选案例${index}`,
      gender: index % 2 === 0 ? '男' : '女',
      classId: index % 2 === 0 ? 1 : 2,
      phone: '',
      email: '',
    }))
  }

  try {
    await managementApi.updateStudent(created[0].id, {
      ...created[0],
      name: '学生, "导出测试"',
    })
    const firstPage = await managementApi.students({ keyword: prefix, page: 1, size: 10 })
    const secondPage = await managementApi.students({ keyword: prefix, page: 2, size: 10 })
    assert.equal(firstPage.total, 12)
    assert.equal(firstPage.list.length, 10)
    assert.equal(secondPage.list.length, 2)

    const classFiltered = await managementApi.students({
      keyword: prefix,
      classId: 2,
      page: 1,
      size: 20,
    })
    assert.equal(classFiltered.total, 6)
    assert.ok(classFiltered.list.every((item) => item.classId === 2))

    const keywordFiltered = await managementApi.students({
      keyword: `筛选案例`,
      page: 1,
      size: 20,
    })
    assert.ok(keywordFiltered.total >= 6)

    const noResults = await managementApi.students({
      keyword: '不存在的学生记录',
      page: 1,
      size: 10,
    })
    assert.equal(noResults.total, 0)
    assert.deepEqual(noResults.list, [])

    const blob = await managementApi.exportStudents({ keyword: prefix })
    const csv = await blob.text()
    const bytes = new Uint8Array(await blob.arrayBuffer())
    assert.deepEqual([...bytes.slice(0, 3)], [0xef, 0xbb, 0xbf])
    assert.ok(csv.includes('"学号","姓名","性别","班级","联系电话","邮箱"'))
    assert.ok(csv.includes(`${prefix}00`))
    assert.ok(csv.includes('"学生, ""导出测试"""'))
    assert.equal((csv.match(new RegExp(prefix, 'g')) || []).length, 12)

    await assert.rejects(managementApi.student(-1), /学生记录不存在/)
  } finally {
    await Promise.all(created.map((item) => managementApi.deleteStudent(item.id)))
  }
})
