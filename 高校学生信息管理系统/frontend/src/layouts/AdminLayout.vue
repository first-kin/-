<template>
  <div>
    <header>
      <div>
        <h2>高校学生信息管理系统</h2>
        <small>{{ sectionTitle }}</small>
      </div>
      <button type="button" @click="handleLogout">退出登录</button>
    </header>
<nav>
  <RouterLink :to="homePath">{{ sectionTitle }}</RouterLink>
 
</nav>
    <main>
      <RouterView />
    </main>
  </div>
</template>
<script setup>
import { useRouter } from 'vue-router'
import { useAuthStore } from '../stores/auth'
import { logout } from '../api/auth'

const router = useRouter()
const auth = useAuthStore()
defineProps({
  homePath: { type: String, default: '/admin' },
  sectionTitle: { type: String, default: '管理员首页' },
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
