<template>
  <section class="management-page">
    <div class="page-heading">
      <div>
        <div class="eyebrow">STUDENT RECORDS</div>
        <h1>学生管理</h1>
        <p>维护学生档案，支持按学号、姓名或班级查询。</p>
      </div>
      <div class="heading-actions">
        <el-button :loading="exporting" @click="exportList">导出当前结果</el-button>
        <el-button type="primary" @click="openEditor()">新增学生</el-button>
      </div>
    </div>

    <el-card shadow="never" class="table-card">
      <div class="toolbar">
        <el-input v-model="filters.keyword" clearable placeholder="学号、姓名或班级" class="search-input" @keyup.enter="search" />
        <el-select v-model="filters.classId" clearable placeholder="全部班级" class="filter-select">
          <el-option v-for="item in classOptions" :key="item.id" :label="item.className" :value="item.id" />
        </el-select>
        <el-button type="primary" plain @click="search">查询</el-button>
        <el-button @click="resetSearch">重置</el-button>
        <span class="toolbar-spacer"></span>
        <span class="muted">共 {{ total }} 名学生</span>
      </div>

      <el-table v-loading="loading" :data="rows" stripe row-key="id" empty-text="暂无学生记录">
        <el-table-column prop="studentNo" label="学号" min-width="130" />
        <el-table-column prop="name" label="姓名" min-width="100" />
        <el-table-column prop="gender" label="性别" width="80" />
        <el-table-column prop="className" label="所属班级" min-width="130" />
        <el-table-column prop="birthDate" label="出生日期" min-width="120">
          <template #default="{ row }">{{ row.birthDate || '—' }}</template>
        </el-table-column>
        <el-table-column prop="phone" label="联系电话" min-width="130">
          <template #default="{ row }">{{ row.phone || '—' }}</template>
        </el-table-column>
        <el-table-column prop="email" label="邮箱" min-width="190">
          <template #default="{ row }">{{ row.email || '—' }}</template>
        </el-table-column>
        <el-table-column label="操作" width="130" fixed="right">
          <template #default="{ row }">
            <el-button link type="primary" @click="openEditor(row)">编辑</el-button>
            <el-button link type="danger" @click="removeStudent(row)">删除</el-button>
          </template>
        </el-table-column>
      </el-table>

      <div class="table-footer">
        <span>第 {{ total ? (page - 1) * pageSize + 1 : 0 }}–{{ Math.min(page * pageSize, total) }} 条，共 {{ total }} 条</span>
        <el-pagination
          v-model:current-page="page"
          v-model:page-size="pageSize"
          :page-sizes="[10, 20, 50]"
          :total="total"
          layout="sizes, prev, pager, next"
          @change="loadStudents"
        />
      </div>
    </el-card>

    <el-dialog v-model="dialogVisible" :title="editing ? '编辑学生档案' : '新增学生'" width="620px" destroy-on-close>
      <el-form ref="formRef" :model="form" :rules="rules" label-width="100px">
        <div class="form-grid">
          <el-form-item label="学号" prop="studentNo">
            <el-input v-model.trim="form.studentNo" :disabled="editing" maxlength="20" placeholder="请输入学号" />
          </el-form-item>
          <el-form-item label="姓名" prop="name">
            <el-input v-model.trim="form.name" maxlength="50" placeholder="请输入姓名" />
          </el-form-item>
          <el-form-item label="性别" prop="gender">
            <el-select v-model="form.gender" placeholder="请选择性别">
              <el-option label="男" value="男" />
              <el-option label="女" value="女" />
            </el-select>
          </el-form-item>
          <el-form-item label="出生日期">
            <el-date-picker v-model="form.birthDate" type="date" value-format="YYYY-MM-DD" placeholder="请选择日期" />
          </el-form-item>
          <el-form-item label="所属班级" prop="classId">
            <el-select v-model="form.classId" placeholder="请选择班级" @change="onClassChange">
              <el-option v-for="item in classOptions" :key="item.id" :label="item.className" :value="item.id" />
            </el-select>
          </el-form-item>
          <el-form-item label="选修课程" prop="courseIds">
            <el-select v-model="form.courseIds" multiple filterable clearable collapse-tags collapse-tags-tooltip placeholder="选择该学生可见的课程">
              <el-option v-for="item in courseOptions" :key="item.id" :label="`${item.courseName}${item.semester ? `（${item.semester}）` : ''}`" :value="item.id" />
            </el-select>
          </el-form-item>
          <el-form-item label="联系电话" prop="phone">
            <el-input v-model.trim="form.phone" maxlength="11" placeholder="选填，11 位手机号" />
          </el-form-item>
          <el-form-item label="邮箱" prop="email">
            <el-input v-model.trim="form.email" maxlength="100" placeholder="选填" />
          </el-form-item>
        </div>
        <el-alert v-if="!editing" title="新增后系统将自动创建学生登录账号；课程仅显示在此处明确选中的内容。" type="info" :closable="false" show-icon />
      </el-form>
      <template #footer>
        <el-button @click="dialogVisible = false">取消</el-button>
        <el-button type="primary" :loading="saving" @click="saveStudent">保存</el-button>
      </template>
    </el-dialog>
  </section>
