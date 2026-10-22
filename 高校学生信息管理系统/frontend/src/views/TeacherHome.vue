<template>
  <section class="teacher-home">
    <!-- 欢迎条 -->
    <div class="welcome">
      <div class="eyebrow">TEACHER PORTAL</div>
      <h1>欢迎回来，{{ profile.name || '老师' }}</h1>
      <p>今日 {{ today }} · 本学期已登记授课 {{ stats.courseCount || 0 }} 门，覆盖学生 {{ stats.studentCount || 0 }} 人</p>
    </div>

    <!-- 快捷统计 -->
    <div class="stats-grid">
      <div class="stat-card blue">
        <div class="stat-icon">📚</div>
        <div>
          <strong>{{ stats.courseCount || 0 }}</strong>
          <small>本学期授课课程</small>
        </div>
      </div>
      <div class="stat-card green">
        <div class="stat-icon">👥</div>
        <div>
          <strong>{{ stats.studentCount || 0 }}</strong>
          <small>覆盖学生人数</small>
        </div>
      </div>
      <div class="stat-card orange">
        <div class="stat-icon">📝</div>
        <div>
          <strong>{{ stats.gradedCount || 0 }}</strong>
          <small>已录入成绩条数</small>
        </div>
      </div>
      <div class="stat-card purple">
        <div class="stat-icon">⭐</div>
        <div>
          <strong>{{ stats.avgScore || '—' }}</strong>
          <small>所授课程平均分</small>
        </div>
      </div>
    </div>

    <!-- 两大块：我的课程 + 快捷操作 -->
    <div class="main-grid">
      <div class="panel courses-panel">
        <div class="panel-head">
          <h2>我执教的课程</h2>
          <RouterLink to="/teacher/courses" class="see-all">查看全部 ›</RouterLink>
        </div>
        <div v-if="myCourses.length" class="course-list">
          <div v-for="c in myCourses" :key="c.id" class="course-item">
            <div class="course-name">{{ c.courseName }}</div>
            <div class="course-meta">
              <span>🎓 {{ c.className || '未指定班级' }}</span>
              <span>⏱ {{ c.courseTime || '时间待定' }}</span>
              <span>⭐ {{ c.credit }} 学分</span>
            </div>
          </div>
        </div>
        <div v-else class="empty-hint">暂无授课课程，请联系教务管理员分配。</div>
      </div>

      <div class="panel action-panel">
        <div class="panel-head">
          <h2>快捷操作</h2>
          <span class="sub">一键直达常用功能</span>
        </div>
        <div class="action-grid">
          <RouterLink to="/teacher/scores" class="action-btn primary">
            <div class="action-icon">📝</div>
            <div>
              <strong>录入成绩</strong>
              <small>为所授课程录入/发布成绩</small>
            </div>
          </RouterLink>
          <RouterLink to="/teacher/courses" class="action-btn">
            <div class="action-icon">📖</div>
            <div>
              <strong>我的课程</strong>
              <small>查看本学期所有授课安排</small>
            </div>
          </RouterLink>
          <RouterLink to="/teacher/profile" class="action-btn">
            <div class="action-icon">👤</div>
            <div>
              <strong>个人中心</strong>
              <small>更新联系方式、修改密码</small>
            </div>
          </RouterLink>
          <RouterLink :to="{ path: '/teacher/scores' }" class="action-btn">
            <div class="action-icon">📅</div>
            <div>
              <strong>最近考试</strong>
              <small>{{ latestExam }}</small>
            </div>
          </RouterLink>
        </div>
      </div>
    </div>
  </section>
</template>

<script setup>
import { computed, onMounted, ref } from 'vue'
import { useAuthStore } from '../stores/auth'
import request from '../api/request'
import { academicApi } from '../api/academic'

const auth = useAuthStore()
const profile = ref({ name: auth.name || '老师' })
const myCourses = ref([])
const stats = ref({})
const latestExam = ref('')

const today = computed(() => {
  const d = new Date()
  const days = ['日','一','二','三','四','五','六']
  return `${d.getFullYear()}年${d.getMonth()+1}月${d.getDate()}日 星期${days[d.getDay()]}`
})

async function loadProfile() {
  try {
    const data = await request.get('/users/me')
    if (data?.name) profile.value.name = data.name
  } catch {}
}

