import { createRouter, createWebHistory } from 'vue-router'
import LoginView from '../views/LoginView.vue'
import AdminHome from '../views/AdminHome.vue'
import AdminLayout from '../layouts/AdminLayout.vue'
import StudentView from '../views/StudentView.vue'

const router = createRouter({
  history: createWebHistory(),
  routes: [
    { path: '/', redirect: '/login' },
    { path: '/login', component: LoginView },
    { path: '/student', component: StudentView },
    {
      path: '/admin',
      component: AdminLayout,
      children: [
        { path: '', component: AdminHome },
      ],
    },
  ],
})

export default router
