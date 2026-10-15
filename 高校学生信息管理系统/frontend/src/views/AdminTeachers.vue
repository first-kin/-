<template>
  <section class="management-page">
    <div class="page-heading">
      <div>
        <div class="eyebrow">TEACHER RECORDS</div>
        <h1>教师管理</h1>
        <p>维护教师档案，支持按工号、姓名和院系查询。</p>
      </div>
      <el-button type="primary" @click="openEditor()">新增教师</el-button>
    </div>

    <el-card shadow="never" class="table-card">
      <div class="toolbar">
        <el-input v-model="filters.keyword" clearable placeholder="工号、姓名或院系" class="search-input" @keyup.enter="search" />
        <el-select v-model="filters.deptId" clearable placeholder="全部院系" class="filter-select">
          <el-option v-for="item in departments" :key="item.id" :label="item.deptName" :value="item.id" />
        </el-select>
        <el-button type="primary" plain @click="search">查询</el-button>
        <el-button @click="resetSearch">重置</el-button>
        <span class="toolbar-spacer"></span>
        <span class="muted">共 {{ total }} 位教师</span>
      </div>

      <el-table v-loading="loading" :data="rows" stripe row-key="id" empty-text="暂无教师记录">
        <el-table-column prop="teacherNo" label="工号" min-width="130" />
        <el-table-column prop="name" label="姓名" min-width="110" />
        <el-table-column prop="gender" label="性别" width="80" />
        <el-table-column prop="title" label="职称" min-width="100">
          <template #default="{ row }">{{ row.title || '—' }}</template>
        </el-table-column>
        <el-table-column prop="deptName" label="所属院系" min-width="150" />
        <el-table-column prop="phone" label="联系电话" min-width="130">
          <template #default="{ row }">{{ row.phone || '—' }}</template>
        </el-table-column>
        <el-table-column prop="email" label="邮箱" min-width="190">
          <template #default="{ row }">{{ row.email || '—' }}</template>
        </el-table-column>
        <el-table-column label="操作" width="130" fixed="right">
          <template #default="{ row }">
            <el-button link type="primary" @click="openEditor(row)">编辑</el-button>
            <el-button link type="danger" @click="removeTeacher(row)">删除</el-button>
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
          @change="loadTeachers"
        />
      </div>
    </el-card>

    <el-dialog v-model="dialogVisible" :title="editing ? '编辑教师档案' : '新增教师'" width="620px" destroy-on-close>
      <el-form ref="formRef" :model="form" :rules="rules" label-width="100px">
        <div class="form-grid">
          <el-form-item label="工号" prop="teacherNo">
            <el-input v-model.trim="form.teacherNo" :disabled="editing" maxlength="20" placeholder="请输入工号" />
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
          <el-form-item label="职称">
            <el-select v-model="form.title" clearable placeholder="请选择职称">
              <el-option v-for="title in titles" :key="title" :label="title" :value="title" />
            </el-select>
          </el-form-item>
          <el-form-item label="所属院系" prop="deptId">
            <el-select v-model="form.deptId" placeholder="请选择院系">
              <el-option v-for="item in departments" :key="item.id" :label="item.deptName" :value="item.id" />
            </el-select>
          </el-form-item>
          <el-form-item label="联系电话" prop="phone">
            <el-input v-model.trim="form.phone" maxlength="11" placeholder="选填，11 位手机号" />
          </el-form-item>
          <el-form-item label="邮箱" prop="email">
            <el-input v-model.trim="form.email" maxlength="100" placeholder="选填" />
          </el-form-item>
        </div>
        <el-alert v-if="!editing" title="新增后系统将按接口约定自动创建教师登录账号。" type="info" :closable="false" show-icon />
      </el-form>
      <template #footer>
        <el-button @click="dialogVisible = false">取消</el-button>
        <el-button type="primary" :loading="saving" @click="saveTeacher">保存</el-button>
      </template>
    </el-dialog>
  </section>
</template>

<script setup>
import { computed, onMounted, reactive, ref } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { managementApi } from '../api/management.js'

