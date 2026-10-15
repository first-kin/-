# B 模块接口清单（学生 / 教师 / 院系与班级）

> 全局约定：所有接口需携带 `Authorization: Bearer <token>`，经 `JwtFilter` 校验；统一响应格式 `{code, msg, data}`，`code=0` 表示成功；`?` 标注的字段为可空（数据库 NULL 时不出现在 JSON 中）。错误码：1001 参数校验失败、1004 无权限、1005 资源不存在、1006 数据重复、1007 业务规则冲突、5000 服务器内部错误。

| 模块 | 方法 | 路径 | 权限 | 请求参数 | 返回字段 | 说明 |
| ---- | ---- | ---- | ---- | ---- | ---- | ---- |
| 学生 | GET | /api/students | admin、teacher | Query：page、size、keyword、classId | data.total / page / size / list[id, studentNo, name, gender, className, phone?, email?] | 分页列表；keyword 按学号/姓名/班级模糊匹配（已转义）；teacher 仅见本人任教班级 |
| 学生 | GET | /api/students/{id} | admin、teacher、student(仅本人) | Path：id | data[id, studentNo, name, gender, birthDate?, classId, className, phone?, email?, address?] | 学生详情；student 角色仅可查看本人档案 |
| 学生 | POST | /api/students | admin | Body：studentNo、name、gender、birthDate?、classId、phone?、email?、address? | data.id | 新增学生；自动创建登录账号（用户名=学号，初始密码=学号后6位，BCrypt）；学号重复返回 1006；事务保证 users+students 原子写入 |
| 学生 | PUT | /api/students/{id} | admin | Body：name、gender、birthDate?、classId、phone?、email?、address? | data=null | 编辑学生；学号不可修改；可空字段传空串写入 NULL；记录不存在返回 1005 |
| 学生 | DELETE | /api/students/{id} | admin | Path：id | data=null | 逻辑删除（is_deleted=1）；影响 0 行返回 1005 |
| 学生 | POST | /api/students/import | admin | multipart/form-data：file | data.total / success / failed / errors[] | CSV 批量导入，列序 studentNo,name,gender,birthDate,classId,phone,email,address；逐行校验学号重复/性别/班级，每行事务独立提交 |
| 学生 | GET | /api/students/export | admin | Query：keyword、classId | CSV 文件流（text/csv; charset=utf-8） | 按当前筛选条件导出带 BOM 的 CSV；文件名 students.csv |
| 教师 | GET | /api/teachers | admin、teacher | Query：page、size、keyword、deptId | data.total / page / size / list[id, teacherNo, name, gender, title?, deptId, deptName, phone?, email?] | 分页列表；keyword 按工号/姓名模糊匹配（已转义）；teacher 仅见本人 |
| 教师 | GET | /api/teachers/{id} | admin、teacher(仅本人) | Path：id | data[id, teacherNo, name, gender, title?, deptId, deptName, phone?, email?] | 教师详情 |
| 教师 | POST | /api/teachers | admin | Body：teacherNo、name、gender、title?、deptId、phone?、email? | data.id | 新增教师；自动创建登录账号（用户名=工号，初始密码=工号后6位，BCrypt）；工号/用户名重复返回 1006；事务保证 users+teachers 原子写入 |
| 教师 | PUT | /api/teachers/{id} | admin | Body：name、gender、title?、deptId、phone?、email? | data=null | 编辑教师；工号不可修改；可空字段传空串写入 NULL |
| 教师 | DELETE | /api/teachers/{id} | admin | Path：id | data=null | 逻辑删除；仍为班主任或存在任教课程时返回 1007（提示含任教课程名） |
| 院系 | GET | /api/departments | 登录用户 | 无 | data[{id, deptName}] | 院系列表（非分页） |
| 院系 | POST | /api/departments | admin | Body：deptName | data.id | 新增院系；名称重复返回 1006 |
| 院系 | PUT | /api/departments/{id} | admin | Body：deptName | data=null | 编辑院系；名称重复返回 1006 |
| 院系 | DELETE | /api/departments/{id} | admin | Path：id | data=null | 物理删除；仍有关联教师或专业时返回 1007 |
| 班级 | GET | /api/classes | admin、teacher | Query：majorId | data[{id, className, majorId, grade, headTeacherId?}] | 班级列表（非分页），可按专业筛选 |
| 班级 | POST | /api/classes | admin | Body：className、majorId、grade、headTeacherId? | data.id | 新增班级；同年级同名返回 1006；专业/班主任不存在返回 1005 |
| 班级 | PUT | /api/classes/{id} | admin | Body：className、majorId、grade、headTeacherId? | data=null | 编辑班级；校验规则同新增 |
| 班级 | DELETE | /api/classes/{id} | admin | Path：id | data=null | 物理删除；班级下仍有在读学生时返回 1007 |

> 数据结构应用说明：LinkedList 暂存学生/班级查询行（头插后倒序回填）、SeqList 作分页与导入导出缓冲、MyStack 收集教师任教课程（删除校验）、MyQueue 保序输出院系列表。