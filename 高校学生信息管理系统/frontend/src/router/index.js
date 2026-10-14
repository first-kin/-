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
    {
  path: '/student',
  component: StudentView,
 meta: { requiresAuth: true, roles: ['student'] },
},
    {
      path: '/admin',
      component: AdminLayout,
     meta: { requiresAuth: true, roles: ['admin'] },
      children: [
        { path: '', component: AdminHome },
      ],
    },
  ],
})
router.beforeEach((to) => {
  const token = localStorage.getItem('token')
  const role = localStorage.getItem('role')

  if (to.meta.requiresAuth && !token) {
    return '/login'
  }

  if (to.meta.roles && !to.meta.roles.includes(role)) {
    if (role === 'admin') return '/admin'
    if (role === 'student') return '/student'
    return '/login'
  }
})
export default router
