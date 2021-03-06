@echo off
rem VC++ 6.0 / VC++ .net / VC++ .net 2003 / VC++ .net 2005 / VC++ ToolKit 2003 でのビルド用バッチファイル

rem ---------------------------
rem パスの設定

rem VC++ Toolkit
Set PATH=C:\Program Files\Microsoft Visual C++ Toolkit 2003\bin;C:\Program Files\Microsoft Platform SDK\Bin;C:\Program Files\Microsoft Platform SDK\Bin\winnt;C:\Program Files\Microsoft Platform SDK\Bin\Win64;%PATH%
Set INCLUDE=C:\Program Files\Microsoft Visual C++ Toolkit 2003\include;C:\Program Files\Microsoft Platform SDK\include;%INCLUDE%
Set LIB=C:\Program Files\Microsoft Visual C++ Toolkit 2003\lib;C:\Program Files\Microsoft Platform SDK\Lib;%LIB%

rem VC++ .net 2005
rem Set PATH=C:\Program Files\Microsoft Visual Studio 8\VC\bin;C:\Program Files\Microsoft Platform SDK\Bin;C:\Program Files\Microsoft Platform SDK\Bin\winnt;C:\Program Files\Microsoft Platform SDK\Bin\Win64;%PATH%
rem Set INCLUDE=C:\Program Files\Microsoft Visual Studio 8\VC\include;C:\Program Files\Microsoft Platform SDK\include;%INCLUDE%
rem Set LIB=C:\Program Files\Microsoft Visual Studio 8\VC\lib;C:\Program Files\Microsoft Platform SDK\Lib;%LIB%

rem VC++ .net 2005 / 必要ならコメントアウトをはずす
rem call "C:\Program Files\Microsoft Visual Studio 8\VC\bin\VCVARS32.BAT"

rem VC++ .net 2003 / 必要ならコメントアウトをはずす
rem call "C:\Program Files\Microsoft Visual Studio .NET 2003\Vc7\bin\vcvars32.bat"

rem VC++ .net (2002) / 必要ならコメントアウトをはずす
rem call "C:\Program Files\Microsoft Visual Studio .NET\Vc7\bin\vcvars32.bat"

rem VC++ 6.0 / 必要ならコメントアウトをはずす
rem ここ以外に __opt2__ の /MAP と /nologo を消す必要がある
rem call "C:\Program Files\Microsoft Visual Studio\VC98\Bin\vcvars32.bat"

rem ---------------------------
rem ビルドオプションの選択

rem txt/sql 選択 ： sql にするならコメントアウトする
set __TXT_MODE__=/D "TXT_ONLY"

rem txt で、ジャーナルを使うならコメントアウトをはずす
rem set __TXT_MODE__=/D "TXT_ONLY" /D "TXT_JOURNAL"

rem login_id2 や IP でごにょごにょしたい人はコメントアウトをはずす
rem set __CMP_AFL2__=/D "CMP_AUTHFIFO_LOGIN2"
rem set __CMP_AFIP__=/D "CMP_AUTHFIFO_IP"

rem httpd を完全に無効にする場合コメントアウトをはずす
rem set __NO_HTTPD__=/D "NO_HTTPD"

rem httpd で外部 CGI を使う場合はコメントアウトする
set __NO_HTTPD_CGI__=/D "NO_HTTPD_CGI"

rem csvdb を完全に無効にする場合コメントアウトをはずす
rem set __NO_CSVDB__=/D "NO_CSVDB"

rem csvdb のスクリプトからの利用を無効にする場合コメントアウトをはずす
rem set __NO_CSVDB_SCRIPT__=/D "NO_CSVDB_SCRIPT"

rem TK SG SL でごにょごにょしたい人はコメントアウトをはずす
rem set __EXCLASS__=/D "TKSGSL"
rem set __EXCLASS__=/D "TKSGSLGSNJ"

rem 動的にMOBのsc_dataを確保したい人はコメントアウトをはずす
rem set __DYNAMIC_STATUS_CHANGE__=/D "DYNAMIC_SC_DATA"

rem account regist MailAddress
rem set __AC_MAIL__=/D "AC_MAIL"

rem ---------------------------
rem コンパイルオプション設定

set __opt1__=/c /W3 /O2 /Op /GA /TC /Zi /I "../common/zlib/" /I "../common/" /D "PACKETVER=6" /D "NEW_006b" /D "FD_SETSIZE=4096"  /D "LOCALZLIB" /D "NDEBUG" /D "_CONSOLE" /D "WIN32" /D "_WIN32" /D "_WIN32_WINDOWS" /D "_CRT_SECURE_NO_DEPRECATE" %__TXT_MODE__% %__CMP_AFL2__% %__CMP_AFIP__% %__NO_HTTPD__% %__NO_HTTPD_CGI__% %__NO_CSVDB__% %__NO_CSVDB_SCRIPT__% %__EXCLASS__% %__DYNAMIC_STATUS_CHANGE__% %__AC_MAIL__%
set __opt2__=/DEBUG /MAP /nologo user32.lib ../common/zlib/*.obj ../common/*.obj *.obj

rem ---------------------------
rem コンパイル

cd src\common\zlib
cl %__opt1__% *.c
cd ..\
cl %__opt1__% *.c 

cd ..\login
cl %__opt1__% *.c 
link %__opt2__% /out:"../../login-server.exe"
cd ..\char
cl %__opt1__% *.c 
link %__opt2__% /out:"../../char-server.exe"
cd ..\map
cl %__opt1__% *.c 
link %__opt2__% /out:"../../map-server.exe"
cd ..\converter
cl %__opt1__% *.c 
link %__opt2__% /out:"../../txt-converter.exe"

cd ..\..\
pause

del src\common\zlib\*.obj
del src\common\*.obj
del src\char\*.obj
del src\login\*.obj
del src\map\*.obj
del src\converter\*.obj
