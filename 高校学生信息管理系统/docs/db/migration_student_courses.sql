-- Run once against databases created before per-student course enrollment.
-- The table intentionally starts empty: students see courses only after an
-- administrator assigns them in Student Management.
CREATE TABLE IF NOT EXISTS student_courses (
  student_id BIGINT UNSIGNED NOT NULL,
  course_id  BIGINT UNSIGNED NOT NULL,
  create_time DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (student_id, course_id),
  KEY idx_student_courses_course (course_id),
  CONSTRAINT fk_student_courses_student FOREIGN KEY (student_id) REFERENCES students(id),
  CONSTRAINT fk_student_courses_course FOREIGN KEY (course_id) REFERENCES courses(id)
) ENGINE=InnoDB COMMENT='学生选课关联表';
