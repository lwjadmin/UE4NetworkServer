USE ANIMALGUYS;

DELETE FROM PLAYER;

SELECT * FROM PLAYER;

INSERT INTO PLAYER VALUES('LWJ','1234','LWJADMIN');



SELECT * FROM PLAYERSTATE;

INSERT INTO PLAYERSTATE
(
    PLAYER_ID,
    PLAYER_GOLD,
    PLAYER_EXP,
    PLAYER_LEVEL,
    PLAYERCHAR_TYPE,
    PLAYERCHAR_BODY_SLOT,
    PLAYERCHAR_HEAD_SLOT,
    PLAYERCHAR_JUMP_STAT,
    PLAYERCHAR_STAMINA_STAT,
    PLAYERCHAR_SPEED_STAT,
    REGISTER_DTTM
)VALUES(
     ?(플레이어ID),
     0,
     0,
     0,
     0,
     0,
     0,
     0, 
     0,
     0, 
     NOW()
);