</template>

<script setup>
import { computed, onMounted, reactive, ref } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { managementApi } from '../api/management.js'
import { academicApi } from '../api/academic.js'

const rows = ref([])
const total = ref(0)
const page = ref(1)
const pageSize = ref(10)
const loading = ref(false)
const saving = ref(false)
const exporting = ref(false)
const dialogVisible = ref(false)
const formRef = ref()
const classOptions = ref([])
const courseOptions = ref([])
const editingId = ref(null)
const editing = computed(() => editingId.value !== null)
const filters = reactive({ keyword: '', classId: undefined })
const initialForm = () => ({
  studentNo: '', name: '', gender: '男', birthDate: '', classId: undefined,
  phone: '', email: '', courseIds: [],
})
const form = reactive(initialForm())
const phoneValidator = (_rule, value, callback) => {
  if (!value || /^1\d{10}$/.test(value)) callback()
  else callback(new Error('请输入有效的 11 位手机号'))
}
const rules = {
  studentNo: [
    { required: true, message: '请输入学号', trigger: 'blur' },
    { pattern: /^\d{8,20}$/, message: '学号需为 8 至 20 位数字', trigger: 'blur' },
  ],
  name: [{ required: true, message: '请输入姓名', trigger: 'blur' }],
  gender: [{ required: true, message: '请选择性别', trigger: 'change' }],
  classId: [{ required: true, message: '请选择班级', trigger: 'change' }],
  phone: [{ validator: phoneValidator, trigger: 'blur' }],
  email: [{ type: 'email', message: '请输入有效邮箱地址', trigger: 'blur' }],
}

async function loadOptions() {
  const [classes, courses] = await Promise.all([
    managementApi.classes(),
    academicApi.courses({ page: 1, size: 100 }),
  ])
  classOptions.value = classes
  courseOptions.value = courses?.list || []
}

// 默认跟随班级选课：取该班级开设的全部课程
function classCourseIds(classId) {
  return courseOptions.value
    .filter((course) => Number(course.classId) === Number(classId))
    .map((course) => Number(course.id))
}

function onClassChange(classId) {
  form.courseIds = classCourseIds(classId)
}

async function loadStudents() {
  loading.value = true
  try {
    const result = await managementApi.students({
      page: page.value,
      size: pageSize.value,
      keyword: filters.keyword,
      classId: filters.classId,
    })
    rows.value = result.list
    total.value = result.total
  } catch (error) {
    ElMessage.error(error.message || '学生列表加载失败')
  } finally {
    loading.value = false
  }
}

function search() {
  page.value = 1
  void loadStudents()
}

function resetSearch() {
  filters.keyword = ''
  filters.classId = undefined
  search()
}

