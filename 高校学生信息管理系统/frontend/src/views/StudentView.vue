<template>
<div class="app-shell">
  <aside class="sidebar" :class="{collapsed}">
    <div class="brand"><div class="brand-mark">知</div><div v-if="!collapsed"><strong>知行</strong><small>校园教务平台</small></div><button class="collapse" @click="collapsed=!collapsed">‹</button></div>
    <div class="nav-caption" v-if="!collapsed">教学事务</div>
    <button v-for="item in menus" :key="item.key" class="nav-item" :class="{active:active===item.key}" @click="active=item.key">
      <span class="nav-icon">{{item.icon}}</span><span v-if="!collapsed">{{item.label}}</span><span v-if="!collapsed && item.key==='scores'" class="nav-dot"></span>
    </button>
    <div class="sidebar-bottom"><div class="help-icon">?</div><span v-if="!collapsed">需要帮助？</span></div>
  </aside>
  <main class="main-area">
    <header class="topbar"><div class="breadcrumb">工作台 <span>/</span> {{currentTitle}}</div><div class="top-actions"><span class="term-tag">2026-2027 学年 · 第一学期</span><button class="icon-button" title="通知">♧<i></i></button><div class="user-mini"><div class="avatar">夏</div><div><b>夏同学</b><small>学生账户</small></div><span>⌄</span></div><button class="logout-button" type="button" @click="handleLogout">退出登录</button></div></header>
    <section class="page-content">
      <template v-if="active==='courses'">
        <div class="page-heading"><div><div class="eyebrow">ACADEMIC SERVICES</div><h1>课程管理</h1><p>查看当前学期课程安排，快速掌握课程信息与上课地点。</p></div><el-button type="primary" plain @click="exportCourses">导出课程表 ↗</el-button></div>
        <div class="stats-grid">
          <div class="stat-card"><div class="stat-label">本学期课程 <span class="stat-symbol blue">▤</span></div><div class="stat-value">{{courseList.length}}<small> 门</small></div><div class="stat-foot">已为你整理本学期课程</div></div>
          <div class="stat-card"><div class="stat-label">累计学分 <span class="stat-symbol violet">✧</span></div><div class="stat-value">{{courseCredits}}<small> 分</small></div><div class="stat-foot">按当前列表课程计算</div></div>
          <div class="stat-card"><div class="stat-label">授课教师 <span class="stat-symbol green">♙</span></div><div class="stat-value">{{teacherCount}}<small> 位</small></div><div class="stat-foot">不同教师人数</div></div>
          <div class="stat-card"><div class="stat-label">正在开课 <span class="stat-symbol orange">◷</span></div><div class="stat-value">{{courseList.filter(c=>c.status==='开课中').length}}<small> 门</small></div><div class="stat-foot">课程状态实时展示</div></div>
        </div>
        <div class="content-card">
          <div class="card-heading"><div><h2>课程列表</h2><p>共 {{filteredCourses.length}} 门课程 · 支持课程名称、编号及教师检索</p></div><el-button type="primary" @click="openCourse()">＋ 新增课程</el-button></div>
          <div class="toolbar"><el-input v-model="courseKeyword" clearable placeholder="搜索课程名称、课程编号或教师..." class="search-input"><template #prefix>⌕</template></el-input><el-select v-model="courseStatus" style="width:140px"><el-option label="全部状态" value="全部状态"/><el-option label="开课中" value="开课中"/><el-option label="已结课" value="已结课"/></el-select><span class="toolbar-spacer"></span><span class="muted">最近更新：今天</span></div>
          <el-table :data="pagedCourses" style="width:100%" row-class-name="table-row" empty-text="没有找到匹配的课程">
            <el-table-column label="课程信息" min-width="230"><template #default="{row}"><div class="course-cell"><div class="course-icon" :class="iconClass(row.code)">{{row.name.slice(0,1)}}</div><div><b>{{row.name}}</b><small>{{row.code}}</small></div></div></template></el-table-column>
            <el-table-column prop="teacher" label="授课教师" min-width="105"/>
            <el-table-column prop="credit" label="学分" width="78"><template #default="{row}"><span class="credit">{{row.credit}} 学分</span></template></el-table-column>
            <el-table-column prop="time" label="上课时间" min-width="130"/>
            <el-table-column prop="room" label="上课地点" min-width="125"/>
            <el-table-column label="课程状态" width="100"><template #default="{row}"><span class="status-pill" :class="row.status==='开课中'?'success':'neutral'"><i></i>{{row.status}}</span></template></el-table-column>
            <el-table-column label="操作" width="115" fixed="right"><template #default="{row}"><el-button link type="primary" @click="openCourse(row)">编辑</el-button><el-button link type="danger" @click="removeCourse(row)">删除</el-button></template></el-table-column>
          </el-table>
          <div class="table-footer"><span>展示第 {{filteredCourses.length?((page-1)*pageSize+1):0}}–{{Math.min(page*pageSize,filteredCourses.length)}} 条，共 {{filteredCourses.length}} 条</span><el-pagination v-model:current-page="page" v-model:page-size="pageSize" :page-sizes="[5,10,20]" layout="prev, pager, next, sizes" :total="filteredCourses.length" small/></div>
        </div>
        <div class="bottom-note"><span class="note-icon">i</span><div><b>温馨提示</b><p>课程安排如有疑问，请联系授课教师或学院教务办公室。课程数据以教务系统最终发布信息为准。</p></div></div>
      </template>
      <template v-else-if="active==='scores'">
        <div class="page-heading"><div><div class="eyebrow">ACADEMIC PERFORMANCE</div><h1>成绩管理</h1><p>查看个人课程成绩与学业表现，及时了解每门课程的学习进度。</p></div><el-button plain @click="exportScores">导出成绩单 ↗</el-button></div>
        <div class="stats-grid">
          <div class="stat-card"><div class="stat-label">已发布成绩 <span class="stat-symbol blue">▤</span></div><div class="stat-value">{{scoreList.length}}<small> 门</small></div><div class="stat-foot">当前可查询的课程成绩</div></div>
          <div class="stat-card"><div class="stat-label">平均成绩 <span class="stat-symbol violet">↗</span></div><div class="stat-value">{{averageScore}}<small> 分</small></div><div class="stat-foot">按课程总评计算</div></div>
          <div class="stat-card"><div class="stat-label">已获学分 <span class="stat-symbol green">✧</span></div><div class="stat-value">{{earnedCredits}}<small> 分</small></div><div class="stat-foot">按总评 60 分及以上统计</div></div>
          <div class="stat-card"><div class="stat-label">当前最高分 <span class="stat-symbol orange">☆</span></div><div class="stat-value">{{highestScore}}<small> 分</small></div><div class="stat-foot">保持状态，继续加油</div></div>
        </div>
        <div class="content-card"><div class="card-heading"><div><h2>我的成绩单</h2><p>成绩仅供学习参考，最终以学校正式成绩为准</p></div><el-tag type="success" effect="light" round>成绩已同步</el-tag></div>
          <div class="toolbar"><el-input v-model="scoreKeyword" clearable placeholder="搜索课程名称或课程编号..." class="search-input"><template #prefix>⌕</template></el-input><el-select v-model="scoreTerm" style="width:190px"><el-option label="全部学期" value="全部学期"/><el-option v-for="t in terms" :key="t" :label="t" :value="t"/></el-select><span class="toolbar-spacer"></span><span class="muted">共 {{filteredScores.length}} 条记录</span></div>
          <el-table :data="filteredScores" style="width:100%" empty-text="暂无成绩记录"><el-table-column label="课程" min-width="230"><template #default="{row}"><div class="course-cell"><div class="course-icon score-icon">{{row.courseName.slice(0,1)}}</div><div><b>{{row.courseName}}</b><small>{{row.courseCode}} · {{row.term}}</small></div></div></template></el-table-column><el-table-column prop="teacher" label="授课教师" min-width="100"/><el-table-column prop="usual" label="平时成绩" width="100"/><el-table-column prop="exam" label="期末成绩" width="100"/><el-table-column label="课程总评" width="120"><template #default="{row}"><div class="score-total" :class="scoreClass(row.total)">{{row.total}}<span> / 100</span></div></template></el-table-column><el-table-column prop="credit" label="学分" width="80"/><el-table-column label="状态" width="100"><template #default="{row}"><span class="status-pill success"><i></i>{{row.status}}</span></template></el-table-column></el-table>
          <div class="table-footer"><span>共 {{filteredScores.length}} 条成绩记录</span><span class="muted">成绩计算规则以课程大纲为准</span></div>
        </div>
        <div class="bottom-note"><span class="note-icon">i</span><div><b>成绩说明</b><p>如发现成绩有误，请在成绩发布后及时联系任课教师核实。演示数据仅用于界面展示，接入后端后将显示真实成绩。</p></div></div>
      </template>
      <template v-else>
        <div class="page-heading"><div><div class="eyebrow">PERSONAL WORKSPACE</div><h1>个人中心</h1><p>管理你的个人资料与账户安全设置。</p></div><el-button type="primary" @click="saveProfile" :loading="saving">保存修改</el-button></div>
        <div class="profile-layout"><div class="profile-card"><div class="profile-cover"></div><div class="profile-main"><div class="profile-avatar">夏</div><h2>{{profile.name||'同学'}}</h2><p>{{profile.major}} · {{profile.className}}</p><el-tag type="success" effect="light" round>在校学生</el-tag><div class="profile-divider"></div><div class="profile-meta"><span>学号</span><b>{{profile.studentNo}}</b></div><div class="profile-meta"><span>所属学院</span><b>{{profile.department}}</b></div><div class="profile-meta"><span>入学年级</span><b>{{profile.grade}}</b></div><div class="profile-tip">保持资料准确，有助于教务信息及时送达。</div></div></div>
        <div class="profile-right"><div class="content-card form-card"><div class="card-heading"><div><h2>基本资料</h2><p>请确保联系方式真实有效</p></div><span class="muted">个人信息</span></div><el-form :model="profile" label-position="top" class="profile-form"><div class="form-grid"><el-form-item label="姓名"><el-input v-model="profile.name" placeholder="请输入姓名"/></el-form-item><el-form-item label="学号"><el-input v-model="profile.studentNo" disabled/></el-form-item><el-form-item label="性别"><el-select v-model="profile.gender" style="width:100%"><el-option label="男" value="男"/><el-option label="女" value="女"/></el-select></el-form-item><el-form-item label="联系电话"><el-input v-model="profile.phone" placeholder="请输入联系电话"/></el-form-item><el-form-item label="电子邮箱"><el-input v-model="profile.email" placeholder="请输入电子邮箱"/></el-form-item><el-form-item label="所在学院"><el-input v-model="profile.department" disabled/></el-form-item><el-form-item label="专业"><el-input v-model="profile.major" disabled/></el-form-item><el-form-item label="行政班级"><el-input v-model="profile.className" disabled/></el-form-item></div></el-form></div>
        <div class="content-card security-card"><div class="card-heading"><div><h2>账户安全</h2><p>定期更新密码，保护账户安全</p></div><div class="security-shield">⌑</div></div><el-form :model="passwordForm" label-position="top" class="password-form"><el-form-item label="当前密码"><el-input v-model="passwordForm.oldPassword" type="password" show-password placeholder="请输入当前密码"/></el-form-item><div class="form-grid"><el-form-item label="新密码"><el-input v-model="passwordForm.newPassword" type="password" show-password placeholder="至少 6 位"/></el-form-item><el-form-item label="确认新密码"><el-input v-model="passwordForm.confirmPassword" type="password" show-password placeholder="再次输入新密码"/></el-form-item></div><el-button @click="changePassword" :loading="changing">更新密码</el-button><span class="muted password-hint">演示模式下原密码为 123456</span></el-form></div></div></div>
      </template>
      <footer class="footer">© 2026 知行校园教务管理平台 <span>让每一次学习，都有迹可循。</span></footer>
    </section>
  </main>
  <el-dialog v-model="courseDialog" :title="editing?'编辑课程':'新增课程'" width="560px" destroy-on-close><el-form ref="courseFormRef" :model="courseForm" :rules="courseRules" label-position="top"><div class="form-grid"><el-form-item label="课程编号" prop="code"><el-input v-model="courseForm.code" placeholder="如 SE203"/></el-form-item><el-form-item label="课程名称" prop="name"><el-input v-model="courseForm.name" placeholder="请输入课程名称"/></el-form-item><el-form-item label="授课教师" prop="teacher"><el-input v-model="courseForm.teacher" placeholder="请输入教师姓名"/></el-form-item><el-form-item label="学分" prop="credit"><el-input-number v-model="courseForm.credit" :min="0.5" :max="10" :step="0.5" style="width:100%"/></el-form-item><el-form-item label="学期" prop="term"><el-input v-model="courseForm.term"/></el-form-item><el-form-item label="上课时间" prop="time"><el-input v-model="courseForm.time" placeholder="如 周一 1-2 节"/></el-form-item><el-form-item label="上课地点" prop="room"><el-input v-model="courseForm.room" placeholder="如 教 2-301"/></el-form-item></div></el-form><template #footer><el-button @click="courseDialog=false">取消</el-button><el-button type="primary" @click="submitCourse">确认保存</el-button></template></el-dialog>
