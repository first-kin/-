<script setup>
import { computed, onMounted, reactive, ref, watch } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { academicApi } from '../api/academic'
import { managementApi } from '../api/management'
import { useAuthStore } from '../stores/auth'

const props = defineProps({ section: { type: String, required: true } })
const auth = useAuthStore()
const isAdmin = computed(() => auth.role === 'admin')
const isTeacher = computed(() => auth.role === 'teacher')
const heading = computed(() => ({ courses: '课程管理', scores: '成绩管理', profile: '个人中心' })[props.section] || '教学事务')
const busy = ref(false)
const courses = ref([]), courseTotal = ref(0), scores = ref([]), scoreTotal = ref(0)
const teachers = ref([]), students = ref([]), classes = ref([])
const statistics = ref(null)
const profile = reactive({ name: '', username: '', role: '', phone: '', email: '', address: '' })
const courseQuery = reactive({ keyword: '', semester: '', page: 1, size: 10 })
const scoreQuery = reactive({ keyword: '', courseId: '', page: 1, size: 10 })
const courseDialog = ref(false), scoreDialog = ref(false), editingId = ref(null)
const courseForm = reactive({ courseName: '', credit: 2, teacherId: null, classId: null, semester: '2026-2027-1', courseTime: '' })
const scoreForm = reactive({ courseId: null, studentId: null, score: null, status: 0, examTime: '' })
const passwordForm = reactive({ oldPassword: '', newPassword: '', confirmPassword: '' })
const roleLabels = { admin: '管理员', teacher: '任课教师', student: '学生' }

function showError(error) { ElMessage.error(error?.message || '请求失败，请稍后重试') }
async function loadCourses() {
  busy.value = true
  try { const result = await academicApi.courses(courseQuery); courses.value = result?.list || []; courseTotal.value = result?.total || 0 }
  catch (error) { showError(error) } finally { busy.value = false }
}
async function loadScores() {
  busy.value = true
  try { const result = await academicApi.scores({ ...scoreQuery, courseId: scoreQuery.courseId || undefined }); scores.value = result?.list || []; scoreTotal.value = result?.total || 0 }
  catch (error) { showError(error) } finally { busy.value = false }
}
async function loadProfile() {
  try { Object.assign(profile, await academicApi.profile()) } catch (error) { showError(error) }
}
function openCourse(row) {
  if (!teachers.value.length) managementApi.teachers({ page: 1, size: 100 }).then((result) => { teachers.value = result.list || [] }).catch(showError)
  if (!classes.value.length) managementApi.classes().then((result) => { classes.value = result || [] }).catch(showError)
  editingId.value = row?.id || null
  Object.assign(courseForm, { courseName: row?.courseName || '', credit: Number(row?.credit) || 2, teacherId: row?.teacherId || null, classId: row?.classId || null, semester: row?.semester || '2026-2027-1', courseTime: row?.courseTime || '' })
  courseDialog.value = true
}
async function saveCourse() {
  if (!courseForm.courseName.trim()) return ElMessage.warning('请填写课程名称')
  if (!courseForm.credit || courseForm.credit < 0.5 || courseForm.credit > 10) return ElMessage.warning('学分需在 0.5 至 10 之间')
  try {
    if (editingId.value) await academicApi.updateCourse(editingId.value, { ...courseForm })
    else await academicApi.createCourse({ ...courseForm })
    courseDialog.value = false
    ElMessage.success('课程信息已保存')
    await loadCourses()
  } catch (error) { showError(error) }
}
async function removeCourse(row) {
  try {
    await ElMessageBox.confirm(`确定删除「${row.courseName}」吗？`, '删除课程', { type: 'warning', confirmButtonText: '确认删除', cancelButtonText: '取消' })
    await academicApi.deleteCourse(row.id)
    ElMessage.success('课程已删除')
    await loadCourses()
  } catch (error) { if (error !== 'cancel' && error !== 'close') showError(error) }
}
async function openScore() {
  Object.assign(scoreForm, { courseId: null, studentId: null, score: null, status: 0, examTime: '' })
  scoreDialog.value = true
  try {
    if (!courses.value.length) await loadCourses()
    if (!students.value.length) students.value = (await managementApi.students({ page: 1, size: 100 })).list || []
  } catch (error) { showError(error) }
}
async function saveScore() {
  if (!scoreForm.courseId || !scoreForm.studentId) return ElMessage.warning('请填写课程 ID 和学生 ID')
  if (Number(scoreForm.status) === 0 && (scoreForm.score == null || Number(scoreForm.score) < 0 || Number(scoreForm.score) > 100)) return ElMessage.warning('成绩需在 0 至 100 之间')
  try { await academicApi.saveScore({ ...scoreForm }); scoreDialog.value = false; ElMessage.success('成绩已保存'); await loadScores() }
  catch (error) { showError(error) }
}
async function loadStatistics(courseId) {
  try { statistics.value = await academicApi.statistics(courseId) } catch (error) { showError(error) }
}
async function saveProfile() {
  try { await academicApi.updateProfile(profile); ElMessage.success('个人资料已保存') } catch (error) { showError(error) }
}
async function updatePassword() {
  if (!passwordForm.oldPassword || !passwordForm.newPassword) return ElMessage.warning('请填写当前密码和新密码')
  if (passwordForm.newPassword.length < 6) return ElMessage.warning('新密码至少需要 6 位')
  if (passwordForm.newPassword !== passwordForm.confirmPassword) return ElMessage.warning('两次输入的新密码不一致')
  try { await academicApi.changePassword(passwordForm); Object.assign(passwordForm, { oldPassword: '', newPassword: '', confirmPassword: '' }); ElMessage.success('密码已更新，请重新登录') }
  catch (error) { showError(error) }
}
watch(() => props.section, (section) => { if (section === 'courses') loadCourses(); if (section === 'scores') loadScores(); if (section === 'profile') loadProfile() })
onMounted(() => { if (props.section === 'courses') loadCourses(); if (props.section === 'scores') loadScores(); if (props.section === 'profile') loadProfile() })
</script>

