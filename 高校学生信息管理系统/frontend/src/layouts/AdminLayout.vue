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
  gap: 24px;
  padding: 16px 24px;
  background: #ecf5ff;
}

nav a {
  color: #303133;
  text-decoration: none;
}

nav a.router-link-exact-active {
  color: #409eff;
  font-weight: bold;
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