async function openEditor(row) {
  editingId.value = row?.id ?? null
  if (row) {
    try {
      const details = await managementApi.student(row.id)
      Object.assign(form, initialForm(), {
        ...details,
        classId: Number(details.classId),
        birthDate: details.birthDate || '',
        courseIds: classCourseIds(details.classId),
      })
    } catch (error) {
      ElMessage.error(error.message || '学生详情加载失败')
      return
    }
  } else {
    Object.assign(form, initialForm())
  }
  dialogVisible.value = true
}

async function saveStudent() {
  if (!formRef.value) return
  try {
    await formRef.value.validate()
  } catch {
    return
  }

  saving.value = true
  try {
    const payload = {
      studentNo: form.studentNo,
      name: form.name,
      gender: form.gender,
      birthDate: form.birthDate || '',
      classId: Number(form.classId),
      phone: form.phone,
      email: form.email,
    }
    const savedStudent = editing.value
      ? await managementApi.updateStudent(editingId.value, payload)
      : await managementApi.addStudent(payload)
    const studentId = editing.value ? editingId.value : Number(savedStudent?.id)
    if (studentId > 0) await managementApi.setStudentCourses(studentId, form.courseIds)
    ElMessage.success(editing.value ? '学生信息已更新' : '学生已新增')
    dialogVisible.value = false
    await loadStudents()
  } catch (error) {
    ElMessage.error(error.message || '学生信息保存失败')
  } finally {
    saving.value = false
  }
}

async function removeStudent(row) {
  try {
    await ElMessageBox.confirm(`确定删除学生“${row.name}”（${row.studentNo}）吗？`, '删除确认', {
      type: 'warning',
      confirmButtonText: '确认删除',
      cancelButtonText: '取消',
    })
  } catch {
    return
  }
  try {
    await managementApi.deleteStudent(row.id)
    ElMessage.success('学生记录已删除')
    if (rows.value.length === 1 && page.value > 1) page.value -= 1
    await loadStudents()
  } catch (error) {
    ElMessage.error(error.message || '学生删除失败')
  }
}

async function exportList() {
  exporting.value = true
  try {
    const blob = await managementApi.exportStudents(filters)
    const url = URL.createObjectURL(blob)
    const link = document.createElement('a')
    link.href = url
    link.download = blob.type.includes('spreadsheet') ? '学生信息.xlsx' : '学生信息.csv'
    document.body.append(link)
    link.click()
    window.setTimeout(() => link.remove(), 100)
    window.setTimeout(() => URL.revokeObjectURL(url), 1000)
    ElMessage.success('学生信息已导出')
  } catch (error) {
    ElMessage.error(error.message || '学生信息导出失败')
  } finally {
    exporting.value = false
  }
}

onMounted(async () => {
  try {
    await loadOptions()
    await loadStudents()
  } catch (error) {
    ElMessage.error(error.message || '班级选项加载失败')
  }
})
</script>

<style scoped>
.management-page { max-width: 1280px; margin: 0 auto; }
.page-heading { display: flex; justify-content: space-between; align-items: center; gap: 18px; margin: 8px 0 22px; }
.eyebrow { color: #409eff; font-size: 12px; font-weight: 700; letter-spacing: 1.3px; }
h1 { margin: 7px 0; color: #202b3c; font-size: 26px; }
.page-heading p { margin: 0; color: #7a8494; }
.heading-actions { display: flex; gap: 10px; }
.table-card { border-radius: 12px; }
.toolbar { display: flex; flex-wrap: wrap; align-items: center; gap: 10px; margin-bottom: 18px; }
.search-input { width: 280px; }
.filter-select { width: 190px; }
.toolbar-spacer { flex: 1; }
.muted, .table-footer { color: #7a8494; font-size: 13px; }
.table-footer { display: flex; justify-content: space-between; align-items: center; gap: 12px; padding-top: 18px; }
.form-grid { display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); column-gap: 16px; }
.form-grid :deep(.el-select), .form-grid :deep(.el-date-editor) { width: 100%; }
@media (max-width: 760px) {
  .page-heading, .table-footer { align-items: flex-start; flex-direction: column; }
  .heading-actions { width: 100%; }
  .search-input, .filter-select { width: 100%; }
  .form-grid { grid-template-columns: 1fr; }
}
</style>
