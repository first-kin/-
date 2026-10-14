import { http } from './client'
const mock = import.meta.env.VITE_USE_MOCK !== 'false'
let courses = [
 {id:1,code:'SE201',name:'数据结构',teacher:'王老师',credit:4,term:'2026-2027-1',time:'周一 1-2 节',room:'教 2-301',status:'开课中'},
 {id:2,code:'SE202',name:'面向对象程序设计',teacher:'李老师',credit:3.5,term:'2026-2027-1',time:'周二 3-4 节',room:'实验楼 A204',status:'开课中'},
 {id:3,code:'MA103',name:'高等数学（上）',teacher:'陈老师',credit:5,term:'2026-2027-1',time:'周三 1-2 节',room:'教 1-105',status:'开课中'},
 {id:4,code:'EN101',name:'大学英语',teacher:'赵老师',credit:2,term:'2026-2027-1',time:'周五 5-6 节',room:'教 3-210',status:'已结课'}
]
let scores = [
 {id:1,courseCode:'SE201',courseName:'数据结构',teacher:'王老师',term:'2026-2027-1',usual:92,exam:88,total:90,credit:4,status:'已发布'},
 {id:2,courseCode:'SE202',courseName:'面向对象程序设计',teacher:'李老师',term:'2026-2027-1',usual:90,exam:94,total:92,credit:3.5,status:'已发布'},
 {id:3,courseCode:'MA103',courseName:'高等数学（上）',teacher:'陈老师',term:'2026-2027-1',usual:82,exam:79,total:80,credit:5,status:'已发布'},
 {id:4,courseCode:'EN101',courseName:'大学英语',teacher:'赵老师',term:'2026-2027-1',usual:95,exam:91,total:93,credit:2,status:'已发布'},
 {id:5,courseCode:'PH105',courseName:'大学物理',teacher:'孙老师',term:'2026-2027-1',usual:78,exam:72,total:74,credit:3,status:'已发布'}
]
let profile = {name:'夏同学',studentNo:'2025030227',gender:'男',department:'计算机学院',major:'软件工程',className:'软件2505班',grade:'2025级',phone:'138****2027',email:'student@example.edu.cn',address:'天津市',username:'2025030227'}
const clone = x => JSON.parse(JSON.stringify(x))
const wait = data => Promise.resolve(clone(data))
export const api = {
  courses: async params => mock ? wait(courses.filter(c => !params?.keyword || `${c.name}${c.code}${c.teacher}`.toLowerCase().includes(params.keyword.toLowerCase()))) : httpGet('/courses', {params}),
  addCourse: async data => { if (!mock) return httpPost('/courses', data); const item={...data,id:Date.now(),status:'开课中'}; courses.unshift(item); return clone(item) },
  updateCourse: async data => { if (!mock) return httpPut(`/courses/${data.id}`,data); courses=courses.map(c=>c.id===data.id?{...c,...data}:c); return clone(data) },
  deleteCourse: async id => { if (!mock) return httpDelete(`/courses/${id}`); courses=courses.filter(c=>c.id!==id); return true },
  scores: async params => mock ? wait(scores.filter(s => (!params?.keyword || `${s.courseName}${s.courseCode}`.toLowerCase().includes(params.keyword.toLowerCase())) && (!params?.term || s.term===params.term))) : httpGet('/scores',{params}),
  profile: async () => mock ? wait(profile) : httpGet('/users/me'),
  updateProfile: async data => { if (!mock) return httpPut('/users/me',data); profile={...profile,...data}; return clone(profile) },
  changePassword: async data => { if (!mock) return httpPost('/users/me/password',data); if (data.oldPassword !== '123456') throw new Error('原密码不正确（演示账号原密码为 123456）'); if(data.newPassword.length<6) throw new Error('新密码至少 6 位'); return true }
}
async function httpGet(url, config) { const {http}=await import('./client'); return http.get(url,config) }
async function httpPost(url, data) { const {http}=await import('./client'); return http.post(url,data) }
async function httpPut(url, data) { const {http}=await import('./client'); return http.put(url,data) }
async function httpDelete(url) { const {http}=await import('./client'); return http.delete(url) }
