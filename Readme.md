# PHP Debugging Extension
A Windbg Extension to debugging PHP running on Windows IIS(NTS).

##Is there a request running?
0:000> !ispr
One PHP page is currently executing.

##Request URL
0:000> !dcru
/index.php

##Detailed Request information
0:000> !phpexts.dcri  
request_method    : GET  
query_string      :  
cookie_data       : ARRAffinity=d47a70ebf0f6d89c53b96d106c670930dbb5081e809a9def66d1b7a77a831c9d;   WAWebSiteSID=09745f447c6546e984dbe9a8cbf02a12; __utmc=258354654; __utma=258354654.1455641795.1384996945.1385003028.1385024121.3; __utmz=258354654.1384996945.1.1.utmcsr=(direct)|utmcc
content_length    : 0  
path_translated   : C:\DWASFiles\Sites\wzhaosite\VirtualDirectory0\site\wwwroot\index.php  
request_uri       : /index.php  
Request_Time      : 10sec  

##PHP call stack
0:000:x86> !dpcs

   Function Name                      File Name  
00  cleanup                            C:\DWASFiles\Sites\wzhaosite\VirtualDirectory0\site\wwwroot\wp-content\plugins\really-simple-captcha\really-simple-captcha.php@251  
01  wpcf7_cleanup_captcha_files        C:\DWASFiles\Sites\wzhaosite\VirtualDirectory0\site\wwwroot\wp-content\plugins\contact-form-7\modules\captcha.php@418  
02  main                               C:\DWASFiles\Sites\wzhaosite\VirtualDirectory0\site\wwwroot\wp-content\plugins\contact-form-7\modules\captcha.php@439  
03  wpcf7_load_modules                 C:\DWASFiles\Sites\wzhaosite\VirtualDirectory0\site\wwwroot\wp-content\plugins\contact-form-7\settings.php@36  
05  do_action                          C:\DWASFiles\Sites\wzhaosite\VirtualDirectory0\site\wwwroot\wp-includes\plugin.php@406  
06  main                               C:\DWASFiles\Sites\wzhaosite\VirtualDirectory0\site\wwwroot\wp-settings.php@211  
07  main                               C:\DWASFiles\Sites\wzhaosite\VirtualDirectory0\site\wwwroot\wp-config.php@90  
08  main                               C:\DWASFiles\Sites\wzhaosite\VirtualDirectory0\site\wwwroot\wp-load.php@29  
09  main                               C:\DWASFiles\Sites\wzhaosite\VirtualDirectory0\site\wwwroot\wp-blog-header.php@12  
0a  main                               C:\DWASFiles\Sites\wzhaosite\VirtualDirectory0\site\wwwroot\index.php@17



##PHP Core Globals
0:000:x86> !dpcg
output_buffering        : 4096  
memory_limit            : 134217728  
max_input_time          : 60  
track_errors            : 0  
display_errors          : 0  
display_startup_errors  : 0  
display_startup_errors  : 0  
log_errors              : 1  
log_errors_max_len      : 1024  
error_log               : D:\home\LogFiles\php_errors.log  
upload_tmp_dir          : C:\DWASFiles\Sites\wzhaosite\Temp  
upload_max_filesize     : 2097152  
file_uploads            : 1  
variables_order         : GPCS  
last_error_type         : E_WARNING  
last_error_message      : unlink(C:\DWASFiles\Sites\wzhaosite\VirtualDirectory0\site\wwwroot/wp-content/uploads/wpcf7_captcha/1696868155.txt): Permission denied  
last_error_file         : C:\DWASFiles\Sites\wzhaosite\VirtualDirectory0\site\wwwroot\wp-content\plugins\really-simple-captcha\really-simple-captcha.php  
last_error_lineno       : 251  


