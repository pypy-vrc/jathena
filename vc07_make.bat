@echo off
rem VC++ 6.0 / VC++ .net / VC++ .net 2003 / VC++ ToolKit 2003 �ł̃r���h�p�o�b�`�t�@�C��

rem ---------------------------
rem �p�X�̐ݒ�

rem VC++ Toolkit
Set PATH=C:\Program Files\Microsoft Visual C++ Toolkit 2003\bin;C:\Program Files\Microsoft SDK\Bin;C:\Program Files\Microsoft SDK\Bin\winnt;C:\Program Files\Microsoft SDK\Bin\Win64;%PATH%
Set INCLUDE=C:\Program Files\Microsoft Visual C++ Toolkit 2003\include;C:\Program Files\Microsoft SDK\include;%INCLUDE%
Set LIB=C:\Program Files\Microsoft Visual C++ Toolkit 2003\lib;C:\Program Files\Microsoft SDK\Lib;%LIB%

rem VC++ .net 2003 / �K�v�Ȃ�R�����g�A�E�g���͂���
rem call "C:\Program Files\Microsoft Visual Studio .NET 2003\Vc7\bin\vcvars32.bat"

rem VC++ .net (2002) / �K�v�Ȃ�R�����g�A�E�g���͂���
rem call "C:\Program Files\Microsoft Visual Studio .NET\Vc7\bin\vcvars32.bat"

rem VC++ 6.0 / �K�v�Ȃ�R�����g�A�E�g���͂���
rem call "C:\Program Files\Microsoft Visual Studio\VC98\Bin\vcvars32.bat"

rem ---------------------------
rem �r���h�I�v�V�����̑I��

rem txt/sql �I�� �F sql �ɂ���Ȃ�R�����g�A�E�g����
set __TXT_MODE__=/D "TXT_ONLY"

rem login_id2 �� IP �ł��ɂ傲�ɂ債�����l�̓R�����g�A�E�g���͂���
rem set __CMP_AFL2__=/D "CMP_AUTHFIFO_LOGIN2"
rem set __CMP_AFIP__=/D "CMP_AUTHFIFO_IP"

rem httpd �����S�ɖ����ɂ���ꍇ�R�����g�A�E�g���͂���
rem set __NO_HTTPD__=/D "NO_HTTPD"

@rem TK SG SL �ł��ɂ傲�ɂ債�����l�̓R�����g�A�E�g���͂���
rem set __TKSGSL__=/D "TKSGSL"

rem ---------------------------
rem �R���p�C���I�v�V�����ݒ�

set __opt1__=/c /W3 /O2 /Op /GA /TC /I "../common/zlib/" /I "../common/" /D "PACKETVER=6" /D "NEW_006b" /D "FD_SETSIZE=4096"  /D "LOCALZLIB" /D "NDEBUG" /D "_CONSOLE" /D "WIN32" /D "_WIN32" /D "_WIN32_WINDOWS" %__TXT_MODE__% %__CMP_AFL2__% %__CMP_AFIP__% %__NO_HTTPD__% %__TKSGSL__%
set __opt2__=/nologo user32.lib ../common/zlib/*.obj ../common/*.obj *.obj

rem ---------------------------
rem �R���p�C��

cd src\common\zlib
cl %__opt1__% *.c
cd ..\
cl %__opt1__% *.c 

cd ..\login
cl %__opt1__% *.c 
link %__opt2__% /out:"../../bin/login-server.exe"
cd ..\char
cl %__opt1__% *.c 
link %__opt2__% /out:"../../bin/char-server.exe"
cd ..\map
cl %__opt1__% *.c 
link %__opt2__% /out:"../../bin/map-server.exe"

cd ..\..\
pause

del src\common\zlib\*.obj
del src\common\*.obj
del src\char\*.obj
del src\login\*.obj
del src\map\*.obj

