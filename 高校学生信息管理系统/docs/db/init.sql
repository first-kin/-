-- ============================================================
-- 高校学生信息管理系统 数据库初始化脚本
-- MySQL 8.0 / InnoDB / utf8mb4
-- ============================================================
CREATE DATABASE IF NOT EXISTS student_mgmt
  DEFAULT CHARSET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE student_mgmt;

-- 1. 用户登录表（三种角色共用）
CREATE TABLE users (
  id              BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  username        VARCHAR(50)  NOT NULL COMMENT '登录名',
  password        VARCHAR(200) NOT NULL COMMENT 'PBKDF2-HMAC-SHA256散列串:pbkdf2$iter$salt$hash',
  role            ENUM('admin','teacher','student') NOT NULL COMMENT '角色',
  status          TINYINT      NOT NULL DEFAULT 1 COMMENT '1启用 0禁用',
  last_login_time DATETIME     NULL,
  create_time     DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
  update_time     DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  is_deleted      TINYINT      NOT NULL DEFAULT 0,
  UNIQUE KEY uk_username (username)
) ENGINE=InnoDB COMMENT='用户登录表';

-- 2. 院系表
CREATE TABLE departments (
  id          BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  dept_name   VARCHAR(100) NOT NULL COMMENT '院系名称',
  create_time DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
  UNIQUE KEY uk_dept_name (dept_name)
) ENGINE=InnoDB COMMENT='院系表';

-- 3. 教师表（依赖 users、departments）
CREATE TABLE teachers (
  id          BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  user_id     BIGINT UNSIGNED NOT NULL UNIQUE COMMENT '关联登录账号',
  teacher_no  VARCHAR(20)  NOT NULL COMMENT '工号',
  name        VARCHAR(50)  NOT NULL,
  gender      ENUM('男','女') NOT NULL DEFAULT '男',
  title       VARCHAR(50)  NULL COMMENT '职称:教授/副教授/讲师/助教',
  dept_id     BIGINT UNSIGNED NOT NULL COMMENT '所属院系',
  phone       VARCHAR(20)  NULL,
  email       VARCHAR(100) NULL,
  create_time DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
  update_time DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  is_deleted  TINYINT      NOT NULL DEFAULT 0,
  UNIQUE KEY uk_teacher_no (teacher_no),
  CONSTRAINT fk_teacher_user FOREIGN KEY (user_id) REFERENCES users(id),
  CONSTRAINT fk_teacher_dept FOREIGN KEY (dept_id) REFERENCES departments(id)
) ENGINE=InnoDB COMMENT='教师表';

-- 4. 专业表（依赖 departments）
CREATE TABLE majors (
  id          BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  major_name  VARCHAR(100) NOT NULL,
  dept_id     BIGINT UNSIGNED NOT NULL,
  create_time DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
  UNIQUE KEY uk_major (major_name, dept_id),
  CONSTRAINT fk_major_dept FOREIGN KEY (dept_id) REFERENCES departments(id)
) ENGINE=InnoDB COMMENT='专业表';

-- 5. 班级表（依赖 majors、teachers）
CREATE TABLE classes (
  id               BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  class_name       VARCHAR(100) NOT NULL COMMENT '如:计算机2401班',
  major_id         BIGINT UNSIGNED NOT NULL,
  grade            INT          NOT NULL COMMENT '入学年级,如2024',
  head_teacher_id  BIGINT UNSIGNED NULL COMMENT '班主任',
  create_time      DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
  UNIQUE KEY uk_class (class_name, grade),
  CONSTRAINT fk_class_major FOREIGN KEY (major_id) REFERENCES majors(id),
  CONSTRAINT fk_class_head  FOREIGN KEY (head_teacher_id) REFERENCES teachers(id)
) ENGINE=InnoDB COMMENT='班级表';

