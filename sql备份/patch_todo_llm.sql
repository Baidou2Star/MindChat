SET NAMES utf8mb4;

CREATE TABLE IF NOT EXISTS `todo_item` (
  `todo_id` bigint UNSIGNED NOT NULL AUTO_INCREMENT,
  `uid` int NOT NULL,
  `source_thread_id` bigint UNSIGNED NOT NULL DEFAULT 0,
  `source_message_id` bigint UNSIGNED NOT NULL DEFAULT 0,
  `source_text` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL,
  `title` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `event_text` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `location` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL DEFAULT '',
  `time_text` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL DEFAULT '',
  `start_time` datetime NULL DEFAULT NULL,
  `end_time` datetime NULL DEFAULT NULL,
  `status` tinyint NOT NULL DEFAULT 0 COMMENT '0=pending,1=done',
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`todo_id`) USING BTREE,
  INDEX `idx_uid_created`(`uid` ASC, `created_at` DESC) USING BTREE,
  INDEX `idx_uid_status`(`uid` ASC, `status` ASC) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
