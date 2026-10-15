import { createRouter, createWebHistory } from 'vue-router'
import LoginView from '../views/LoginView.vue'
import AdminHome from '../views/AdminHome.vue'
import AdminLayout from '../layouts/AdminLayout.vue'
import StudentView from '../views/StudentView.vue'
import ModulePending from '../views/ModulePending.vue'
import { homeForRole, useAuthStore } from '../stores/auth'

const router = createRouter({
  history: createWebHistory(),
  routes: [
    { path: '/', redirect: '/login' },
    { path: '/login', name: 'login', component: LoginView },
    {
      path: '/admin',
      component: AdminLayout,
      props: { homePath: '/admin', sectionTitle: '管理员首页' },
      meta: { requiresAuth: true, roles: ['admin'] },
      children: [{ path: '', component: AdminHome }],
    },
    {
      path: '/teacher',
      component: AdminLayout,
      props: { homePath: '/teacher', sectionTitle: '教师首页' },
      meta: { requiresAuth: true, roles: ['teacher'] },
      children: [{ path: '', component: ModulePending }],
    },
    {
      path: '/student',
      component: StudentView,
      meta: { requiresAuth: true, roles: ['student'] },
    },
    { path: '/:pathMatch(.*)*', redirect: '/' },
  ],
})

router.beforeEach((to) => {
  const auth = useAuthStore()
  const loggedIn = auth.hydrate()
  const home = homeForRole(auth.role)

  if (to.path === '/login' && loggedIn) return home
  if (to.meta.requiresAuth && !loggedIn) {
    return { path: '/login', query: { redirect: to.fullPath } }
  }
  if (to.meta.roles && !to.meta.roles.includes(auth.role)) return home
})

export default router