-- 6. 学生表（依赖 users、classes）
CREATE TABLE students (
  id          BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  user_id     BIGINT UNSIGNED NOT NULL UNIQUE COMMENT '关联登录账号',
  student_no  VARCHAR(20)  NOT NULL COMMENT '学号',
  name        VARCHAR(50)  NOT NULL,
  gender      ENUM('男','女') NOT NULL DEFAULT '男',
  birth_date  DATE         NULL,
  class_id    BIGINT UNSIGNED NOT NULL,
  phone       VARCHAR(20)  NULL,
  email       VARCHAR(100) NULL,
  address     VARCHAR(200) NULL,
  photo_url   VARCHAR(255) NULL,
  create_time DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
  update_time DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  is_deleted  TINYINT      NOT NULL DEFAULT 0,
  UNIQUE KEY uk_student_no (student_no),
  KEY idx_student_class (class_id),
  CONSTRAINT fk_student_user  FOREIGN KEY (user_id)  REFERENCES users(id),
  CONSTRAINT fk_student_class FOREIGN KEY (class_id) REFERENCES classes(id)
) ENGINE=InnoDB COMMENT='学生表';

-- 7. 课程表（依赖 teachers）
CREATE TABLE courses (
  id          BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  course_name VARCHAR(100) NOT NULL,
  credit      DECIMAL(3,1) NOT NULL COMMENT '学分0.5~10',
  teacher_id  BIGINT UNSIGNED NULL COMMENT '授课教师',
  class_id    BIGINT UNSIGNED NULL COMMENT '开设班级，该班学生可见此课程',
  semester    VARCHAR(20)  NULL COMMENT '如2025-2026-1',
  course_time VARCHAR(100) NULL COMMENT '上课时间地点描述',
  create_time DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
  update_time DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  is_deleted  TINYINT      NOT NULL DEFAULT 0,
  UNIQUE KEY uk_course_name (course_name),
  KEY idx_course_teacher (teacher_id),
  KEY idx_course_class (class_id),
  CONSTRAINT fk_course_teacher FOREIGN KEY (teacher_id) REFERENCES teachers(id),
  CONSTRAINT fk_course_class FOREIGN KEY (class_id) REFERENCES classes(id)
) ENGINE=InnoDB COMMENT='课程表';

-- 8. 成绩表（学生-课程的M:N联系,依赖 students、courses）
CREATE TABLE scores (
  id          BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  student_id  BIGINT UNSIGNED NOT NULL,
  course_id   BIGINT UNSIGNED NOT NULL,
  score       DECIMAL(5,1) NULL COMMENT '0~100,NULL表示未录入',
  status      TINYINT      NOT NULL DEFAULT 0 COMMENT '0正常 1缺考 2缓考',
  exam_time   DATE         NULL COMMENT '考试日期',
  create_time DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
  update_time DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  UNIQUE KEY uk_student_course (student_id, course_id),
  KEY idx_score_course (course_id),
  CONSTRAINT fk_score_student FOREIGN KEY (student_id) REFERENCES students(id),
  CONSTRAINT fk_score_course  FOREIGN KEY (course_id)  REFERENCES courses(id)
) ENGINE=InnoDB COMMENT='成绩表';

-- 9. 通知公告表
CREATE TABLE notices (
  id           BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  title        VARCHAR(200) NOT NULL,
  content      TEXT         NOT NULL,
  publisher_id BIGINT UNSIGNED NOT NULL COMMENT '发布人user_id',
  is_top       TINYINT      NOT NULL DEFAULT 0 COMMENT '1置顶',
  create_time  DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
  update_time  DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  is_deleted   TINYINT      NOT NULL DEFAULT 0,
  KEY idx_notice_pub (publisher_id)
) ENGINE=InnoDB COMMENT='通知公告表';

-- 10. 操作日志表
CREATE TABLE logs (
  id          BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  user_id     BIGINT UNSIGNED NULL,
  action      VARCHAR(200) NOT NULL COMMENT '操作描述',
  ip          VARCHAR(45)  NULL COMMENT '兼容IPv6',
  create_time DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
  KEY idx_log_user (user_id),
  KEY idx_log_time (create_time)
) ENGINE=InnoDB COMMENT='操作日志表';

-- ============================================================
-- 初始管理员账号 admin
-- 密码为 password 的 PBKDF2-HMAC-SHA256 散列(固定盐种子,仅初始账号)
-- 格式: pbkdf2$<迭代次数>$<盐hex>$<摘要hex>；首次登录后请立即修改密码
-- 如需自定义初始密码,用后端 util::hashPassword 生成后替换下方字符串
-- ============================================================
INSERT INTO users (username, password, role)
VALUES ('admin', 'pbkdf2$10000$3fa9b2c8d4e5f60718293a4b5c6d7e8f$d8502eac06243c7411dc71a33951663e2d299b6500f91cbccdff0bfccba38a4f', 'admin');