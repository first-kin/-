-- Allow reusing a course name after its previous course was soft-deleted.
-- Run once against databases created before this change.
-- Active course names remain unique; deleted rows are excluded because the
-- generated value is NULL, and MySQL permits multiple NULLs in a unique key.
ALTER TABLE courses
  DROP INDEX uk_course_name,
  ADD COLUMN active_course_name VARCHAR(100)
    GENERATED ALWAYS AS (CASE WHEN is_deleted = 0 THEN course_name ELSE NULL END) STORED,
  ADD UNIQUE KEY uk_course_name (active_course_name);
