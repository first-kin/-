<template>
  <section class="admin-home">
    <div class="welcome">
      <div class="eyebrow">ADMINISTRATION</div>
      <h1>管理员工作台</h1>
      <p>维护校园基础信息，快速进入学生、教师及组织架构管理。</p>
    </div>
    <div class="module-grid">
      <RouterLink class="module-card" to="/admin/students">
        <span class="module-icon student-icon">学</span>
        <span><strong>学生管理</strong><small>学生档案查询、新增、编辑与删除</small></span>
        <span class="module-arrow">›</span>
      </RouterLink>
      <RouterLink class="module-card" to="/admin/teachers">
        <span class="module-icon teacher-icon">师</span>
        <span><strong>教师管理</strong><small>教师档案查询、新增、编辑与删除</small></span>
        <span class="module-arrow">›</span>
      </RouterLink>
      <RouterLink class="module-card" to="/admin/organization">
        <span class="module-icon org-icon">院</span>
        <span><strong>院系 / 班级</strong><small>维护院系、班级及班主任关联</small></span>
        <span class="module-arrow">›</span>
      </RouterLink>
    </div>
  </section>
</template>

<style scoped>
.admin-home { max-width: 1100px; margin: 0 auto; }
.welcome { margin: 12px 0 28px; }
.eyebrow { color: #409eff; font-size: 12px; font-weight: 700; letter-spacing: 1.4px; }
h1 { margin: 8px 0; color: #202b3c; font-size: 28px; }
.welcome p { margin: 0; color: #7a8494; }
.module-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 16px; }
.module-card { display: flex; align-items: center; gap: 14px; min-height: 104px; padding: 20px; border: 1px solid #e7ebf2; border-radius: 12px; background: #fff; color: inherit; text-decoration: none; transition: border-color .2s, transform .2s; }
.module-card:hover { transform: translateY(-2px); border-color: #91caff; }
.module-card strong, .module-card small { display: block; }
.module-card small { margin-top: 8px; color: #818b9b; line-height: 1.5; }
.module-icon { display: grid; flex: 0 0 42px; height: 42px; place-items: center; border-radius: 12px; font-weight: 700; }
.student-icon { color: #3478f6; background: #edf4ff; }
.teacher-icon { color: #16a085; background: #e8f8f4; }
.org-icon { color: #8a61d4; background: #f2edff; }
.module-arrow { margin-left: auto; color: #9aa4b2; font-size: 24px; }
</style>