const rows = ref([])
const total = ref(0)
const page = ref(1)
const pageSize = ref(10)
const loading = ref(false)
const saving = ref(false)
const dialogVisible = ref(false)
const formRef = ref()
const departments = ref([])
const editingId = ref(null)
const editing = computed(() => editingId.value !== null)
const filters = reactive({ keyword: '', deptId: undefined })
const titles = ['教授', '副教授', '讲师', '助教']
const initialForm = () => ({
  teacherNo: '', name: '', gender: '男', title: '', deptId: undefined, phone: '', email: '',
})
const form = reactive(initialForm())
const phoneValidator = (_rule, value, callback) => {
  if (!value || /^1\d{10}$/.test(value)) callback()
  else callback(new Error('请输入有效的 11 位手机号'))
}
const rules = {
  teacherNo: [{ required: true, message: '请输入工号', trigger: 'blur' }],
  name: [{ required: true, message: '请输入姓名', trigger: 'blur' }],
  gender: [{ required: true, message: '请选择性别', trigger: 'change' }],
  deptId: [{ required: true, message: '请选择院系', trigger: 'change' }],
  phone: [{ validator: phoneValidator, trigger: 'blur' }],
  email: [{ type: 'email', message: '请输入有效邮箱地址', trigger: 'blur' }],
}

async function loadTeachers() {
  loading.value = true
  try {
    const result = await managementApi.teachers({
      page: page.value,
      size: pageSize.value,
      keyword: filters.keyword,
      deptId: filters.deptId,
    })
    rows.value = result.list
    total.value = result.total
  } catch (error) {
    ElMessage.error(error.message || '教师列表加载失败')
  } finally {
    loading.value = false
  }
}

function search() {
  page.value = 1
  void loadTeachers()
}

function resetSearch() {
  filters.keyword = ''
  filters.deptId = undefined
  search()
}

function openEditor(row) {
  editingId.value = row?.id ?? null
  Object.assign(form, initialForm(), row ? { ...row, deptId: Number(row.deptId) } : {})
  dialogVisible.value = true
}

async function saveTeacher() {
  if (!formRef.value) return
  try {
    await formRef.value.validate()
  } catch {
    return
  }
  saving.value = true
  try {
    const payload = {
      teacherNo: form.teacherNo,
      name: form.name,
      gender: form.gender,
      title: form.title,
      deptId: Number(form.deptId),
      phone: form.phone,
      email: form.email,
    }
    if (editing.value) await managementApi.updateTeacher(editingId.value, payload)
    else await managementApi.addTeacher(payload)
    ElMessage.success(editing.value ? '教师信息已更新' : '教师已新增')
    dialogVisible.value = false
    await loadTeachers()
  } catch (error) {
    ElMessage.error(error.message || '教师信息保存失败')
  } finally {
    saving.value = false
  }
}

async function removeTeacher(row) {
  try {
    await ElMessageBox.confirm(`确定删除教师“${row.name}”（${row.teacherNo}）吗？若仍关联课程或班级，后端将拒绝删除。`, '删除确认', {
      type: 'warning',
      confirmButtonText: '确认删除',
      cancelButtonText: '取消',
    })
  } catch {
    return
  }
  try {
    await managementApi.deleteTeacher(row.id)
    ElMessage.success('教师记录已删除')
    if (rows.value.length === 1 && page.value > 1) page.value -= 1
    await loadTeachers()
  } catch (error) {
    ElMessage.error(error.message || '教师删除失败')
  }
}

onMounted(async () => {
  try {
    departments.value = await managementApi.departments()
    await loadTeachers()
  } catch (error) {
    ElMessage.error(error.message || '院系选项加载失败')
  }
})
</script>

<style scoped>
.management-page { max-width: 1280px; margin: 0 auto; }
.page-heading { display: flex; justify-content: space-between; align-items: center; gap: 18px; margin: 8px 0 22px; }
.eyebrow { color: #409eff; font-size: 12px; font-weight: 700; letter-spacing: 1.3px; }
h1 { margin: 7px 0; color: #202b3c; font-size: 26px; }
.page-heading p { margin: 0; color: #7a8494; }
.table-card { border-radius: 12px; }
.toolbar { display: flex; flex-wrap: wrap; align-items: center; gap: 10px; margin-bottom: 18px; }
.search-input { width: 280px; }
.filter-select { width: 190px; }
.toolbar-spacer { flex: 1; }
.muted, .table-footer { color: #7a8494; font-size: 13px; }
.table-footer { display: flex; justify-content: space-between; align-items: center; gap: 12px; padding-top: 18px; }
.form-grid { display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); column-gap: 16px; }
.form-grid :deep(.el-select) { width: 100%; }
@media (max-width: 760px) {
  .page-heading, .table-footer { align-items: flex-start; flex-direction: column; }
  .search-input, .filter-select { width: 100%; }
  .form-grid { grid-template-columns: 1fr; }
}
</style>
