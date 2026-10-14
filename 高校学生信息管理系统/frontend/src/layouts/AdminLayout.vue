<template>
  <div>
    <header>
      <h2>高校学生信息管理系统</h2>
    </header>

    <main>
      <RouterView />
    </main>
  </div>
</template>
<style scoped>
header {
  padding: 16px 24px;
  background: #409eff;
  color: white;
}

header h2 {
  margin: 0;
}

main {
  padding: 24px;
}
</style>