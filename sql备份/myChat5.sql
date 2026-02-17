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

 Date: 16/02/2026 20:19:50
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
  `msg_type` tinyint NOT NULL DEFAULT 0 COMMENT '0=文本 1=图片 2=视频 3=文件',
  PRIMARY KEY (`message_id`) USING BTREE,
  INDEX `idx_thread_created`(`thread_id` ASC, `created_at` ASC) USING BTREE,
  INDEX `idx_thread_message`(`thread_id` ASC, `message_id` ASC) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 398 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of chat_message
-- ----------------------------
INSERT INTO `chat_message` VALUES (33, 35, 1002, 1019, '您好,我是llfc', '2025-07-02 06:41:08', '2025-07-24 08:27:45', 2, 0);
INSERT INTO `chat_message` VALUES (34, 35, 1019, 1002, 'We are friends now!', '2025-07-02 06:41:08', '2025-07-24 08:27:39', 2, 0);
INSERT INTO `chat_message` VALUES (35, 35, 1002, 1019, '你好，很高兴认识你', '2025-07-24 21:36:30', '2025-07-24 21:36:30', 2, 0);
INSERT INTO `chat_message` VALUES (36, 35, 1019, 1002, '我也是，很高兴认识你', '2025-07-24 21:36:43', '2025-07-24 21:36:43', 2, 0);
INSERT INTO `chat_message` VALUES (37, 35, 1019, 1002, '中午吃点什么？', '2025-07-25 22:48:04', '2025-07-25 22:48:04', 2, 0);
INSERT INTO `chat_message` VALUES (38, 35, 1002, 1019, '现在外卖三国杀，折扣力度大，赶紧薅羊毛', '2025-07-25 22:48:36', '2025-07-25 22:48:36', 2, 0);
INSERT INTO `chat_message` VALUES (39, 35, 1019, 1002, '刚看了下，我得了一个25减25的券，我去订外卖去了', '2025-07-25 23:09:26', '2025-07-25 23:09:26', 2, 0);
INSERT INTO `chat_message` VALUES (40, 35, 1002, 1019, '我怎么没看到，你在哪个平台？', '2025-07-25 23:09:40', '2025-07-25 23:09:40', 2, 0);
INSERT INTO `chat_message` VALUES (41, 35, 1019, 1002, '我看错了，那不是外卖券，那是超市打折券', '2025-07-25 23:10:20', '2025-07-25 23:10:20', 2, 0);
INSERT INTO `chat_message` VALUES (42, 35, 1019, 1002, '好吧，那我还是去看看外卖活动', '2025-07-27 09:07:21', '2025-07-27 09:07:21', 2, 0);
INSERT INTO `chat_message` VALUES (43, 35, 1002, 1019, '嗯，有什么推荐的外卖可以告诉我', '2025-07-27 09:07:37', '2025-07-27 09:07:37', 2, 0);
INSERT INTO `chat_message` VALUES (44, 35, 1019, 1002, '汉堡炸鸡怎么样？要不要一起订', '2025-07-29 17:40:20', '2025-07-29 17:40:20', 2, 0);
INSERT INTO `chat_message` VALUES (45, 36, 1176, 1175, '您好,我是yulinyi', '2025-08-22 03:07:31', '2025-08-22 03:07:31', 2, 0);
INSERT INTO `chat_message` VALUES (355, 50, 1297, 1298, '您好,我是sjj', '2026-02-11 14:42:42', '2026-02-11 14:42:42', 2, 0);
INSERT INTO `chat_message` VALUES (356, 50, 1298, 1297, 'We are friends now!', '2026-02-11 14:42:42', '2026-02-11 14:42:42', 2, 0);
INSERT INTO `chat_message` VALUES (367, 50, 1298, 1297, 'f8fd7b80-c2b3-4fc5-923e-313d761e4058.jpg', '2026-02-11 17:17:14', '2026-02-11 17:17:15', 2, 1);
INSERT INTO `chat_message` VALUES (368, 50, 1297, 1298, '93baec01-871e-4732-a7f2-6f1f3e8cf35b.jpg', '2026-02-11 17:17:25', '2026-02-11 17:17:25', 2, 1);
INSERT INTO `chat_message` VALUES (369, 50, 1298, 1297, '7d613c10-3eae-49e2-bcc5-2ad8665b57f7.jpg', '2026-02-11 17:21:52', '2026-02-11 17:21:52', 2, 1);
INSERT INTO `chat_message` VALUES (370, 50, 1297, 1298, '4c34d464-6a29-4a50-905a-455d5cad1744.jpg', '2026-02-11 17:22:41', '2026-02-11 17:22:42', 2, 1);
INSERT INTO `chat_message` VALUES (371, 50, 1297, 1298, 'bee0664a-f054-428a-9ce1-d6aafa78d59e.jpg', '2026-02-11 17:24:03', '2026-02-11 17:24:04', 2, 1);
INSERT INTO `chat_message` VALUES (372, 50, 1297, 1298, '2565cea8-e077-4ea2-b838-861428a88377.jpg', '2026-02-11 17:24:48', '2026-02-11 17:24:48', 2, 1);
INSERT INTO `chat_message` VALUES (373, 50, 1297, 1298, 'b25d3758-e859-45b4-a203-fc30e55c3e42.jpg', '2026-02-11 17:26:44', '2026-02-11 17:26:45', 2, 1);
INSERT INTO `chat_message` VALUES (374, 50, 1298, 1297, '你好', '2026-02-11 17:43:11', '2026-02-11 17:43:11', 2, 0);
INSERT INTO `chat_message` VALUES (375, 50, 1298, 1297, '5fd942ff-1830-4fa7-93cd-07677abbc3ea.jpg', '2026-02-11 17:43:26', '2026-02-11 17:43:27', 2, 1);
INSERT INTO `chat_message` VALUES (376, 50, 1297, 1298, '3c75e334-6474-4c4c-852a-993eaa169f8c.jpg', '2026-02-11 17:43:34', '2026-02-11 17:43:34', 2, 1);
INSERT INTO `chat_message` VALUES (377, 50, 1298, 1297, '26300ac1-07a8-4f3d-95b3-0f59d9334f9c.jpg', '2026-02-11 17:49:13', '2026-02-11 17:49:13', 2, 1);
INSERT INTO `chat_message` VALUES (378, 50, 1297, 1298, '1b15f2b2-6055-44e4-b6e6-95d628faeaa3.jpg', '2026-02-11 17:49:28', '2026-02-11 17:49:29', 2, 1);
INSERT INTO `chat_message` VALUES (379, 50, 1298, 1297, '0baa0224-c352-4ed3-8253-158094aa9dec.jpg', '2026-02-11 18:14:45', '2026-02-11 18:14:46', 2, 1);
INSERT INTO `chat_message` VALUES (380, 50, 1297, 1298, '086dce5f-cedd-4b32-9623-8fefab93046a.jpg', '2026-02-11 18:14:59', '2026-02-11 18:15:00', 2, 1);
INSERT INTO `chat_message` VALUES (381, 50, 1298, 1297, '3e71bf20-f4f9-4ad3-b24c-c301b43f1c67.jpg', '2026-02-11 21:55:42', '2026-02-11 21:55:42', 2, 1);
INSERT INTO `chat_message` VALUES (382, 50, 1297, 1298, '72df4db7-70fe-4fd7-857b-67f309e819ac.jpg', '2026-02-11 21:55:46', '2026-02-11 21:55:46', 2, 1);