<template>
  <section class="academic-page">
    <div class="page-heading"><div><div class="eyebrow">ACADEMIC SERVICES</div><h1>{{ heading }}</h1><p>高校教务信息一览，数据以系统记录为准。</p></div><div class="term-chip">2026—2027 学年 · 第一学期</div></div>

    <template v-if="section === 'courses'">
      <div class="panel"><div class="panel-heading"><div><h2>课程列表</h2><p>共 {{ courseTotal }} 条课程记录</p></div><el-button v-if="isAdmin" type="primary" @click="openCourse()">＋ 新建课程</el-button></div>
        <div class="toolbar"><el-input v-model="courseQuery.keyword" clearable placeholder="搜索课程名称或教师" @keyup.enter="courseQuery.page = 1; loadCourses()" @clear="loadCourses"/><el-input v-model="courseQuery.semester" clearable placeholder="按学期筛选" @keyup.enter="loadCourses"/><el-button @click="courseQuery.page = 1; loadCourses()">筛选</el-button></div>
        <el-table :data="courses" v-loading="busy" class="data-table"><el-table-column prop="courseName" label="课程名称" min-width="180"/><el-table-column prop="teacherName" label="授课教师" min-width="120"/><el-table-column label="开设班级" min-width="160"><template #default="{ row }">{{ row.className || '未指定' }}</template></el-table-column><el-table-column prop="credit" label="学分" width="90"/><el-table-column prop="semester" label="学期" min-width="140"/><el-table-column prop="courseTime" label="上课时间" min-width="140"/><el-table-column v-if="isAdmin" label="操作" width="150" fixed="right"><template #default="{ row }"><el-button link type="primary" @click="openCourse(row)">编辑</el-button><el-button link type="danger" @click="removeCourse(row)">删除</el-button></template></el-table-column><template #empty><span class="empty">暂无课程，请调整搜索条件或检查服务状态。</span></template></el-table>
        <div class="pagination"><span>共 {{ courseTotal }} 条</span><el-pagination v-model:current-page="courseQuery.page" v-model:page-size="courseQuery.size" :total="courseTotal" layout="prev, pager, next" @current-change="loadCourses"/></div>
      </div>
    </template>

    <template v-else-if="section === 'scores'">
      <div class="panel"><div class="panel-heading"><div><h2>成绩记录</h2><p>{{ isTeacher ? '仅显示本人任教课程的成绩' : '按课程、学期或学生检索' }}</p></div><el-button v-if="isAdmin || isTeacher" type="primary" @click="openScore">＋ 录入成绩</el-button></div>
        <div class="toolbar"><el-input v-model="scoreQuery.keyword" clearable placeholder="课程、学生姓名或学号" @keyup.enter="scoreQuery.page = 1; loadScores()" @clear="loadScores"/><el-input v-model="scoreQuery.courseId" clearable placeholder="课程 ID" @keyup.enter="loadScores"/><el-button @click="scoreQuery.page = 1; loadScores()">筛选</el-button></div>
        <el-table :data="scores" v-loading="busy" class="data-table"><el-table-column prop="courseName" label="课程" min-width="160"/><el-table-column prop="studentName" label="学生" min-width="110"/><el-table-column prop="studentNo" label="学号" min-width="130"/><el-table-column prop="score" label="成绩" width="100"><template #default="{ row }">{{ row.score ?? '—' }}</template></el-table-column><el-table-column label="状态" width="100"><template #default="{ row }">{{ ['正常', '缺考', '缓考'][Number(row.status)] || '未录入' }}</template></el-table-column><el-table-column prop="examTime" label="考试日期" min-width="120"/><el-table-column v-if="isAdmin || isTeacher" label="课程统计" width="110"><template #default="{ row }"><el-button link type="primary" @click="loadStatistics(row.courseId)">查看统计</el-button></template></el-table-column><template #empty><span class="empty">暂无成绩记录。</span></template></el-table>
        <div class="pagination"><span>共 {{ scoreTotal }} 条</span><el-pagination v-model:current-page="scoreQuery.page" v-model:page-size="scoreQuery.size" :total="scoreTotal" layout="prev, pager, next" @current-change="loadScores"/></div>
      </div>
      <div v-if="statistics" class="panel stats-panel"><div class="panel-heading"><div><h2>{{ statistics.courseName }} · 成绩概况</h2><p>统计结果由成绩服务计算</p></div><el-button text @click="statistics = null">收起</el-button></div><div class="metrics"><div><small>平均分</small><b>{{ Number(statistics.avgScore || 0).toFixed(1) }}</b></div><div><small>最高分</small><b>{{ statistics.maxScore ?? '—' }}</b></div><div><small>最低分</small><b>{{ statistics.minScore ?? '—' }}</b></div><div><small>及格率</small><b>{{ Math.round(Number(statistics.passRate || 0) * 100) }}%</b></div></div><div class="distribution"><span v-for="item in statistics.distribution || []" :key="item.segment">{{ item.segment }}<b>{{ item.count }}</b></span></div></div>
    </template>

    <template v-else>
      <div class="profile-layout"><article class="panel profile-card"><div class="profile-avatar">{{ (profile.name || auth.name || '用').slice(0, 1) }}</div><h2>{{ profile.name || auth.name }}</h2><p>{{ roleLabels[profile.role || auth.role] || '账户' }}</p><div class="profile-meta"><span>登录账号</span><b>{{ profile.username || '—' }}</b></div><div v-if="profile.department" class="profile-meta"><span>所属学院</span><b>{{ profile.department }}</b></div><div v-if="profile.className" class="profile-meta"><span>行政班级</span><b>{{ profile.className }}</b></div></article>
        <div><article class="panel form-panel"><div class="panel-heading"><div><h2>联系资料</h2><p>更新手机、邮箱和联系地址</p></div></div><el-form label-position="top" class="form-grid"><el-form-item label="手机号"><el-input v-model="profile.phone" maxlength="11" placeholder="常用手机号"/></el-form-item><el-form-item label="电子邮箱"><el-input v-model="profile.email" placeholder="name@example.edu.cn"/></el-form-item><el-form-item class="wide" label="联系地址"><el-input v-model="profile.address" placeholder="选填"/></el-form-item></el-form><el-button type="primary" @click="saveProfile">保存资料</el-button></article>
        <article class="panel form-panel security-panel"><div class="panel-heading"><div><h2>账户安全</h2><p>修改密码后请使用新密码重新登录</p></div></div><el-form label-position="top" class="form-grid"><el-form-item label="当前密码"><el-input v-model="passwordForm.oldPassword" type="password" show-password autocomplete="current-password"/></el-form-item><el-form-item label="新密码"><el-input v-model="passwordForm.newPassword" type="password" show-password autocomplete="new-password"/></el-form-item><el-form-item class="wide" label="确认新密码"><el-input v-model="passwordForm.confirmPassword" type="password" show-password autocomplete="new-password"/></el-form-item></el-form><el-button @click="updatePassword">更新密码</el-button></article></div></div>
    </template>

    <el-dialog v-model="courseDialog" :title="editingId ? '编辑课程' : '新建课程'" width="520px"><el-form label-position="top" class="dialog-form"><el-form-item label="课程名称" required><el-input v-model="courseForm.courseName" placeholder="例如：数据结构"/></el-form-item><div class="dialog-grid"><el-form-item label="学分"><el-input-number v-model="courseForm.credit" :min="0.5" :max="10" :step="0.5"/></el-form-item><el-form-item label="任课教师"><el-select v-model="courseForm.teacherId" clearable filterable placeholder="选择教师"><el-option v-for="item in teachers" :key="item.id" :label="`${item.name}（${item.teacherNo}）`" :value="item.id"/></el-select></el-form-item></div><el-form-item label="开设班级"><el-select v-model="courseForm.classId" clearable filterable placeholder="选择班级（仅该班学生可见）"><el-option v-for="item in classes" :key="item.id" :label="item.className" :value="item.id"/></el-select></el-form-item><el-form-item label="学期"><el-input v-model="courseForm.semester" placeholder="2026-2027-1"/></el-form-item><el-form-item label="上课时间"><el-input v-model="courseForm.courseTime" placeholder="例如：周一 1-2 节"/></el-form-item></el-form><template #footer><el-button @click="courseDialog = false">取消</el-button><el-button type="primary" @click="saveCourse">保存课程</el-button></template></el-dialog>
    <el-dialog v-model="scoreDialog" title="录入成绩" width="480px"><el-form label-position="top" class="dialog-form"><div class="dialog-grid"><el-form-item label="课程" required><el-select v-model="scoreForm.courseId" filterable placeholder="选择课程"><el-option v-for="item in courses" :key="item.id" :label="item.courseName" :value="item.id"/></el-select></el-form-item><el-form-item label="学生" required><el-select v-model="scoreForm.studentId" filterable placeholder="选择学生"><el-option v-for="item in students" :key="item.id" :label="`${item.name}（${item.studentNo}）`" :value="item.id"/></el-select></el-form-item></div><el-form-item label="成绩状态"><el-select v-model="scoreForm.status"><el-option label="正常" :value="0"/><el-option label="缺考" :value="1"/><el-option label="缓考" :value="2"/></el-select></el-form-item><el-form-item v-if="Number(scoreForm.status) === 0" label="成绩（0–100）"><el-input-number v-model="scoreForm.score" :min="0" :max="100" :precision="1"/></el-form-item><el-form-item label="考试日期"><el-input v-model="scoreForm.examTime" type="date"/></el-form-item></el-form><template #footer><el-button @click="scoreDialog = false">取消</el-button><el-button type="primary" @click="saveScore">保存成绩</el-button></template></el-dialog>
  </section>