async function loadStats() {
  try {
    const now = new Date()
    const semester = `${now.getFullYear()}-${String(now.getFullYear()+1).slice(2)}-1`
    // 1. 我的课程
    const courses = await academicApi.courses({ page: 1, size: 50 })
    myCourses.value = courses?.list || []

    // 2. 成绩统计
    let gradedCount = 0
    let totalScore = 0
    let scoreCnt = 0
    const scores = await academicApi.scores({ page: 1, size: 500 })
    const list = Array.isArray(scores) ? scores : (scores?.list || [])
    list.forEach(s => {
      gradedCount++
      if (s.score != null && s.status === 0) { totalScore += Number(s.score); scoreCnt++ }
    })

    // 3. 覆盖学生数（每个课程的学生数）——简化：用成绩列表的学生去重数
    const students = new Set(list.map(s => s.studentId).filter(Boolean))

    stats.value = {
      courseCount: myCourses.value.length,
      studentCount: students.size,
      gradedCount,
      avgScore: scoreCnt ? (totalScore / scoreCnt).toFixed(1) : '—',
    }

    // 4. 最近考试日期
    const withExam = list.filter(s => s.examTime).sort((a,b) => b.examTime.localeCompare(a.examTime))
    latestExam.value = withExam[0]?.examTime || '暂无考试记录'
  } catch (e) {
    console.warn('teacher home stats load failed', e)
  }
}

onMounted(() => {
  loadProfile()
  loadStats()
})
</script>

<style scoped>
.teacher-home { max-width: 1200px; margin: 0 auto; }
.welcome { margin: 8px 0 24px; }
.eyebrow { color: #67c23a; font-size: 12px; font-weight: 700; letter-spacing: 1.4px; }
h1 { margin: 8px 0; color: #202b3c; font-size: 28px; }
.welcome p { margin: 0; color: #7a8494; }

.stats-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
  gap: 16px;
  margin-bottom: 24px;
}
.stat-card {
  display: flex; align-items: center; gap: 16px;
  padding: 18px 22px; border-radius: 14px; background: #fff;
  border: 1px solid #ecf0f6; transition: transform .2s;
}
.stat-card:hover { transform: translateY(-2px); }
.stat-icon {
  width: 48px; height: 48px; border-radius: 12px;
  display: grid; place-items: center; font-size: 22px;
}
.stat-card strong { display: block; font-size: 24px; color: #202b3c; }
.stat-card small { color: #818b9b; font-size: 13px; }
.blue   .stat-icon { background: #edf4ff; }
.green  .stat-icon { background: #e8f8f4; }
.orange .stat-icon { background: #fef4e6; }
.purple .stat-icon { background: #f2edff; }

.main-grid {
  display: grid;
  grid-template-columns: 1.3fr 1fr;
  gap: 20px;
}
@media (max-width: 900px) { .main-grid { grid-template-columns: 1fr; } }

.panel {
  background: #fff; border-radius: 14px; padding: 22px;
  border: 1px solid #ecf0f6;
}
.panel-head {
  display: flex; justify-content: space-between; align-items: center;
  margin-bottom: 18px;
}
.panel-head h2 { margin: 0; font-size: 18px; color: #202b3c; }
.panel-head .sub { color: #9aa4b2; font-size: 13px; }
.see-all { color: #409eff; text-decoration: none; font-size: 13px; }

.course-list { display: flex; flex-direction: column; gap: 10px; }
.course-item {
  padding: 14px 16px; border-radius: 10px; background: #f7f9fc;
  border-left: 3px solid #67c23a;
}
.course-name { font-weight: 600; color: #202b3c; }
.course-meta {
  margin-top: 6px; display: flex; gap: 18px;
  flex-wrap: wrap; color: #7a8494; font-size: 12px;
}
.empty-hint { color: #9aa4b2; padding: 20px 0; text-align: center; }

.action-grid {
  display: grid; grid-template-columns: 1fr 1fr; gap: 12px;
}
.action-btn {
  display: flex; align-items: flex-start; gap: 12px;
  padding: 16px; border-radius: 10px; text-decoration: none;
  border: 1px solid #ecf0f6; background: #fafbff; color: inherit;
  transition: transform .15s, border-color .15s;
}
.action-btn:hover { transform: translateY(-2px); border-color: #91caff; }
.action-btn.primary { background: linear-gradient(135deg,#67c23a,#529b2e); color: #fff; border-color: transparent; }
.action-btn.primary small { color: rgba(255,255,255,.85); }
.action-icon { font-size: 20px; }
.action-btn strong { display: block; font-size: 14px; }
.action-btn small { display: block; margin-top: 4px; color: #818b9b; font-size: 12px; line-height: 1.4; }
</style>
