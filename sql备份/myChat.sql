/*
 Navicat Premium Data Transfer

 Source Server         : WSL2_MySQL
 Source Server Type    : MySQL
 Source Server Version : 80044 (8.0.44-0ubuntu0.24.04.2)
 Source Host           : 127.0.0.1:3306
 Source Schema         : myChat

 Target Server Type    : MySQL
 Target Server Version : 80044 (8.0.44-0ubuntu0.24.04.2)
 File Encoding         : 65001

 Date: 28/01/2026 11:11:45
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

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
) ENGINE = InnoDB AUTO_INCREMENT = 93 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of friend
-- ----------------------------
INSERT INTO `friend` VALUES (55, 1055, 1054, 'sqy');
INSERT INTO `friend` VALUES (56, 1054, 1055, '');
INSERT INTO `friend` VALUES (61, 1012, 1056, 'test28');
INSERT INTO `friend` VALUES (62, 1056, 1012, '');
INSERT INTO `friend` VALUES (63, 1012, 1050, 'test23');
INSERT INTO `friend` VALUES (64, 1050, 1012, '');
INSERT INTO `friend` VALUES (81, 1002, 1019, 'zack');
INSERT INTO `friend` VALUES (82, 1019, 1002, '');
INSERT INTO `friend` VALUES (89, 1025, 1059, 'sjj');
INSERT INTO `friend` VALUES (90, 1059, 1025, '');
INSERT INTO `friend` VALUES (91, 1059, 1060, 'vip88');
INSERT INTO `friend` VALUES (92, 1060, 1059, '');

-- ----------------------------
-- Table structure for friend_apply
-- ----------------------------
DROP TABLE IF EXISTS `friend_apply`;
CREATE TABLE `friend_apply`  (
  `id` bigint NOT NULL AUTO_INCREMENT,
  `from_uid` int NOT NULL,
  `to_uid` int NOT NULL,
  `status` smallint NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `from_to_uid`(`from_uid` ASC, `to_uid` ASC) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 70 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of friend_apply
-- ----------------------------
INSERT INTO `friend_apply` VALUES (6, 1023, 1002, 0);
INSERT INTO `friend_apply` VALUES (49, 1054, 1055, 1);
INSERT INTO `friend_apply` VALUES (52, 1056, 1012, 0);
INSERT INTO `friend_apply` VALUES (63, 1019, 1002, 1);
INSERT INTO `friend_apply` VALUES (64, 1032, 1035, 0);
INSERT INTO `friend_apply` VALUES (68, 1059, 1025, 1);
INSERT INTO `friend_apply` VALUES (69, 1060, 1059, 1);

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
  `icon` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `uid`(`uid` ASC) USING BTREE,
  UNIQUE INDEX `email`(`email` ASC) USING BTREE,
  INDEX `name`(`name` ASC) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 64 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of user
-- ----------------------------
INSERT INTO `user` VALUES (3, 1002, 'llfc', 'secondtonone1@163.com', '654321)', 'llfc', '', 0, ':/res/head_1.jpg');
INSERT INTO `user` VALUES (4, 1003, 'tc', '18165031775@qq.com', '123456', 'tc', '', 0, ':/res/head_1.jpg');
INSERT INTO `user` VALUES (5, 1004, 'yuanweihua', '1456188862@qq.com', '}kyn;89>?<', 'yuanweihua', '', 0, ':/res/head_1.jpg');
INSERT INTO `user` VALUES (6, 1005, 'test', '2022202210033@whu.edu.cn', '}kyn;89>?<', 'test', '', 0, ':/res/head_1.jpg');
INSERT INTO `user` VALUES (8, 1007, 'fhr', '3157199927@qq.com', 'xuexi1228', 'fhr', '', 0, ':/res/head_1.jpg');
INSERT INTO `user` VALUES (9, 1008, 'zglx2008', 'zglx2008@163.com', '123456', 'zglx2008', '', 0, ':/res/head_1.jpg');
INSERT INTO `user` VALUES (13, 1012, 'resettest', '1042958553@qq.com', '230745', 'resettest', '', 0, ':/res/head_1.jpg');
INSERT INTO `user` VALUES (14, 1013, 'rss_test', '1685229455@qq.com', '123456', 'rss_test', '', 0, ':/res/head_1.jpg');
INSERT INTO `user` VALUES (15, 1014, '123456789', '1152907774@qq.com', '123456789', '123456789', '', 0, ':/res/head_1.jpg');
INSERT INTO `user` VALUES (16, 1015, 'aaaaaaa', '3031719794@qq.com', '777777', 'aaaaaaa', '', 0, ':/res/head_1.jpg');
INSERT INTO `user` VALUES (17, 1016, 'aaa', '2996722319@qq.com', '222222', 'aaa', '', 0, ':/res/head_1.jpg');
INSERT INTO `user` VALUES (20, 1019, 'zack', '1017234088@qq.com', '654321)', 'zack', '', 0, ':/res/head_1.jpg');
INSERT INTO `user` VALUES (21, 1020, 'aatext', '1584736136@qq.com', '745230', 'aatext', '', 0, ':/res/head_1.jpg');
INSERT INTO `user` VALUES (22, 1021, 'ferrero1', '1220292901@qq.com', '1234', 'ferrero1', '', 0, ':/res/head_1.jpg');
INSERT INTO `user` VALUES (23, 1022, 'ferrero2', '15504616642@163.com', '1234', 'ferrero2', '', 0, ':/res/head_1.jpg');
INSERT INTO `user` VALUES (24, 1023, 'lyf', '3194811890@qq.com', '123456', 'lyf', '', 0, ':/res/head_1.jpg');
INSERT INTO `user` VALUES (25, 1024, 'lh', '2494350589@qq.com', 'fb8::>:;8<', 'lh', '', 0, ':/res/head_1.jpg');
INSERT INTO `user` VALUES (26, 1025, 'jf', 'luojianfeng553@163.com', '745230', 'jf', '', 0, ':/res/head_1.jpg');
INSERT INTO `user` VALUES (33, 1031, 'zjm', '1013049201@qq.com', '745230', 'zjm', '', 0, ':/res/head_1.jpg');
INSERT INTO `user` VALUES (34, 1032, 'yxc', '1003314442@qq.com', '123', 'yxc', '', 0, ':/res/head_1.jpg');
INSERT INTO `user` VALUES (37, 1035, 'wangyu', '962087148@qq.com', '123456', 'wangyu', '', 0, ':/res/head_1.jpg');
INSERT INTO `user` VALUES (39, 1037, 'chae', '318192621@qq.com', '123456', 'chae', '', 0, ':/res/head_1.jpg');
INSERT INTO `user` VALUES (40, 1038, 'summer', '1586856388@qq.com', '654321)', 'summer', '', 0, ':/res/head_2.jpg');
INSERT INTO `user` VALUES (44, 1042, 'zzz', '3434321837@qq.com', '|l~745', '', '', 0, '');
INSERT INTO `user` VALUES (45, 1043, 'sadda', 'z1668047679@163.com', '123456', '', '', 0, '');
INSERT INTO `user` VALUES (46, 1044, 'qwe', '1368326038@qq.com', '1234', '', '', 0, '');
INSERT INTO `user` VALUES (52, 1050, 'test23', '161945471@qq.com', '230745', 'test23', '', 0, ':/res/head_3.jpg');
INSERT INTO `user` VALUES (53, 1051, '123', '1767269204@qq.com', '123', '', '', 0, '');
INSERT INTO `user` VALUES (54, 1052, 'zjc', '766741776@qq.com', '745230', '', '', 0, '');
INSERT INTO `user` VALUES (55, 1053, 'test_1', 'zzsr_0719@qq.com', '123456', '', '', 0, '');
INSERT INTO `user` VALUES (56, 1054, 'sqy', '3175614975@qq.com', '745230', 'sqy', '', 0, ':/res/head_2.jpg');
INSERT INTO `user` VALUES (57, 1055, 'ocz', 'q3175614975@163.com', '745230', 'ocz', '', 0, ':/res/head_3.jpg');
INSERT INTO `user` VALUES (58, 1056, 'test28', '1669475972@qq.com', '230745', 'test28', '', 0, ':/res/head_1.jpg');
INSERT INTO `user` VALUES (60, 1058, 'NoOne', '1764850358@qq.com', '745230', '', '', 0, '');
INSERT INTO `user` VALUES (61, 1059, 'sjj', '12@qq.com', '745230', '', '', 0, '');
INSERT INTO `user` VALUES (62, 1060, '88vip', 'taobao@ali.com', '745230', '', '', 0, '');
INSERT INTO `user` VALUES (63, 1061, 'ad', 'ad@qeds.com', '745230', '', '', 0, '');

-- ----------------------------
-- Table structure for user_id
-- ----------------------------
DROP TABLE IF EXISTS `user_id`;
CREATE TABLE `user_id`  (
  `id` int NOT NULL,
  PRIMARY KEY (`id`) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of user_id
-- ----------------------------
INSERT INTO `user_id` VALUES (1061);

-- ----------------------------
-- Procedure structure for reg_user
-- ----------------------------
DROP PROCEDURE IF EXISTS `reg_user`;
delimiter ;;
CREATE PROCEDURE `reg_user`(IN `new_name` VARCHAR(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci, 
    IN `new_email` VARCHAR(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci, 
    IN `new_pwd` VARCHAR(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci, 
    OUT `result` INT)
BEGIN
    DECLARE EXIT HANDLER FOR SQLEXCEPTION
    BEGIN
        ROLLBACK;
        SET result = -1;
    END;

    START TRANSACTION;

    -- 检查用户名是否已存在
    IF EXISTS (SELECT 1 FROM `user` WHERE `name` = new_name) THEN
        SET result = 0; 
        COMMIT;
    ELSE
        -- 检查email是否已存在
        IF EXISTS (SELECT 1 FROM `user` WHERE `email` = new_email) THEN
            SET result = 0; 
            COMMIT;
        ELSE
            -- 更新自增 ID
            UPDATE `user_id` SET `id` = `id` + 1;
            SELECT `id` INTO @new_id FROM `user_id`;
            
            -- 插入新记录
            INSERT INTO `user` (`uid`, `name`, `email`, `pwd`) 
            VALUES (@new_id, new_name, new_email, new_pwd);
            
            SET result = @new_id; 
            COMMIT;
        END IF;
    END IF;
END
;;
delimiter ;

-- ----------------------------
-- Procedure structure for reg_user_debug
-- ----------------------------
DROP PROCEDURE IF EXISTS `reg_user_debug`;
delimiter ;;
CREATE PROCEDURE `reg_user_debug`(IN `new_name` VARCHAR(255), 
    IN `new_email` VARCHAR(255), 
    IN `new_pwd` VARCHAR(255), 
    OUT `result` INT)
BEGIN
    -- 注意：这里去掉了 DECLARE EXIT HANDLER
    START TRANSACTION;
    IF EXISTS (SELECT 1 FROM `user` WHERE `name` = new_name) THEN
        SET result = 0;
        COMMIT;
    ELSEIF EXISTS (SELECT 1 FROM `user` WHERE `email` = new_email) THEN
        SET result = 0;
        COMMIT;
    ELSE
        UPDATE `user_id` SET `id` = `id` + 1;
        SELECT `id` INTO @new_id FROM `user_id`;
        -- 如果这里报错，你会直接在控制台看到具体的 Error Code 和消息
        INSERT INTO `user` (`uid`, `name`, `email`, `pwd`) VALUES (@new_id, new_name, new_email, new_pwd);
        SET result = @new_id;
        COMMIT;
    END IF;
END
;;
delimiter ;

SET FOREIGN_KEY_CHECKS = 1;
