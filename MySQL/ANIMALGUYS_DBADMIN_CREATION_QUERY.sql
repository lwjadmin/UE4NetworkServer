/*----------------------------------------------------------
	2022.12.13 이웅재 
    해당 쿼리는 root권한으로 로그인하여 생성하는 것을 권장합니다!
----------------------------------------------------------*/

-- 계정 권한 테이블 : user이 포함된 DB접속[root권한만 접속가능!]
use mysql;
-- 사용자 : AnimGuysAdmin 삭제 
DELETE FROM mysql.user where user = 'AnimalGuysAdmin';
-- 사용자 : AnimGuysAdmin 추가, 172.16.2.* 대역에서 사용할수 있도록 설정, 비밀번호 : Passw0rd
create user 'AnimalGuysAdmin'@'172.16.2.%' identified by 'Passw0rd'; 
-- DBMS 사용자 조회 
select host, user from mysql.user;
-- 5. 사용자 : @UserID에게 사용할 DB테이블 : animalguys에 대한 모든 권한 설정
GRANT ALL PRIVILEGES ON animalguys.* to 'AnimalGuysAdmin'@'172.16.2.%';
-- 6. 권한 변경사항 반영
FLUSH PRIVILEGES;
-- DBMS 사용자 조회 (정상 업데이트 확인)
select host, user from mysql.user;
