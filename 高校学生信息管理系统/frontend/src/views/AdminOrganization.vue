<template>
  <section class="management-page">
    <div class="page-heading">
      <div>
        <div class="eyebrow">ORGANIZATION</div>
        <h1>院系 / 班级管理</h1>
        <p>维护院系与班级基础信息，并为班级指定班主任。</p>
      </div>
    </div>

    <el-tabs v-model="activeTab" class="organization-tabs">
      <el-tab-pane label="院系管理" name="departments">
        <el-card shadow="never" class="table-card">
          <div class="panel-heading">
            <div><h2>院系列表</h2><p>共 {{ departments.length }} 个院系</p></div>
            <el-button type="primary" @click="openDepartmentEditor()">新增院系</el-button>
          </div>
          <el-table v-loading="departmentLoading" :data="departments" stripe row-key="id" empty-text="暂无院系">
            <el-table-column prop="id" label="院系 ID" width="110" />
            <el-table-column prop="deptName" label="院系名称" min-width="220" />
            <el-table-column label="操作" width="140" fixed="right">
              <template #default="{ row }">
                <el-button link type="primary" @click="openDepartmentEditor(row)">编辑</el-button>
                <el-button link type="danger" @click="removeDepartment(row)">删除</el-button>
              </template>
            </el-table-column>
          </el-table>
        </el-card>
      </el-tab-pane>

      <el-tab-pane label="班级管理" name="classes">
        <el-card shadow="never" class="table-card">
          <div class="panel-heading">
            <div><h2>班级列表</h2><p>维护专业、年级及班主任关联</p></div>
            <el-button type="primary" @click="openClassEditor()">新增班级</el-button>
          </div>
          <el-table v-loading="classLoading" :data="classes" stripe row-key="id" empty-text="暂无班级">
            <el-table-column prop="className" label="班级名称" min-width="160" />
            <el-table-column label="所属学院" min-width="160">
              <template #default="{ row }">{{ row.deptName || '未设置' }}</template>
            </el-table-column>
            <el-table-column label="专业" min-width="140">
              <template #default="{ row }">{{ row.majorName || `专业 #${row.majorId}` }}</template>
            </el-table-column>
            <el-table-column prop="grade" label="入学年级" width="120" />
            <el-table-column label="班主任" min-width="140">
              <template #default="{ row }">{{ teacherName(row.headTeacherId) }}</template>
            </el-table-column>
            <el-table-column label="操作" width="140" fixed="right">
              <template #default="{ row }">
                <el-button link type="primary" @click="openClassEditor(row)">编辑</el-button>
                <el-button link type="danger" @click="removeClass(row)">删除</el-button>
              </template>
            </el-table-column>
          </el-table>
        </el-card>
      </el-tab-pane>
    </el-tabs>

    <el-dialog v-model="departmentDialog" :title="departmentEditing ? '编辑院系' : '新增院系'" width="460px" destroy-on-close>
      <el-form ref="departmentFormRef" :model="departmentForm" :rules="departmentRules" label-width="100px">
        <el-form-item label="院系名称" prop="deptName">
          <el-input v-model.trim="departmentForm.deptName" maxlength="100" placeholder="请输入院系名称" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="departmentDialog = false">取消</el-button>
        <el-button type="primary" :loading="saving" @click="saveDepartment">保存</el-button>
      </template>
    </el-dialog>

    <el-dialog v-model="classDialog" :title="classEditing ? '编辑班级' : '新增班级'" width="520px" destroy-on-close>
      <el-form ref="classFormRef" :model="classForm" :rules="classRules" label-width="100px">
        <el-form-item label="班级名称" prop="className">
          <el-input v-model.trim="classForm.className" maxlength="100" placeholder="如：软件2505班" />
        </el-form-item>
        <el-form-item label="专业 ID" prop="majorId">
          <el-input-number v-model="classForm.majorId" :min="1" :precision="0" controls-position="right" />
          <div class="field-hint">按专业基础数据中的 ID 填写；当前接口文档未定义专业管理接口。</div>
        </el-form-item>
        <el-form-item label="入学年级" prop="grade">
          <el-input-number v-model="classForm.grade" :min="2000" :max="2100" :precision="0" controls-position="right" />
        </el-form-item>
        <el-form-item label="班主任">
          <el-select v-model="classForm.headTeacherId" clearable placeholder="不指定班主任">
            <el-option v-for="item in teachers" :key="item.id" :label="`${item.name}（${item.teacherNo}）`" :value="item.id" />
          </el-select>
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="classDialog = false">取消</el-button>
        <el-button type="primary" :loading="saving" @click="saveClass">保存</el-button>
      </template>
    </el-dialog>
  </section>
</template>

<script setup>
import { computed, onMounted, reactive, ref } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { managementApi } from '../api/management.js'

const activeTab = ref('departments')
const departments = ref([])
const classes = ref([])
const teachers = ref([])
const departmentLoading = ref(false)
const classLoading = ref(false)
const saving = ref(false)
const departmentDialog = ref(false)
const classDialog = ref(false)
const departmentFormRef = ref()
const classFormRef = ref()
const departmentId = ref(null)
const classId = ref(null)
const departmentEditing = computed(() => departmentId.value !== null)
const classEditing = computed(() => classId.value !== null)
const departmentForm = reactive({ deptName: '' })
const classForm = reactive({ className: '', majorId: 1, grade: new Date().getFullYear(), headTeacherId: undefined })
const departmentRules = {
  deptName: [{ required: true, message: '请输入院系名称', trigger: 'blur' }],
}
const classRules = {
  className: [{ required: true, message: '请输入班级名称', trigger: 'blur' }],
  majorId: [{ required: true, type: 'number', message: '请输入有效的专业 ID', trigger: 'change' }],
  grade: [{ required: true, type: 'number', message: '请输入入学年级', trigger: 'change' }],
}

