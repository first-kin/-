<template>
  <main class="login-page">
    <section class="login-card">
      <h1>高校学生信息管理系统</h1>
      <p class="subtitle">请登录后继续</p>

      <form @submit.prevent="handleLogin">
        <label>
          用户名
          <input v-model="username" type="text" placeholder="请输入用户名" />
        </label>

        <label>
          密码
          <input v-model="password" type="password" placeholder="请输入密码" />
        </label>

        <button type="submit" :disabled="loading">
  {{ loading ? '正在登录…' : '登录' }}
</button>

        <p v-if="message" class="message">{{ message }}</p>
      </form>
    </section>
  </main>
</template>

<script setup>
import { ref } from 'vue'
import { login } from '../api/auth'
import { useRouter } from 'vue-router'

const router = useRouter()

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
  message.value = '正在登录，请稍候……'

  try {
    const result = await login(username.value.trim(), password.value)

    if (!result?.token) {
      message.value = '登录接口未返回凭证，请联系后端同学'
      return
    }

    const pages = {
      admin: '/admin',
      student: '/student'
    }

    const target = pages[result.role]

    if (!target) {
      message.value = result.role === 'teacher'
        ? '教师页面尚未完成，暂时无法进入'
        : '登录接口返回的角色无法识别'
      return
    }

    localStorage.setItem('token', result.token)
    message.value = ''
    await router.replace(target)
    } catch (error) {
    message.value = error.response?.status === 500
      ? '登录服务暂时异常，请联系后端同学检查'
      : error.message || '登录失败，请稍后重试'
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
