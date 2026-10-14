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

        <button type="submit">登录</button>

        <p v-if="message" class="message">{{ message }}</p>
      </form>
    </section>
  </main>
</template>

<script setup>
import { ref } from 'vue'

const username = ref('')
const password = ref('')
const message = ref('')

function handleLogin() {
  if (!username.value || !password.value) {
    message.value = '请输入用户名和密码'
    return
  }

  message.value = `欢迎你，${username.value}`
}
</script>

<style>
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
  color: #67c23a;
  text-align: center;
}
</style>