##Server Variables
0:000:x86> !dpsv  
item #0	_FCGI_X_PIPE_ = 141d118  
item #1	PHP_FCGI_MAX_REQUESTS = 141d1a0  
item #2	PHPRC = 141d200  
item #3	APP_POOL_CONFIG = 1421880  
item #4	APP_POOL_ID = 1421908  
item #5	COMPUTERNAME = 141d370  
item #6	PROCESSOR_ARCHITEW6432 = 141d3e0  
item #7	PUBLIC = 141d460  
item #8	LOCALAPPDATA = 141d4f0  
item #9	PSModulePath = 141d5d8  
item #10	PROCESSOR_ARCHITECTURE = 141d648  
item #11	Path = 141d828  
item #12	CommonProgramFiles(x86) = 141d8b0  
item #13	ProgramFiles(x86) = 141d938  
item #14	PROCESSOR_LEVEL = 141d9a8  
item #15	ProgramFiles = 141da28  
item #16	PATHEXT = 141dac8  
item #17	USERPROFILE = 141dbe0  
item #18	SystemRoot = 141dc50  
item #19	ALLUSERSPROFILE = 141dce0  
item #20	FP_NO_HOST_CHECK = 141dd50  
item #21	ProgramData = 141dde8  
item #22	PROCESSOR_REVISION = 141de50  
item #23	USERNAME = 141dec8  
item #24	CommonProgramW6432 = 141df20  
item #25	CommonProgramFiles = 141dfb0  
item #26	OS = 141e028  
item #27	PROCESSOR_IDENTIFIER = 141e0b8  
item #28	ComSpec = 141e100  
item #29	SystemDrive = 141e168  
item #30	TEMP = 141e1f0  
item #31	NUMBER_OF_PROCESSORS = 141e258  
item #32	APPDATA = 141e2f0  
item #33	TMP = 141e480  
item #34	ProgramW6432 = 141e4f0  
item #35	windir = 141e568  
item #36	USERDOMAIN = 141e5d8  
item #37	WEBSITE_NODE_DEFAULT_VERSION = 141e640  
item #38	APPSETTING_WEBSITE_NODE_DEFAULT_VERSION = 141e670  
item #39	PHP_ZENDEXTENSIONS = 141e6c0  
item #40	APPSETTING_PHP_ZENDEXTENSIONS = 141e750  
item #41	REMOTEDEBUGGINGVERSION = 141e7d8  
item #42	APPSETTING_REMOTEDEBUGGINGVERSION = 141e858  
item #43	ScmType = 141e8d8  
item #44	APPSETTING_ScmType = 141e940  
item #45	MYSQLCONNSTR_wzhaosite = 141ea18  
item #46	HOME = 141eac0  
item #47	windows_tracing_flags = 141eb28  
item #48	windows_tracing_logfile = 141eba0  
item #49	WEBSITE_INSTANCE_ID = 141ec58  
item #50	WEBSITE_COMPUTE_MODE = 141ecd0  
item #51	WEBSITE_SITE_MODE = 141ed48  
item #52	WEBSITE_SITE_NAME = 141edc0  
item #53	REMOTEDEBUGGINGPORT = 141ee30  
item #54	REMOTEDEBUGGINGBITVERSION = 141eea0  
item #55	WEBSOCKET_CONCURRENT_REQUEST_LIMIT = 141ef18  
item #56	ORIG_PATH_INFO = 141f090  
item #57	URL = 141f170  
item #58	SERVER_SOFTWARE = 141f260  
item #59	SERVER_PROTOCOL = 141f350  
item #60	SERVER_PORT_SECURE = 141f430  
item #61	SERVER_PORT = 141f508  
item #62	SERVER_NAME = 141f5e8  
item #63	SCRIPT_NAME = 141f6c8  
item #64	SCRIPT_FILENAME = 141f868  
item #65	REQUEST_URI = 141fb48  
item #66	REQUEST_METHOD = 141fc20  
item #67	REMOTE_USER = 141fcf8  
item #68	REMOTE_PORT = 141fdc8  
item #69	REMOTE_HOST = 141fea8  
item #70	REMOTE_ADDR = 141ff88  
item #71	QUERY_STRING = 1420060  
item #72	PATH_TRANSLATED = 14201c0  
item #73	LOGON_USER = 1420298  
item #74	LOCAL_ADDR = 1420378  
item #75	INSTANCE_META_PATH = 1420470  
item #76	INSTANCE_NAME = 1420560  
item #77	INSTANCE_ID = 1420648  
item #78	HTTPS_SERVER_SUBJECT = 14206e0  
item #79	HTTPS_SERVER_ISSUER = 14207c8  
item #80	HTTPS_SECRETKEYSIZE = 14208a8  
item #81	HTTPS_KEYSIZE = 1420988  
item #82	HTTPS = 1420a60  
item #83	GATEWAY_INTERFACE = 1420b38  
item #84	DOCUMENT_ROOT = 1420c88  
item #85	CONTENT_TYPE = 1420d68  
item #86	CONTENT_LENGTH = 1420e48  
item #87	CERT_SUBJECT = 1420f28  
item #88	CERT_SERIALNUMBER = 1421008  
item #89	CERT_ISSUER = 14210e0  
item #90	CERT_FLAGS = 14211b0  
item #91	CERT_COOKIE = 1421280  
item #92	AUTH_USER = 1421350  
item #93	AUTH_PASSWORD = 1421428  
item #94	AUTH_TYPE = 1421500  
item #95	APPL_PHYSICAL_PATH = 1421648  
item #96	APPL_MD_PATH = 1421730  
item #97	IIS_UrlRewriteModule = 1421940  
item #98	LOG_QUERY_STRING = 1421a88  
item #99	HTTP_X_ARR_LOG_ID = 1421ba8  
item #100	HTTP_X_FORWARDED_FOR = 1421cb0  
item #101	HTTP_X_ORIGINAL_URL = 1421d98  
item #102	HTTP_X_MS_REQUEST_ID = 1421ec0  
item #103	HTTP_X_LIVEUPGRADE = 1421fa8  
item #104	HTTP_USER_AGENT = 1422108  
item #105	HTTP_MAX_FORWARDS = 14221e8  
item #106	HTTP_HOST = 14222d0  
item #107	HTTP_COOKIE = 14225d0  
item #108	HTTP_ACCEPT_LANGUAGE = 1422700  
item #109	HTTP_ACCEPT_ENCODING = 1422800  
item #110	HTTP_ACCEPT = 1422920  
item #111	HTTP_CONTENT_LENGTH = 14229f8  
item #112	HTTP_CONNECTION = 1422ae8  
item #113	FCGI_ROLE = 1422bd0  
item #114	PHP_SELF = 1422cb0  
item #115	REQUEST_TIME_FLOAT = 1422d08  
item #116	REQUEST_TIME = 1422d68  

To get details of the value, for example, **REMOTE_HOST**  
item #69	REMOTE_HOST = **141fea8**  

0:000:x86> da poi(141fea8)  
0141fe90  "167.220.224.44"  


##Cookies
0:000:x86> !dpck  
item #0	ARRAffinity = 141ca80  
item #1	WAWebSiteSID = 141cbf0  
item #2	__utmc = 141ccf0  
item #3	__utma = 141ce60  
item #4	__utmz = 141cfb0  


##Dump HTTP globals
!dphgs <id>  
track ID out of index, valid track ID:  
TRACK_VARS_POST		0  
TRACK_VARS_GET		1  
TRACK_VARS_COOKIE	2  
TRACK_VARS_SERVER	3  
TRACK_VARS_ENV		4  
TRACK_VARS_FILES	5  
//TRACK_VARS_REQUEST	6(only 6 in total actually)  

##Dump specific HTTP globals
!dphg <id> <global name>  

0:000:x86> !dphg 3 **REQUEST_TIME**  
REQUEST_TIME = **1422d68**  

To get the time when request started  
0:000:x86> .formats poi(1422d68)  
Evaluate expression:  
  ...  
  Time:    *Thu Nov 21 17:33:07 2013*  
  ...  

