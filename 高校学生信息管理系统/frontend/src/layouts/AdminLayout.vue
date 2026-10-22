<template>
  <div>
    <header>
      <div class="brand">
        <div class="brand-logo">知</div>
        <div>
          <h2>知行 · 教务管理平台</h2>
          <small>{{ currentSectionTitle }}</small>
        </div>
      </div>
      <div class="user-area">
        <span class="term-tag">2026—2027 学年 · 第一学期</span>
        <div class="user-chip">
          <div class="user-avatar">{{ (auth.name || auth.username || '用').slice(0, 1) }}</div>
          <div class="user-info">
            <b>{{ auth.name || auth.username }}</b>
            <small>{{ roleLabel }}</small>
          </div>
        </div>
        <button type="button" @click="handleLogout">退出登录</button>
      </div>
    </header>
    <nav>
      <RouterLink :to="homePath">{{ sectionTitle }}</RouterLink>
      <template v-if="homePath === '/admin'">
        <RouterLink to="/admin/students">学生管理</RouterLink>
        <RouterLink to="/admin/teachers">教师管理</RouterLink>
        <RouterLink to="/admin/organization">院系/班级</RouterLink>
        <RouterLink to="/admin/courses">课程管理</RouterLink>
        <RouterLink to="/admin/scores">成绩管理</RouterLink>
        <RouterLink to="/admin/profile">个人中心</RouterLink>
      </template>
      <template v-else-if="homePath === '/teacher'">
        <RouterLink to="/teacher/courses">课程管理</RouterLink>
        <RouterLink to="/teacher/scores">成绩管理</RouterLink>
        <RouterLink to="/teacher/profile">个人中心</RouterLink>
      </template>
    </nav>
    <main>
      <RouterView />
    </main>
  </div>
</template>
<script setup>
import { computed } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { useAuthStore } from '../stores/auth'
import { logout } from '../api/auth'

const router = useRouter()
const route = useRoute()
const auth = useAuthStore()
const props = defineProps({
  homePath: { type: String, default: '/admin' },
  sectionTitle: { type: String, default: '管理员首页' },
})
const currentSectionTitle = computed(() => {
  if (props.homePath === '/teacher') {
    return {
      '/teacher': '教师工作台',
      '/teacher/courses': '我的课程',
      '/teacher/scores': '成绩录入',
      '/teacher/profile': '个人中心',
    }[route.path] || props.sectionTitle
  }
  return {
    '/admin': '管理员首页',
    '/admin/students': '学生管理',
    '/admin/teachers': '教师管理',
    '/admin/organization': '院系 / 班级管理',
    '/admin/courses': '课程管理',
    '/admin/scores': '成绩管理',
    '/admin/profile': '个人中心',
  }[route.path] || props.sectionTitle
})
const roleLabel = computed(() => ({ admin: '管理员', teacher: '任课教师' }[auth.role] || '未知'))

function handleLogout() {
  const token = auth.token
  auth.clearSession()
  void logout(token).catch(() => {})
  void router.replace('/login')
}
</script>
<style scoped>
header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 16px 28px;
  background: linear-gradient(90deg, #2b6cb0, #409eff);
  color: white;
}
.brand { display: flex; align-items: center; gap: 14px; }
.brand-logo {
  width: 42px; height: 42px; border-radius: 10px;
  background: rgba(255,255,255,.2);
  display: grid; place-items: center;
  font-size: 20px; font-weight: 700;
}
header h2 { margin: 0; font-size: 20px; }
header small { display: block; margin-top: 3px; opacity: .8; font-size: 12px; }
.user-area { display: flex; align-items: center; gap: 16px; }
.term-tag {
  padding: 5px 12px; border-radius: 20px;
  background: rgba(255,255,255,.18); font-size: 12px;
}
.user-chip {
  display: flex; align-items: center; gap: 10px;
  padding: 6px 14px 6px 6px; border-radius: 24px;
  background: rgba(255,255,255,.15);
}
.user-avatar {
  width: 32px; height: 32px; border-radius: 50%;
  background: #fff; color: #409eff;
  display: grid; place-items: center;
  font-weight: 700;
}
.user-info b { display: block; font-size: 13px; }
.user-info small { margin: 0; opacity: .75; font-size: 11px; }
main { padding: 24px; }
nav {
  display: flex;
  flex-wrap: wrap;
  gap: 0;
  padding: 0;
  background: #ecf5ff;
  border-top: 1px solid #a0cfff;
  border-bottom: 1px solid #a0cfff;
}

nav a {
  flex: 1 1 0;
  min-width: 96px;
  padding: 14px 8px;
  border-right: 1px solid #a0cfff;
  color: #303133;
  text-align: center;
  text-decoration: none;
}

nav a:last-child { border-right: none; }
nav a:hover { background: #d9ecff; }
nav a.router-link-exact-active {
  background: #ffffff;
  color: #409eff;
  font-weight: bold;
  box-shadow: inset 0 -2px 0 #409eff;
}
header button {
  padding: 8px 18px;
  border: 1px solid rgba(255,255,255,.6);
  border-radius: 6px;
  background: transparent;
  color: white;
  cursor: pointer;
  font-size: 13px;
}
header button:hover { background: rgba(255,255,255,.2); }
</style>