</template>

<style scoped>
.academic-page{max-width:1280px;margin:auto;color:#202a3b}.page-heading{display:flex;align-items:flex-end;justify-content:space-between;margin:8px 0 25px}.eyebrow{font-size:10px;letter-spacing:1.4px;font-weight:700;color:#7887ee;margin-bottom:8px}.page-heading h1{font-size:26px;margin:0;font-weight:700}.page-heading p,.panel-heading p{font-size:12px;color:#929bad;margin:7px 0 0}.term-chip{padding:8px 12px;border-radius:8px;background:#f7f8fc;color:#69748a;font-size:11px}.panel{background:#fff;border:1px solid #edf0f5;border-radius:13px;padding:20px 20px 14px;margin-bottom:17px}.panel-heading{display:flex;justify-content:space-between;align-items:center;margin-bottom:17px}.panel-heading h2{font-size:15px;margin:0;color:#263247}.panel-heading p{font-size:11px}.toolbar{display:flex;gap:10px;margin-bottom:15px}.toolbar>.el-input{max-width:230px}.toolbar :deep(.el-input__wrapper),.form-grid :deep(.el-input__wrapper){box-shadow:0 0 0 1px #e8ecf3 inset;border-radius:8px;min-height:36px}.data-table{--el-table-border-color:#f0f2f7;--el-table-header-bg-color:#f8f9fc;--el-table-row-hover-bg-color:#fafbff;--el-table-text-color:#536075;--el-table-header-text-color:#8993a5;font-size:12px}.data-table :deep(th.el-table__cell){font-size:11px;font-weight:600;height:43px}.data-table :deep(td.el-table__cell){height:55px}.empty{color:#a0a8b6;font-size:11px}.pagination{display:flex;align-items:center;justify-content:space-between;padding-top:13px;color:#9aa3b2;font-size:11px}.profile-layout{display:grid;grid-template-columns:245px minmax(0,1fr);gap:17px;align-items:start}.profile-card{text-align:center;padding-top:25px}.profile-avatar{height:58px;width:58px;border-radius:18px;background:linear-gradient(145deg,#596ff0,#8997f8);display:grid;place-items:center;color:white;font-size:23px;font-weight:700;margin:auto auto 12px}.profile-card h2{font-size:16px;margin:0}.profile-card>p{font-size:11px;color:#929bad;margin:6px 0 18px}.profile-meta{display:flex;justify-content:space-between;border-top:1px solid #f0f2f6;padding:12px 0;font-size:10px}.profile-meta span{color:#98a1b0}.profile-meta b{font-weight:600;color:#4c586d}.form-panel{padding-bottom:20px}.form-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:0 15px}.form-grid :deep(.el-form-item){margin-bottom:12px}.form-grid :deep(.el-form-item__label){font-size:11px;color:#69758a;padding-bottom:5px}.wide{grid-column:1/-1}.security-panel{margin-top:15px}.metrics{display:grid;grid-template-columns:repeat(4,1fr);gap:10px}.metrics>div{background:#f8f9fc;border-radius:9px;padding:13px}.metrics small,.metrics b{display:block}.metrics small{font-size:10px;color:#929bad}.metrics b{font-size:19px;margin-top:6px}.distribution{display:flex;gap:9px;flex-wrap:wrap;margin-top:14px}.distribution span{display:flex;gap:10px;background:#f7f8fc;padding:7px 10px;border-radius:7px;color:#7d8797;font-size:10px}.distribution b{color:#5267e8}.dialog-form :deep(.el-form-item__label){font-size:11px}.dialog-form :deep(.el-input__wrapper){box-shadow:0 0 0 1px #e8ecf3 inset;border-radius:8px}.dialog-form :deep(.el-select){width:100%}.dialog-grid{display:grid;grid-template-columns:1fr 1fr;gap:15px}.academic-page :deep(.el-button){border-radius:8px}.academic-page :deep(.el-button--primary){--el-color-primary:#596ce8}.academic-page :deep(.el-dialog){border-radius:14px}.academic-page :deep(.el-dialog__title){font-size:16px;font-weight:600}
@media(max-width:700px){.page-heading{align-items:flex-start;gap:12px;flex-direction:column}.page-heading h1{font-size:23px}.term-chip{font-size:10px}.panel{padding:16px 12px 12px}.toolbar{flex-wrap:wrap}.toolbar>.el-input{max-width:none;flex:1 1 150px}.data-table{min-width:630px}.panel{overflow:hidden}.pagination{gap:8px;flex-wrap:wrap}.profile-layout{grid-template-columns:1fr}.profile-card{text-align:left}.profile-avatar{margin:0 0 12px}.metrics{grid-template-columns:repeat(2,1fr)}.dialog-grid{gap:8px}}
</style>
