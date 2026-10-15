
# 前端运行说明

本项目使用 Vue 3、Vite、Vue Router、Pinia、Axios 和 Element Plus。

## 第一次运行

在 VS Code 打开项目后，打开终端并进入 `frontend` 文件夹：

```powershell
cd .\frontend
npm.cmd install
```

如果终端路径已经以 `frontend>` 结尾，跳过 `cd` 那一行。

`npm.cmd` 可在 Windows PowerShell 默认禁止运行脚本时使用。

## 演示登录

没有启动后端时，在当前 PowerShell 窗口输入：

```powershell
$env:VITE_MOCK_AUTH = "true"
npm.cmd run dev
```

浏览器打开 Vite 显示的本地地址（通常是 `http://localhost:5173`）。演示账号如下，密码均为 `123456`：

| 角色 | 用户名 |
| --- | --- |
| 管理员 | `demo-admin` |
| 教师 | `demo-teacher` |
| 学生 | `demo-student` |

演示数据保存在前端内存中，刷新后会恢复初始数据。演示登录只在 Vite 开发模式中启用，不是后端账号，也不能用于正式环境。管理员可从顶部导航进入学生管理、教师管理和院系/班级管理；学生、教师列表支持搜索和分页，演示模式支持档案 CRUD，学生列表可导出 CSV。真实后端模式下，学生导出使用接口约定的 Excel 文件流。

要停止开发服务器，在终端按 `Ctrl+C`。再次启动时，如果仍在同一个 PowerShell 窗口，演示模式设置会保留。

## 真实后端联调

先启动监听 `8080` 端口的后端，再在前端终端执行：

```powershell
$env:VITE_MOCK_AUTH = "false"
$env:VITE_USE_MOCK = "false"
npm.cmd run dev
```

Vite 会把 `/api` 请求代理到 `http://localhost:8080`。当前仓库的认证控制器和 JWT 过滤器尚未实现，因此真实登录需要等后端完成后再联调。关闭该终端并重新打开后，临时环境变量会清除。

## 检查代码

```powershell
npm.cmd test
npm.cmd run build
```

测试覆盖登录状态恢复、过期会话清理、角色校验、请求携带 Token、未登录响应处理，以及学生、教师、院系和班级管理的演示数据 CRUD、唯一值校验、组合筛选、分页、空结果和 CSV 编码/转义。