-- ----------------------------
-- Table structure for chat_thread
-- ----------------------------
DROP TABLE IF EXISTS `chat_thread`;
CREATE TABLE `chat_thread`  (
  `id` bigint UNSIGNED NOT NULL AUTO_INCREMENT,
  `type` enum('private','group') CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL,
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 52 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of chat_thread
-- ----------------------------
INSERT INTO `chat_thread` VALUES (35, 'private', '2025-07-02 06:41:08');
INSERT INTO `chat_thread` VALUES (36, 'private', '2025-08-22 03:07:31');
INSERT INTO `chat_thread` VALUES (37, 'private', '2025-10-03 08:43:18');
INSERT INTO `chat_thread` VALUES (38, 'private', '2025-12-09 10:00:37');
INSERT INTO `chat_thread` VALUES (39, 'private', '2025-12-09 10:00:44');
INSERT INTO `chat_thread` VALUES (40, 'private', '2026-01-06 13:55:39');
INSERT INTO `chat_thread` VALUES (41, 'private', '2026-01-30 02:20:52');
INSERT INTO `chat_thread` VALUES (42, 'private', '2026-01-31 09:34:34');
INSERT INTO `chat_thread` VALUES (43, 'private', '2026-01-31 09:39:00');
INSERT INTO `chat_thread` VALUES (44, 'private', '2026-01-31 14:19:27');
INSERT INTO `chat_thread` VALUES (45, 'group', '2026-01-31 14:41:29');
INSERT INTO `chat_thread` VALUES (46, 'group', '2026-01-31 14:53:29');
INSERT INTO `chat_thread` VALUES (47, 'group', '2026-01-31 14:59:27');
INSERT INTO `chat_thread` VALUES (48, 'group', '2026-01-31 15:11:13');
INSERT INTO `chat_thread` VALUES (49, 'private', '2026-02-05 04:28:26');
INSERT INTO `chat_thread` VALUES (50, 'private', '2026-02-11 14:42:42');
INSERT INTO `chat_thread` VALUES (51, 'private', '2026-02-12 20:23:21');

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
) ENGINE = InnoDB AUTO_INCREMENT = 623 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of friend
-- ----------------------------
INSERT INTO `friend` VALUES (55, 1055, 1054, 'sqy');
INSERT INTO `friend` VALUES (56, 1054, 1055, '');
INSERT INTO `friend` VALUES (619, 1297, 1298, 'rice');
INSERT INTO `friend` VALUES (620, 1298, 1297, 'sjj');
INSERT INTO `friend` VALUES (621, 1296, 1297, 'sjj');
INSERT INTO `friend` VALUES (622, 1297, 1296, '张四');

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
) ENGINE = InnoDB AUTO_INCREMENT = 456 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of friend_apply
-- ----------------------------
INSERT INTO `friend_apply` VALUES (455, 1296, 1297, 1, '您好,我是张四', 'sjj');

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
-- Records of group_chat
-- ----------------------------
INSERT INTO `group_chat` VALUES (45, NULL, '2026-01-31 14:41:30');
INSERT INTO `group_chat` VALUES (46, NULL, '2026-01-31 14:53:29');
INSERT INTO `group_chat` VALUES (47, NULL, '2026-01-31 14:59:28');
INSERT INTO `group_chat` VALUES (48, NULL, '2026-01-31 15:11:14');

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
-- Records of group_chat_member
-- ----------------------------
INSERT INTO `group_chat_member` VALUES (45, 1213, 0, '2026-01-31 14:41:30', NULL);
INSERT INTO `group_chat_member` VALUES (45, 1275, 2, '2026-01-31 14:41:30', NULL);
INSERT INTO `group_chat_member` VALUES (46, 1213, 0, '2026-01-31 14:53:29', NULL);
INSERT INTO `group_chat_member` VALUES (46, 1275, 2, '2026-01-31 14:53:29', NULL);
INSERT INTO `group_chat_member` VALUES (47, 1213, 0, '2026-01-31 14:59:28', NULL);
INSERT INTO `group_chat_member` VALUES (47, 1275, 2, '2026-01-31 14:59:28', NULL);
INSERT INTO `group_chat_member` VALUES (48, 1213, 0, '2026-01-31 15:11:14', NULL);
INSERT INTO `group_chat_member` VALUES (48, 1275, 2, '2026-01-31 15:11:14', NULL);

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
-- Records of private_chat
-- ----------------------------
INSERT INTO `private_chat` VALUES (35, 1019, 1002, '2025-07-02 06:41:08');
INSERT INTO `private_chat` VALUES (36, 1175, 1176, '2025-08-22 03:07:31');
INSERT INTO `private_chat` VALUES (37, 0, 1217, '2025-10-03 08:43:18');
INSERT INTO `private_chat` VALUES (38, 1093, 2020, '2025-12-09 10:00:37');
INSERT INTO `private_chat` VALUES (39, 2020, 2030, '2025-12-09 10:00:44');
INSERT INTO `private_chat` VALUES (40, 1267, 1268, '2026-01-06 13:55:39');
INSERT INTO `private_chat` VALUES (41, 2181, 2179, '2026-01-30 02:20:52');
INSERT INTO `private_chat` VALUES (42, 1019, 1005, '2026-01-31 09:34:34');
INSERT INTO `private_chat` VALUES (43, 0, 2181, '2026-01-31 09:39:01');
INSERT INTO `private_chat` VALUES (44, 1275, 1213, '2026-01-31 14:19:27');
INSERT INTO `private_chat` VALUES (49, 0, 1291, '2026-02-05 04:28:26');
INSERT INTO `private_chat` VALUES (50, 1297, 1298, '2026-02-11 14:42:42');
INSERT INTO `private_chat` VALUES (51, 1296, 1297, '2026-02-12 20:23:21');