function teacherName(id) {
  return teachers.value.find((item) => Number(item.id) === Number(id))?.name || (id ? `教师 #${id}` : '未指定')
}

async function loadDepartments() {
  departmentLoading.value = true
  try {
    departments.value = await managementApi.departments()
  } catch (error) {
    ElMessage.error(error.message || '院系列表加载失败')
  } finally {
    departmentLoading.value = false
  }
}

async function loadClasses() {
  classLoading.value = true
  try {
    classes.value = await managementApi.classes()
  } catch (error) {
    ElMessage.error(error.message || '班级列表加载失败')
  } finally {
    classLoading.value = false
  }
}

function openDepartmentEditor(row) {
  departmentId.value = row?.id ?? null
  departmentForm.deptName = row?.deptName || ''
  departmentDialog.value = true
}

async function saveDepartment() {
  if (!departmentFormRef.value) return
  try {
    await departmentFormRef.value.validate()
  } catch {
    return
  }
  saving.value = true
  try {
    const payload = { deptName: departmentForm.deptName }
    if (departmentEditing.value) await managementApi.updateDepartment(departmentId.value, payload)
    else await managementApi.addDepartment(payload)
    ElMessage.success(departmentEditing.value ? '院系信息已更新' : '院系已新增')
    departmentDialog.value = false
    await loadDepartments()
  } catch (error) {
    ElMessage.error(error.message || '院系信息保存失败')
  } finally {
    saving.value = false
  }
}

async function removeDepartment(row) {
  try {
    await ElMessageBox.confirm(`确定删除院系“${row.deptName}”吗？若院系仍有教师或专业关联，后端可能拒绝删除。`, '删除确认', {
      type: 'warning',
      confirmButtonText: '确认删除',
      cancelButtonText: '取消',
    })
  } catch {
    return
  }
  try {
    await managementApi.deleteDepartment(row.id)
    ElMessage.success('院系已删除')
    await loadDepartments()
  } catch (error) {
    ElMessage.error(error.message || '院系删除失败')
  }
}

function openClassEditor(row) {
  classId.value = row?.id ?? null
  Object.assign(classForm, {
    className: row?.className || '',
    majorId: Number(row?.majorId) || 1,
    grade: Number(row?.grade) || new Date().getFullYear(),
    headTeacherId: row?.headTeacherId ? Number(row.headTeacherId) : undefined,
  })
  classDialog.value = true
}

async function saveClass() {
  if (!classFormRef.value) return
  try {
    await classFormRef.value.validate()
  } catch {
    return
  }
  saving.value = true
  try {
    const payload = {
      className: classForm.className,
      majorId: Number(classForm.majorId),
      grade: Number(classForm.grade),
      headTeacherId: classForm.headTeacherId ? Number(classForm.headTeacherId) : null,
    }
    if (classEditing.value) await managementApi.updateClass(classId.value, payload)
    else await managementApi.addClass(payload)
    ElMessage.success(classEditing.value ? '班级信息已更新' : '班级已新增')
    classDialog.value = false
    await loadClasses()
  } catch (error) {
    ElMessage.error(error.message || '班级信息保存失败')
  } finally {
    saving.value = false
  }
}

async function removeClass(row) {
  try {
    await ElMessageBox.confirm(`确定删除班级“${row.className}”吗？若仍有关联学生，后端可能拒绝删除。`, '删除确认', {
      type: 'warning',
      confirmButtonText: '确认删除',
      cancelButtonText: '取消',
    })
  } catch {
    return
  }
  try {
    await managementApi.deleteClass(row.id)
    ElMessage.success('班级已删除')
    await loadClasses()
  } catch (error) {
    ElMessage.error(error.message || '班级删除失败')
  }
}

onMounted(async () => {
  const results = await Promise.allSettled([
    loadDepartments(),
    loadClasses(),
    managementApi.teachers({ page: 1, size: 100 }),
  ])
  if (results[2].status === 'fulfilled') teachers.value = results[2].value.list
  else ElMessage.error(results[2].reason?.message || '班主任选项加载失败')
})
</script>

<style scoped>
.management-page { max-width: 1280px; margin: 0 auto; }
.page-heading { display: flex; justify-content: space-between; align-items: center; margin: 8px 0 18px; }
.eyebrow { color: #409eff; font-size: 12px; font-weight: 700; letter-spacing: 1.3px; }
h1 { margin: 7px 0; color: #202b3c; font-size: 26px; }
.page-heading p, .panel-heading p { margin: 0; color: #7a8494; }
.organization-tabs { margin-top: 6px; }
.table-card { border-radius: 12px; }
.panel-heading { display: flex; justify-content: space-between; align-items: center; gap: 16px; margin-bottom: 18px; }
h2 { margin: 0 0 6px; color: #29364a; font-size: 18px; }
.field-hint { color: #909399; font-size: 12px; line-height: 1.5; }
.management-page :deep(.el-select) { width: 100%; }
@media (max-width: 640px) {
  .panel-heading { align-items: flex-start; flex-direction: column; }
}
</style>
