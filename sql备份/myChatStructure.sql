/*
 Navicat Premium Data Transfer

 Source Server         : WSL2_MySQL
 Source Server Type    : MySQL
 Source Server Version : 80045 (8.0.45-0ubuntu0.24.04.1)
 Source Host           : 127.0.0.1:3306
 Source Schema         : myChat

 Target Server Type    : MySQL
 Target Server Version : 80045 (8.0.45-0ubuntu0.24.04.1)
 File Encoding         : 65001

 Date: 06/02/2026 20:59:30
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for chat_message
-- ----------------------------
DROP TABLE IF EXISTS `chat_message`;
CREATE TABLE `chat_message`  (
  `message_id` bigint UNSIGNED NOT NULL AUTO_INCREMENT,
  `thread_id` bigint UNSIGNED NOT NULL,
  `sender_id` bigint UNSIGNED NOT NULL,
  `recv_id` bigint UNSIGNED NOT NULL,
  `content` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `status` tinyint NOT NULL DEFAULT 0 COMMENT '0=未读 1=已读 2=撤回',
  PRIMARY KEY (`message_id`) USING BTREE,
  INDEX `idx_thread_created`(`thread_id` ASC, `created_at` ASC) USING BTREE,
  INDEX `idx_thread_message`(`thread_id` ASC, `message_id` ASC) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 25 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for chat_thread
-- ----------------------------
DROP TABLE IF EXISTS `chat_thread`;
CREATE TABLE `chat_thread`  (
  `id` bigint UNSIGNED NOT NULL AUTO_INCREMENT,
  `type` enum('private','group') CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 28 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for friend
-- ----------------------------
DROP TABLE IF EXISTS `friend`;
CREATE TABLE `friend`  (
  `id` int UNSIGNED NOT NULL AUTO_INCREMENT,
  `self_id` int NOT NULL,
  `friend_id` int NOT NULL,
  `back` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL DEFAULT '',
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `self_friend`(`self_id` ASC, `friend_id` ASC) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 239 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for friend_apply
-- ----------------------------
DROP TABLE IF EXISTS `friend_apply`;
CREATE TABLE `friend_apply`  (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `from_uid` int NOT NULL,
  `to_uid` int NOT NULL,
  `status` smallint NOT NULL DEFAULT 0,
  `descs` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL DEFAULT '',
  `back_name` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL DEFAULT '',
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `from_to_uid`(`from_uid` ASC, `to_uid` ASC) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 184 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for group_chat
-- ----------------------------
DROP TABLE IF EXISTS `group_chat`;
CREATE TABLE `group_chat`  (
  `thread_id` bigint UNSIGNED NOT NULL COMMENT '引用chat_thread.id',
  `name` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL DEFAULT NULL COMMENT '群聊名称',
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`thread_id`) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for group_chat_member
-- ----------------------------
DROP TABLE IF EXISTS `group_chat_member`;
CREATE TABLE `group_chat_member`  (
  `thread_id` bigint UNSIGNED NOT NULL COMMENT '引用 group_chat_thread.thread_id',
  `user_id` bigint UNSIGNED NOT NULL COMMENT '引用 user.user_id',
  `role` tinyint NOT NULL DEFAULT 0 COMMENT '0=普通成员,1=管理员,2=创建者',
  `joined_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `muted_until` timestamp NULL DEFAULT NULL COMMENT '如果被禁言，可存到什么时候',
  PRIMARY KEY (`thread_id`, `user_id`) USING BTREE,
  INDEX `idx_user_threads`(`user_id` ASC) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for private_chat
-- ----------------------------
DROP TABLE IF EXISTS `private_chat`;
CREATE TABLE `private_chat`  (
  `thread_id` bigint UNSIGNED NOT NULL COMMENT '引用chat_thread.id',
  `user1_id` bigint UNSIGNED NOT NULL,
  `user2_id` bigint UNSIGNED NOT NULL,
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`thread_id`) USING BTREE,
  UNIQUE INDEX `uniq_private_thread`(`user1_id` ASC, `user2_id` ASC) USING BTREE,
  INDEX `idx_private_user1_thread`(`user1_id` ASC, `thread_id` ASC) USING BTREE,
  INDEX `idx_private_user2_thread`(`user2_id` ASC, `thread_id` ASC) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for user
-- ----------------------------
DROP TABLE IF EXISTS `user`;
CREATE TABLE `user`  (
  `id` int NOT NULL AUTO_INCREMENT,
  `uid` int NOT NULL DEFAULT 0,
  `name` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL DEFAULT NULL,
  `email` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL DEFAULT NULL,
  `pwd` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `nick` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `desc` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `sex` int NOT NULL DEFAULT 0,
  `icon` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT ':/res/head_0.jpg',
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `uid`(`uid` ASC) USING BTREE,
  UNIQUE INDEX `email`(`email` ASC) USING BTREE,
  INDEX `name`(`name` ASC) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 745253 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for user_id
-- ----------------------------
DROP TABLE IF EXISTS `user_id`;
CREATE TABLE `user_id`  (
  `id` int NOT NULL,
  PRIMARY KEY (`id`) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Table structure for todo_item
-- ----------------------------
DROP TABLE IF EXISTS `todo_item`;
CREATE TABLE `todo_item`  (
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
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Procedure structure for reg_user
-- ----------------------------
DROP PROCEDURE IF EXISTS `reg_user`;
delimiter ;;
CREATE PROCEDURE `reg_user`(IN `new_name` VARCHAR(255), 
    IN `new_email` VARCHAR(255), 
    IN `new_pwd` VARCHAR(255), 
    OUT `result` INT)
BEGIN
    -- 如果在执行过程中遇到任何错误，则回滚事务
    DECLARE EXIT HANDLER FOR SQLEXCEPTION
    BEGIN
        -- 回滚事务
        ROLLBACK;
        -- 设置返回值为-1，表示错误
        SET result = -1;
    END;
    -- 开始事务
    START TRANSACTION;
    -- 检查用户名是否已存在
    IF EXISTS (SELECT 1 FROM `user` WHERE `name` = new_name) THEN
        SET result = 0; -- 用户名已存在
        COMMIT;
    ELSE
        -- 用户名不存在，检查email是否已存在
        IF EXISTS (SELECT 1 FROM `user` WHERE `email` = new_email) THEN
            SET result = 0; -- email已存在
            COMMIT;
        ELSE
            -- email也不存在，更新user_id表
            UPDATE `user_id` SET `id` = `id` + 1;
            -- 获取更新后的id
            SELECT `id` INTO @new_id FROM `user_id`;
            -- 在user表中插入新记录
            INSERT INTO `user` (`uid`, `name`, `email`, `pwd`) VALUES (@new_id, new_name, new_email, new_pwd);
            -- 设置result为新插入的uid
            SET result = @new_id; -- 插入成功，返回新的uid
            COMMIT;
        END IF;
    END IF;
END
;;
delimiter ;

-- ----------------------------
-- Procedure structure for reg_user_procedure
-- ----------------------------
DROP PROCEDURE IF EXISTS `reg_user_procedure`;
delimiter ;;
CREATE PROCEDURE `reg_user_procedure`(IN `new_name` VARCHAR(255), 
                IN `new_email` VARCHAR(255), 
                IN `new_pwd` VARCHAR(255), 
                OUT `result` INT)
BEGIN
                -- 如果在执行过程中遇到任何错误，则回滚事务
                DECLARE EXIT HANDLER FOR SQLEXCEPTION
                BEGIN
                    -- 回滚事务
                    ROLLBACK;
                    -- 设置返回值为-1，表示错误
                    SET result = -1;
                END;
                -- 开始事务
                START TRANSACTION;
                -- 检查用户名是否已存在
                IF EXISTS (SELECT 1 FROM `user` WHERE `name` = new_name) THEN
                    SET result = 0; -- 用户名已存在
                    COMMIT;
                ELSE
                    -- 用户名不存在，检查email是否已存在
                    IF EXISTS (SELECT 1 FROM `user` WHERE `email` = new_email) THEN
                        SET result = 0; -- email已存在
                        COMMIT;
                    ELSE
                        -- email也不存在，更新user_id表
                        UPDATE `user_id` SET `id` = `id` + 1;
                        -- 获取更新后的id
                        SELECT `id` INTO @new_id FROM `user_id`;
                        -- 在user表中插入新记录
                        INSERT INTO `user` (`uid`, `name`, `email`, `pwd`) VALUES (@new_id, new_name, new_email, new_pwd);
                        -- 设置result为新插入的uid
                        SET result = @new_id; -- 插入成功，返回新的uid
                        COMMIT;
                    END IF;
                END IF;
            END
;;
delimiter ;

SET FOREIGN_KEY_CHECKS = 1;