-- ----------------------------
-- Table structure for test
-- ----------------------------
DROP TABLE IF EXISTS `test`;
CREATE TABLE `test`  (
  `id` int NOT NULL,
  `product_no` varchar(20) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci NULL DEFAULT NULL,
  `name` varchar(255) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci NULL DEFAULT NULL,
  `price` decimal(10, 2) NULL DEFAULT NULL,
  PRIMARY KEY (`id`) USING BTREE,
  INDEX `index_product_no_name`(`product_no` ASC, `name` ASC) USING BTREE,
  INDEX `index_id_name`(`id` ASC, `name` ASC) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8mb3 COLLATE = utf8mb3_general_ci ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of test
-- ----------------------------
INSERT INTO `test` VALUES (1, '0001', 'apple', 6.00);
INSERT INTO `test` VALUES (2, '0002', 'banana', 2.00);
INSERT INTO `test` VALUES (3, '0003', 'orange', 3.00);
INSERT INTO `test` VALUES (4, '0004', 'iphone13', 5000.00);
INSERT INTO `test` VALUES (5, '0005', 'ipad8', 3500.00);
INSERT INTO `test` VALUES (6, '0006', 'macbookpro', 10000.00);
INSERT INTO `test` VALUES (7, '0007', 'ps5', 4000.00);
INSERT INTO `test` VALUES (8, '0008', 'grape', 10.00);
INSERT INTO `test` VALUES (9, '0009', 'watermelon', 40.00);
INSERT INTO `test` VALUES (10, '0010', 'mango', 8.00);

-- ----------------------------
-- Table structure for user
-- ----------------------------
DROP TABLE IF EXISTS `user`;
CREATE TABLE `user`  (
  `id` int NOT NULL AUTO_INCREMENT,
  `uid` int NOT NULL DEFAULT 0,
  `name` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `email` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `pwd` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `nick` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `desc` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `sex` int NOT NULL DEFAULT 0,
  `icon` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT ':/res/head_0.jpg',
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `uid`(`uid` ASC) USING BTREE,
  UNIQUE INDEX `email`(`email` ASC) USING BTREE,
  INDEX `name`(`name` ASC) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 745442 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of user
-- ----------------------------
INSERT INTO `user` VALUES (3, 1002, 'llfc', 'secondtonone1@163.com', '745230', 'llfc', '', 0, ':/res/head_3.jpg');
INSERT INTO `user` VALUES (4, 1003, 'tc', '18165031775@qq.com', '123456', 'tc', '', 0, ':/res/head_1.jpg');
INSERT INTO `user` VALUES (5, 1004, 'yuanweihua', '1456188862@qq.com', '}kyn;89>?<', 'yuanweihua', '', 0, ':/res/head_1.jpg');
INSERT INTO `user` VALUES (6, 1005, 'test', '2022202210033@whu.edu.cn', '}kyn;89>?<', 'test', '', 0, ':/res/head_1.jpg');
INSERT INTO `user` VALUES (8, 1007, 'fhr', '3157199927@qq.com', 'xuexi1228', 'fhr', '', 0, ':/res/head_1.jpg');
INSERT INTO `user` VALUES (9, 1008, 'zglx2008', 'zglx2008@163.com', '123456', 'zglx2008', '', 0, ':/res/head_1.jpg');
INSERT INTO `user` VALUES (745438, 1295, '张三', 'zhangsan@test.com', '123456', '', '', 0, ':/res/head_0.jpg');
INSERT INTO `user` VALUES (745439, 1296, '张四', 'zhang@test.com', '745230', '', '', 0, ':/res/head_0.jpg');
INSERT INTO `user` VALUES (745440, 1297, 'sjj', 'sjj@qq.com', '745230', '', '', 0, 'a324d77d-ad74-46dc-a761-4a6a40645a3f.png');
INSERT INTO `user` VALUES (745441, 1298, 'rice', 'rice@qq.com', '745230', '', '', 0, ':/res/head_0.jpg');

-- ----------------------------
-- Table structure for user_id
-- ----------------------------
DROP TABLE IF EXISTS `user_id`;
CREATE TABLE `user_id`  (
  `id` int NOT NULL AUTO_INCREMENT,
  PRIMARY KEY (`id`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 1299 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of user_id
-- ----------------------------
INSERT INTO `user_id` VALUES (1298);

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