</div>
</template>
<script setup>
import { computed, onMounted, reactive, ref, watch } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage, ElMessageBox } from 'element-plus'
import { api } from '../api'
import { logout } from '../api/auth'
import { useAuthStore } from '../stores/auth'
const router = useRouter()
const auth = useAuthStore()
function handleLogout() {
  const token = auth.token
  auth.clearSession()
  void logout(token).catch(() => {})
  void router.replace('/login')
}
const menus=[{key:'courses',label:'课程管理',icon:'▤'},{key:'scores',label:'成绩管理',icon:'▥'},{key:'profile',label:'个人中心',icon:'♙'}]
const active=ref('courses'),collapsed=ref(false),courseList=ref([]),scoreList=ref([]),profile=reactive({}),saving=ref(false),changing=ref(false)
const currentTitle=computed(()=>menus.find(m=>m.key===active.value)?.label||'工作台')
const courseKeyword=ref(''),courseStatus=ref('全部状态'),page=ref(1),pageSize=ref(5),scoreKeyword=ref(''),scoreTerm=ref('全部学期')
const filteredCourses=computed(()=>courseList.value.filter(c=>(courseStatus.value==='全部状态'||c.status===courseStatus.value)&&(!courseKeyword.value||`${c.name}${c.code}${c.teacher}`.toLowerCase().includes(courseKeyword.value.toLowerCase()))))
const pagedCourses=computed(()=>filteredCourses.value.slice((page.value-1)*pageSize.value,page.value*pageSize.value))
const courseCredits=computed(()=>courseList.value.reduce((s,c)=>s+Number(c.credit||0),0).toFixed(1).replace(/\.0$/,''))
const teacherCount=computed(()=>new Set(courseList.value.map(c=>c.teacher)).size)
const terms=computed(()=>[...new Set(scoreList.value.map(s=>s.term))])
const filteredScores=computed(()=>scoreList.value.filter(s=>(!scoreKeyword.value||`${s.courseName}${s.courseCode}`.toLowerCase().includes(scoreKeyword.value.toLowerCase()))&&(scoreTerm.value==='全部学期'||s.term===scoreTerm.value)))
const averageScore=computed(()=>scoreList.value.length?(scoreList.value.reduce((s,r)=>s+r.total,0)/scoreList.value.length).toFixed(1):'—')
const earnedCredits=computed(()=>scoreList.value.filter(s=>s.total>=60).reduce((s,r)=>s+r.credit,0).toFixed(1).replace(/\.0$/,''))
const highestScore=computed(()=>scoreList.value.length?Math.max(...scoreList.value.map(s=>s.total)):'—')
const courseDialog=ref(false),editing=ref(false),courseFormRef=ref()
const blankCourse=()=>({code:'',name:'',teacher:'',credit:2,term:'2026-2027-1',time:'',room:''})
const courseForm=reactive(blankCourse())
const courseRules={code:[{required:true,message:'请填写课程编号',trigger:'blur'}],name:[{required:true,message:'请填写课程名称',trigger:'blur'}],teacher:[{required:true,message:'请填写授课教师',trigger:'blur'}],credit:[{required:true,message:'请填写学分',trigger:'change'}],term:[{required:true,message:'请填写学期',trigger:'blur'}]}
const passwordForm=reactive({oldPassword:'',newPassword:'',confirmPassword:''})
function iconClass(code){return ['SE','CS'].some(x=>code.startsWith(x))?'course-blue':code.startsWith('MA')?'course-violet':code.startsWith('EN')?'course-orange':'course-green'}
function scoreClass(n){return n>=90?'excellent':n>=80?'good':n>=60?'pass':'fail'}
async function load(){try{courseList.value=await api.courses();scoreList.value=await api.scores();Object.assign(profile,await api.profile())}catch(e){ElMessage.error(e.message||'加载数据失败')}}
function openCourse(row){editing.value=!!row;Object.assign(courseForm,blankCourse(),row||{});courseDialog.value=true}
async function submitCourse(){try{await courseFormRef.value.validate();if(editing.value)await api.updateCourse({...courseForm});else await api.addCourse({...courseForm});courseList.value=await api.courses();courseDialog.value=false;ElMessage.success('课程信息已保存')}catch(e){if(e!==false)ElMessage.error(e.message||'请检查表单内容')}}
async function removeCourse(row){try{await ElMessageBox.confirm(`确定删除课程「${row.name}」吗？此操作不可撤销。`,'删除确认',{type:'warning',confirmButtonText:'确认删除',cancelButtonText:'取消'});await api.deleteCourse(row.id);courseList.value=await api.courses();ElMessage.success('课程已删除')}catch(e){if(e!=='cancel'&&e!=='close')ElMessage.error(e.message||'删除失败')}}
function downloadCsv(name,rows){if(!rows.length){ElMessage.warning('暂无可导出的数据');return}const csv='\ufeff'+[Object.keys(rows[0]).join(','),...rows.map(r=>Object.values(r).map(v=>`"${String(v??'').replaceAll('"','""')}"`).join(','))].join('\r\n');const url=URL.createObjectURL(new Blob([csv],{type:'text/csv;charset=utf-8;'}));const a=document.createElement('a');a.href=url;a.download=name;a.click();URL.revokeObjectURL(url);ElMessage.success('导出文件已生成')}
function exportCourses(){downloadCsv('课程列表.csv',filteredCourses.value)}function exportScores(){downloadCsv('个人成绩单.csv',filteredScores.value)}
async function saveProfile(){saving.value=true;try{await api.updateProfile({...profile});ElMessage.success('个人资料保存成功')}catch(e){ElMessage.error(e.message)}finally{saving.value=false}}
async function changePassword(){if(!passwordForm.oldPassword||!passwordForm.newPassword){ElMessage.warning('请完整填写密码信息');return}if(passwordForm.newPassword.length<6){ElMessage.warning('新密码至少 6 位');return}if(passwordForm.newPassword!==passwordForm.confirmPassword){ElMessage.warning('两次输入的新密码不一致');return}changing.value=true;try{await api.changePassword({...passwordForm});Object.assign(passwordForm,{oldPassword:'',newPassword:'',confirmPassword:''});ElMessage.success('密码更新成功。接入真实后端后应重新登录')}catch(e){ElMessage.error(e.message)}finally{changing.value=false}}
watch([courseKeyword,courseStatus],()=>page.value=1)
onMounted(load)
</script>

<style scoped>
.logout-button {
  border: 1px solid #e3e7f0;
  border-radius: 7px;
  padding: 7px 11px;
  background: #fff;
  color: #667188;
  font-size: 12px;
  white-space: nowrap;
  cursor: pointer;
}

.logout-button:hover {
  border-color: #596ce8;
  background: #f7f8ff;
  color: #596ce8;
}
</style>
