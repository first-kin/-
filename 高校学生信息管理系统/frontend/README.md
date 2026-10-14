# 知行 · 高校学生信息管理系统前端（F 负责模块）

## 已实现内容
- Vue 3 + Vite + Element Plus，响应式管理后台布局。
- 课程管理：列表、关键词搜索、状态筛选、分页、新增/编辑/删除、CSV 导出。
- 成绩管理：成绩列表、课程搜索、学期筛选、平均分/最高分/已获学分统计、CSV 导出。
- 个人中心：基本资料编辑、资料保存、修改密码表单校验。
- Axios 请求封装、Bearer Token 请求头、Vite `/api` 代理。
- Mock 演示数据，方便后端未完成时本地演示。

## 启动
需要 Node.js 18 或更高版本。
```bash
cd frontend
npm install
cp .env.example .env
npm run dev
```
浏览器打开 Vite 输出的地址（通常为 `http://localhost:5173`）。

默认 `VITE_USE_MOCK=true`，界面用本地演示数据。注意：Mock 数据仅保存在当前页面运行内存中，刷新会恢复初始数据，不会写入数据库。演示模式的原密码为 `123456`。

## 接入真实后端
1. 将 `.env` 中 `VITE_USE_MOCK` 改为 `false`。
2. 确认后端地址可通过 Vite `/api` 代理访问；当前代理目标为 `http://localhost:8080`。
3. 根据后端实际 Controller 路由与 JSON 响应结构，修改 `src/api/index.js` 中的路径和字段映射。
4. 登录完成后将后端返回的 Token 写入 `localStorage.setItem('campus_token', token)`；当前项目没有实现登录页，因此不要把演示页面当作完整认证系统。
5. 与后端成员约定统一响应格式。当前 Axios 封装默认将 `response.data` 作为业务数据返回；若后端返回 `{code,msg,data}`，需在 `client.js` 中统一解包。
6. 与后端联调时确认：课程删除是否受已有成绩限制；学生是否有权限新增/编辑/删除课程；成绩端是否只返回当前登录学生自己的成绩；个人中心允许修改哪些字段；密码修改成功后是否使旧 Token 失效。

## 当前 API 路径约定（需要与后端核对）
| 功能 | 方法 | 路径 |
|---|---|---|
| 课程列表/筛选 | GET | `/courses` |
| 新增课程 | POST | `/courses` |
| 修改课程 | PUT | `/courses/:id` |
| 删除课程 | DELETE | `/courses/:id` |
| 成绩列表/筛选 | GET | `/scores` |
| 当前用户资料 | GET | `/users/me` |
| 修改个人资料 | PUT | `/users/me` |
| 修改密码 | POST | `/users/me/password` |

这些路径是前端暂定接口契约，不代表你们现有 C++ 后端已经实现。项目压缩包当前未包含完整前端，也未发现已完成的课程/成绩/用户 Controller，因此接入真实服务前需要团队一起核对。

## GitHub 协作建议
- 将 `frontend/` 作为独立前端目录提交，不要覆盖队友的 `backend/`、`docs/db/` 等文件。
- 建议分支：`feature/frontend-course-score-profile`。
- 提交前运行 `npm run build`，通过后再开 Pull Request。
- `.env` 不提交真实密钥或个人数据；提交 `.env.example` 即可。
- 如果队友已经创建前端工程，请先比较 `package.json`、路由和全局样式，再按模块移植 `src/App.vue`、`src/api/` 与 `src/styles/`，不要直接覆盖他人的入口文件。

## 设计说明
采用近期常见的轻量 SaaS 管理后台视觉：浅灰蓝画布、白色卡片、低饱和蓝紫主色、清晰留白、细分隔线、简洁数据卡片与轻量状态标签。参考了 Vue 3 + Element Plus 管理后台项目的组件化与页面组织方式，而非直接复制模板。
