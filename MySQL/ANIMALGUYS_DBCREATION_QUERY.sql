DROP DATABASE IF EXISTS ANIMALGUYS;
-- DB생성 : ANIMALGUYS
CREATE DATABASE ANIMALGUYS;
-- DB사용 : ANIMALGUYS
USE ANIMALGUYS;

-- 테이블 생성 : 플레이어
DROP TABLE IF EXISTS PLAYER;
CREATE TABLE PLAYER
(
    PLAYER_ID   VARCHAR(30)  NOT NULL, -- 플레이어ID(PK)
    PLAYER_PWD  VARCHAR(200) NOT NULL, -- 플레이어비밀번호
    PLAYER_NAME VARCHAR(30)  NOT NULL  -- 플레이어명
);
ALTER TABLE PLAYER
ADD CONSTRAINT PK1_PLAYER PRIMARY KEY(PLAYER_ID);
/*------------------------------------------------------------
    플레이어ID는 8자 이내 영문/숫자 ONLY
    플레이어비밀번호는 8자 이내 영문/숫자 ONLY, SHA 적용
    플레이어명은 8자 이내 영문/숫자 ONLY
------------------------------------------------------------*/

-- 테이블 생성 : 플레이어 상태
DROP TABLE IF EXISTS PLAYERSTATE;
CREATE TABLE PLAYERSTATE
(
    PLAYER_ID               VARCHAR(30) NOT NULL, -- 플레이어ID(PK/FK)
    PLAYER_GOLD             INT         NOT NULL, -- 플레이어 보유골드
    PLAYER_EXP              INT         NOT NULL, -- 플레이어 경험치
    PLAYER_LEVEL            INT         NOT NULL, -- 플레이어 레벨
    PLAYERCHAR_TYPE         INT         NOT NULL, -- 플레이어 캐릭터-타입
    PLAYERCHAR_BODY_SLOT    INT         NOT NULL, -- 플레이어 캐릭터-몸슬롯
    PLAYERCHAR_HEAD_SLOT    INT         NOT NULL, -- 플레이어 캐릭터-머리슬롯
    PLAYERCHAR_JUMP_STAT    INT         NOT NULL, -- 플레이어 스탯-점프력
    PLAYERCHAR_STAMINA_STAT INT         NOT NULL, -- 플레이어 스탯-스태미나
    PLAYERCHAR_SPEED_STAT   INT         NOT NULL, -- 플레이어 스탯-이동속도
    REGISTER_DTTM           DATETIME    NOT NULL  -- 업데이트 일시
);
ALTER TABLE PLAYERSTATE
ADD CONSTRAINT PK1_PLAYERSTATE PRIMARY KEY(PLAYER_ID);

ALTER TABLE PLAYERSTATE
ADD CONSTRAINT FK1_PLAYERSTATE
FOREIGN KEY(PLAYER_ID) REFERENCES PLAYER(PLAYER_ID);

ALTER TABLE PLAYERSTATE
ALTER REGISTER_DTTM SET DEFAULT(CURRENT_TIMESTAMP);
/*------------------------------------------------------------
    플레이어 캐릭터-타입 
	: 0(고양이), 1(돼지), 2(개), 3(토끼)
	플레이어 캐릭터-몸슬롯
	: Mesh에 따라 순번을 부여하여 적용!
	플레이어 캐릭터-머리슬롯
	: Mesh에 따라 순번을 부여하여 적용!
------------------------------------------------------------*/

-- 테이블 생성 : 현재 접속중인 플레이어 
DROP TABLE IF EXISTS CURLOGINPLAYER;
CREATE TABLE CURLOGINPLAYER
(
    LOGIN_PLAYER_ID VARCHAR(30) NOT NULL, -- 현재 접속중인 플레이어ID
    LOGIN_DTTM      DATETIME    NULL,     -- 로그인일시
    LOGOUT_DTTM     DATETIME    NULL,     -- 로그아웃일시
    LOGIN_STAT      INT         NOT NULL  -- 로그인상태
);
/*------------------------------------------------------------
    로그인상태
	: 1(로그인), 2(로그아웃:기본값)
------------------------------------------------------------*/

-- 테이블 생성 : 게임 세션
DROP TABLE IF EXISTS GAMESESSION;
CREATE TABLE GAMESESSION
(
    SESSION_ID         INT         NOT NULL, -- 세션ID(PK)
	HOST_PLAYER_ID     VARCHAR(30) NOT NULL, -- 호스트(방장) 플레이어ID
    SESSION_NAME       VARCHAR(30) NOT NULL, -- 세션명
    SESSION_PWD        VARCHAR(30) NULL,     -- 세션비밀번호
    SESSION_PLAYER     INT         NOT NULL, -- 세션접속 플레이어
    SESSION_STATE      INT         NOT NULL, -- 세션상태
    SESSION_OPEN_DTTM  DATETIME    NULL,     -- 세션시작일시
    SESSION_CLOSE_DTTM DATETIME    NULL      -- 세션종료일시
);
ALTER TABLE GAMESESSION
ADD CONSTRAINT PK1_GAMESESSION PRIMARY KEY(SESSION_ID);
ALTER TABLE GAMESESSION MODIFY SESSION_ID INT NOT NULL AUTO_INCREMENT;
/*------------------------------------------------------------
    세션ID : DB생성후 순번반환
	세션접속 플레이어 : 2/4/8/16
	세션상태 
	1(대기:기본값),2(시작),3(종료)
	세션이 만들어지면 시작일시 입력, 세션이 종료(최후의1인)되면 종료일시 입력
------------------------------------------------------------*/

DROP TABLE IF EXISTS CHATTINGLOG;
-- 채팅로그
CREATE TABLE CHATTINGLOG
(
    PLAYER_ID        VARCHAR(30)  NOT NULL, -- 플레이어ID
    CHATCHANNEL_TYPE INT          NOT NULL, -- 채널타입
    SESSION_ID       INT          NULL,     -- 세션ID
    CHAT_MSG         VARCHAR(100) NULL,     -- 채팅메시지
    REGISTER_DTTM    DATETIME     NOT NULL  -- 등록일시
);
ALTER TABLE CHATTINGLOG
ALTER REGISTER_DTTM SET DEFAULT(CURRENT_TIMESTAMP);
/*------------------------------------------------------------
    플레이어ID : 작성자
	채널타입   : 1(세션목록), 2(로비)
	세션ID    : 채널타입이 2(로비)일 경우 세션ID
	채팅메시지  : 30자이내 
------------------------------------------------------------*/

DROP TABLE IF EXISTS WHISPERLOG;
-- 귓속말로그
CREATE TABLE WHISPERLOG
(
    SENDER_PLAYER_ID   VARCHAR(30)  NOT NULL, -- 보낸 플레이어
    RECEIVER_PLAYER_ID VARCHAR(30)  NOT NULL, -- 받는 플레이어
    CHAT_MSG           VARCHAR(100) NULL,     -- 채팅메시지
    REGISTER_DTTM      DATETIME     NOT NULL  -- 등록일시
);
ALTER TABLE WHISPERLOG
ALTER REGISTER_DTTM SET DEFAULT(CURRENT_TIMESTAMP);
/*------------------------------------------------------------
    채팅메시지  : 30자이내
	보낸플레이어 : 보낸이
	받는플레이어 : 받는이
------------------------------------------------------------*/