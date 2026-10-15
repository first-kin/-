<template>
  <main class="login-page">
    <section class="login-card">
      <h1>高校学生信息管理系统</h1>
      <p class="subtitle">请登录后继续</p>
      <p v-if="demoMode" class="demo-hint">演示模式：demo-admin、demo-teacher 或 demo-student，密码 123456</p>

      <form @submit.prevent="handleLogin">
        <label>
          用户名
          <input v-model="username" type="text" autocomplete="username" placeholder="请输入用户名" />
        </label>

        <label>
          密码
          <input v-model="password" type="password" autocomplete="current-password" placeholder="请输入密码" />
        </label>

        <button type="submit" :disabled="loading">{{ loading ? '正在登录…' : '登录' }}</button>

        <p v-if="message" class="message" role="alert">{{ message }}</p>
      </form>
    </section>
  </main>
</template>

<script setup>
import { ref } from 'vue'
import { login } from '../api/auth'
import { useAuthStore, homeForRole } from '../stores/auth'
import { useRouter } from 'vue-router'

const router = useRouter()
const auth = useAuthStore()
const demoMode = import.meta.env.DEV && import.meta.env.VITE_MOCK_AUTH === 'true' && import.meta.env.VITE_USE_MOCK !== 'false'

const username = ref('')
const password = ref('')
const message = ref('')
const loading = ref(false)
async function handleLogin() {
  if (loading.value) return
  if (!username.value.trim() || !password.value) {
    message.value = '请输入用户名和密码'
    return
  }
  loading.value = true
  message.value = ''

  try {
    const result = await login(username.value.trim(), password.value)

    auth.setSession(result)
    const requestedTarget = typeof router.currentRoute.value.query.redirect === 'string'
      ? router.currentRoute.value.query.redirect
      : ''
    const target = requestedTarget.startsWith('/') && !requestedTarget.startsWith('//')
      ? requestedTarget
      : homeForRole(result.role)
    message.value = ''
    await router.replace(target)
  } catch (error) {
    message.value = error.message || '登录失败，请稍后重试'
  } finally {
    loading.value = false
  }
}
</script>

<style scoped>
* {
  box-sizing: border-box;
}

body {
  margin: 0;
  font-family: Arial, sans-serif;
}

.login-page {
  min-height: 100vh;
  display: flex;
  justify-content: center;
  align-items: center;
  background: #f2f5f9;
}

.login-card {
  width: 380px;
  padding: 36px;
  border-radius: 12px;
  background: white;
  box-shadow: 0 8px 30px rgb(0 0 0 / 12%);
}

h1 {
  margin: 0;
  color: #303133;
  text-align: center;
  font-size: 24px;
}

.subtitle {
  margin: 12px 0 28px;
  color: #909399;
  text-align: center;
}

.demo-hint {
  margin: -12px 0 22px;
  padding: 10px;
  border-radius: 6px;
  background: #ecf5ff;
  color: #426188;
  line-height: 1.5;
  font-size: 12px;
}

label {
  display: block;
  margin-bottom: 18px;
  color: #606266;
}

input {
  width: 100%;
  margin-top: 8px;
  padding: 11px;
  border: 1px solid #dcdfe6;
  border-radius: 6px;
  font-size: 14px;
}

button {
  width: 100%;
  padding: 11px;
  border: 0;
  border-radius: 6px;
  color: white;
  background: #409eff;
  font-size: 16px;
  cursor: pointer;
}

.message {
  margin-bottom: 0;
  color: #f56c6c;
  text-align: center;
}
</style>
