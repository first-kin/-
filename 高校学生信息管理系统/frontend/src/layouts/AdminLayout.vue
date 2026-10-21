<template>
  <div>
    <header>
      <div>
        <h2>高校学生信息管理系统</h2>
        <small>{{ currentSectionTitle }}</small>
      </div>
      <button type="button" @click="handleLogout">退出登录</button>
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
  if (props.homePath !== '/admin') return props.sectionTitle
  return {
    '/admin': '管理员首页',
    '/admin/students': '学生管理',
    '/admin/teachers': '教师管理',
    '/admin/organization': '院系 / 班级管理',
    '/admin/courses': '课程管理',
    '/admin/scores': '成绩管理',
    '/admin/profile': '个人中心',
    '/teacher/courses': '课程管理',
    '/teacher/scores': '成绩管理',
    '/teacher/profile': '个人中心',
  }[route.path] || props.sectionTitle
})

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
  padding: 16px 24px;
  background: #409eff;
  color: white;
}

header h2 {
  margin: 0;
}

header small {
  display: block;
  margin-top: 4px;
  opacity: 0.8;
}

main {
  padding: 24px;
}
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

nav a:last-child {
  border-right: none;
}

nav a:hover {
  background: #d9ecff;
}

nav a.router-link-exact-active {
  background: #ffffff;
  color: #409eff;
  font-weight: bold;
  box-shadow: inset 0 -2px 0 #409eff;
}
header button {
  padding: 8px 16px;
  border: 1px solid white;
  border-radius: 6px;
  background: transparent;
  color: white;
  cursor: pointer;
}

header button:hover {
  background: rgb(255 255 255 / 20%);
}
</style>
