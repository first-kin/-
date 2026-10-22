import { createRouter, createWebHistory } from 'vue-router'
import LoginView from '../views/LoginView.vue'
import AdminHome from '../views/AdminHome.vue'
import AdminLayout from '../layouts/AdminLayout.vue'
import StudentView from '../views/StudentView.vue'
import ModulePending from '../views/ModulePending.vue'
import AdminStudents from '../views/AdminStudents.vue'
import AdminTeachers from '../views/AdminTeachers.vue'
import AdminOrganization from '../views/AdminOrganization.vue'
import AcademicModule from '../views/AcademicModule.vue'
import { homeForRole, useAuthStore } from '../stores/auth'
import TeacherHome from '../views/TeacherHome.vue'

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
      children: [
        { path: '', component: AdminHome },
        { path: 'students', component: AdminStudents },
        { path: 'teachers', component: AdminTeachers },
        { path: 'organization', component: AdminOrganization },
        { path: 'courses', component: AcademicModule, props: { section: 'courses' } },
        { path: 'scores', component: AcademicModule, props: { section: 'scores' } },
        { path: 'profile', component: AcademicModule, props: { section: 'profile' } },
      ],
    },
    {
      path: '/teacher',
      component: AdminLayout,
      props: { homePath: '/teacher', sectionTitle: '教师首页' },
      meta: { requiresAuth: true, roles: ['teacher'] },
      children: [
        { path: '', component: TeacherHome },
        { path: 'courses', component: AcademicModule, props: { section: 'courses' } },
        { path: 'scores', component: AcademicModule, props: { section: 'scores' } },
        { path: 'profile', component: AcademicModule, props: { section: 'profile' } },
      ],
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
