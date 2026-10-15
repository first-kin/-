import test from 'node:test'
import assert from 'node:assert/strict'
import { academicApi } from '../src/api/academic.js'

test('课程列表支持关键词、学期、分页及课程维护校验', async () => {
  const first = await academicApi.courses({ keyword: '数据', semester: '2026-2027-1', page: 1, size: 1 })
  assert.equal(first.total, 1)
  assert.equal(first.list[0].courseName, '数据结构')

  const created = await academicApi.createCourse({ courseName: 'F 模块用例课程', credit: 2, teacherId: 1, semester: '2026-2027-1', courseTime: '周六 1-2 节' })
  try {
    assert.ok(created.id)
    await academicApi.updateCourse(created.id, { courseName: 'F 模块更新课程', credit: 3, teacherId: 1, semester: '2026-2027-1', courseTime: '周日 1-2 节' })
    const updated = await academicApi.courses({ keyword: 'F 模块更新课程', page: 1, size: 10 })
    assert.equal(updated.list[0].credit, 3)
    await academicApi.deleteCourse(created.id)
    assert.equal((await academicApi.courses({ keyword: 'F 模块更新课程' })).total, 0)
  } finally {
    const remaining = await academicApi.courses({ keyword: 'F 模块更新课程' })
    for (const item of remaining.list) await academicApi.deleteCourse(item.id)
  }
})

test('成绩录入校验并计算课程统计，关联成绩的课程不可删除', async () => {
  const course = await academicApi.createCourse({ courseName: 'F 模块成绩用例', credit: 2, teacherId: 1, semester: '2026-2027-1' })
  try {
    await assert.rejects(academicApi.saveScore({ courseId: course.id, studentId: 8, status: 0, score: 101 }), /0~100/)
    await academicApi.saveScore({ courseId: course.id, studentId: 8, status: 0, score: 86 })
    const result = await academicApi.statistics(course.id)
    assert.equal(result.avgScore, 86)
    assert.equal(result.passRate, 1)
    assert.equal(result.distribution.find((item) => item.segment === '80-89').count, 1)
    await assert.rejects(academicApi.deleteCourse(course.id), /已有成绩记录/)
  } finally {
    // The module mock is isolated to this test process; no production data is changed.
  }
})

test('个人资料字段和密码规则可用', async () => {
  const original = await academicApi.profile()
  await academicApi.updateProfile({ ...original, phone: '13800009999', email: 'f-test@example.edu.cn', address: '测试地址' })
  assert.equal((await academicApi.profile()).phone, '13800009999')
  await academicApi.updateProfile(original)
  await assert.rejects(academicApi.changePassword({ oldPassword: 'wrong', newPassword: 'abcdef' }), /原密码不正确/)
  await assert.rejects(academicApi.changePassword({ oldPassword: '123456', newPassword: '123' }), /至少 6 位/)
  assert.equal(await academicApi.changePassword({ oldPassword: '123456', newPassword: 'newpass123' }), true)
})
