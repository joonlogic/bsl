
* 디렉토리 설명

- app/
	bsl_diag64 : 64비트 단위 레지스터 억세스
	bsl_diag64_mdio : 10G MDIO영역 억세스 & PHY chip 초기화

- api/
	libbsl.a : 정적 라이브러리 파일

- module/
	bsl_load   : 모듈 적재 스크립트


* 컴파일 / 설치 방법 (기본 설치 디렉토리 : /bsl)
# make clean
# make
# make install


* 모듈 적재 방법
# cd module/
# make
# ./bsl_load

