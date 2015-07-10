/*-----------------------------------------------------------------------------
   Copyright (c) 2000  Microsoft Corporation

Module:
  exts.cpp

  Sampe file showing couple of extension examples
  
  //cts-wzhao@live.com added PHP debugging feature based on the sample
-----------------------------------------------------------------------------*/
#include "phpexts.h"
#include "constants.h"

#pragma region TRACING_UTILS


bool isDbg()
{
#ifdef _DEBUG
	return 1;
#else
	return 0;
#endif

}


//#define DBG_TRACE (dprintf("%s(%d)-<%s>: ", __FILE__, __LINE__, __FUNCTION__), dprintf)
#define DBG_TRACE(fmt, ...) \
		if (isDbg()) \
							{dprintf("%s(%d)-<%s>: "##fmt, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); dprintf("\n");}




void DBG_OUT(PCSTR format, ULONG64 value)
{
	#ifdef _DEBUG
		dprintf("\n******\t");
		dprintf(format, value);
		dprintf("\t******\n");
	#endif
}

#pragma endregion


//trim spaces
#define SKIP_WSPACE(s)  while (*s && (*s == ' ' || *s == '\t')) {++s;}  


/*
Read string from the specified address with specified length
*/
HRESULT readString(ULONG_PTR offSet, ULONG len, char* buf)
{
	CHAR mem;
	ULONG bytes;
	ULONG_PTR address = offSet;
	ULONG i = 0;
	char* out = buf;

	while (i < len)
	{
		if (ReadMemory(address + i, &mem, sizeof(CHAR), &bytes) && (bytes == sizeof(CHAR)))
		{
			if (mem == 0)
			{
				break;
			}
			else
			{
				*out = mem;
				i++;
				out++;
			}
		}
		else
		{
			return S_FALSE;
		}
	}

	*out = '\0';
	return S_OK;
}

//
//When the request started
//php_cgi!_sapi_globals_struct->global_request_time
//
HRESULT getRequestExecutingTime(ULONG *execTime)
{
	ULONG64 Address;
	ULONG64 sapiGlobal;
	ULONG_PTR ptr;

	double rqTime;

	Address= GetExpression("php_cgi!_imp__sapi_globals");
	if (!Address)
	{
		dprintf("Error in getting php_cgi!_imp__sapi_globals");
		return S_FALSE;
	}

	DBG_TRACE("php_cgi!_imp__sapi_globals : %x", Address);

	if (!ReadPointer(Address, &sapiGlobal))
	{
		dprintf("Error in reading _sapi_globals_struct at %x", Address);
		return S_FALSE;
	}

	//ULONG64 ptrGloblas = 0x6fc263a0;
	
	ptr = (ULONG_PTR)sapiGlobal;

	if (GetFieldValue(ptr, "php_cgi!_sapi_globals_struct", "global_request_time", rqTime)) {
		dprintf("Error in reading global_request_time at %p\n",	ptr);
		return S_FALSE;
	}

	DBG_TRACE("global_request_time: float:%f double:(%f) ULONG:(%x)", (float)rqTime, rqTime, (ULONG)rqTime);

	*execTime = (ULONG)rqTime;

	return S_OK;
}

void printRequestTimeTaken()
{
	ULONG exeTime;

	if (S_FALSE == getRequestExecutingTime(&exeTime))
		dprintf("Request_Time: NULL");
	else
	{
		dprintf("%s% 6s: ", "Request_Time", " ");
		dprintf("%dsec\n", g_DbgSessionTime - exeTime);
	}
}


//for bucket items key and data
struct Track_Var {
	ULONG_PTR pData;
	PCHAR     key;
};


//php_cgi!bucket
//+ 0x000 h                : 0x72417520
//+ 0x004 nKeyLength : 0xe
//+ 0x008 pData : 0x0150153c Void
//+ 0x00c pDataPtr : 0x014fef00 Void
//+ 0x010 pListNext : 0x015015a0 bucket
//...
//+0x020 arKey : 0x01501554  "_FCGI_X_PIPE_"

HRESULT readBuckets(ULONG_PTR address, ULONG *size, ULONG_PTR out)
{
	ULONG keyLen;
	ULONG64 key;
	ULONG64 data;
	ULONG64 next;
	struct Track_Var *bucket = (struct Track_Var *)out;
	char *buf;
	ULONG i;

	for (i = 0; i < *size; i++)
	{
		
		DBG_TRACE("item #%d, result %x", i, bucket);

		//Key Length
		if (GetFieldValue(address, "php_cgi!bucket", "nKeyLength", keyLen)) {
			dprintf("Error in reading php_cgi!bucket->nKeyLength at %\n", address);
			return S_FALSE;
		}

		DBG_TRACE("\tkeyLen %d", keyLen);

		if (keyLen == 0)
			continue;

		//Key
		if (GetFieldValue(address, "php_cgi!bucket", "arKey", key)) {
			dprintf("Error in reading php_cgi!bucket->key at %\n", address);
			return S_FALSE;
		}

		DBG_TRACE("\tkey %x", key);

		buf = (char*)malloc(keyLen);
		if (NULL == buf)
			return S_FALSE;

		readString((ULONG_PTR)key, keyLen, buf);
		DBG_TRACE("\tarKey %s", buf);

		//Data
		if (GetFieldValue(address, "php_cgi!bucket", "pDataPtr", data)) {
			dprintf("Error in reading php_cgi!bucket->data at %\n", address);
			return S_FALSE;
		}
		DBG_TRACE("\tdata %x", data);


		bucket->pData = (ULONG_PTR)data;
		bucket->key = buf;

		//next item
		if (GetFieldValue(address, "php_cgi!bucket", "pListNext", next)) {
			dprintf("Error in reading php_cgi!bucket->pListNext at %\n", address);
			return S_FALSE;
		}

		address = (ULONG_PTR)next;
		DBG_TRACE("\tnext item %x", address);
		if (!address)
		{
			DBG_TRACE("\taddress is NULL");
			//the item starts from 0x0
			*size = i + 1;
			break;
		}
		bucket++;
	}
	
	DBG_TRACE("\tread %d items in total", i+1);

	return S_OK;
}


//
//0:000> dt _hashtable 01501500
//php_cgi!_hashtable
//+ 0x008 nNumOfElements : 0x7d
//+ 0x014 pListHead : 0x01501530 bucket
//+ 0x018 pListTail : 0x01507b00 bucket
//+ 0x01c arBuckets : 0x01503ce8  -> 0x01507780 bucket
//
ULONG getNumOfElements(LONG_PTR address)
{
	ULONG numOfElements;

	if (GetFieldValue(address, "php_cgi!_hashtable", "nNumOfElements", numOfElements)) {
		dprintf("Error in reading php_cgi!_hashtable->nNumOfElements at %p\n", address);
		return 0;
	}

	return numOfElements;
}

HRESULT readHashTable(ULONG_PTR address, ULONG* numOfElems, ULONG_PTR *out)
{
	ULONG64 head;
	
	if (GetFieldValue(address, "php_cgi!_hashtable", "pListHead", head)) {
		dprintf("Error in reading php_cgi!_hashtable->pListHead at %p\n", address);
		return S_FALSE;
	}

	*numOfElems = getNumOfElements(address);
	DBG_TRACE("# of elements %d\n", *numOfElems);
	
	if (0 == numOfElems)
		*out = NULL;

	*out = (ULONG_PTR)malloc(sizeof(struct Track_Var) * (*numOfElems));
	if (NULL == out)
		return S_FALSE;

	DBG_TRACE("out %x(%x)", out, (*out));

	//return S_OK;
	
	if (S_FALSE == readBuckets((ULONG_PTR)head, numOfElems, *out))
	{
		DBG_TRACE("readBuckets failed %x\n", head);
		if (NULL != out)
			free(out);
		return S_FALSE;
	}

	DBG_TRACE("Real #of items #%d, result %x", *numOfElems, *out);

	return S_OK;

}


void freeHashTable(struct Track_Var* address, ULONG size)
{
	struct Track_Var *bucket = (struct Track_Var *)address;
	ULONG i;

	for (i = 0; i < size; i++)
	{
		DBG_TRACE("bucket = %x", bucket);
		DBG_TRACE("item #%d", i);
		if (bucket->key)
			free(bucket->key);

		bucket++;
	}

	free(address);

}


HRESULT getHTTPGlobalsPtr(ULONG64 *address)
{
	ULONG64 coreGlobals, ptrGlobals;
	ULONG offset;

	coreGlobals = GetExpression("php_cgi!_imp__core_globals");
	DBG_TRACE("php_cgi!_imp__core_globals : %x", coreGlobals);

	if (!ReadPointer(coreGlobals, &ptrGlobals))
	{
		dprintf("Error in reading _php_core_globals at %p\n",
			coreGlobals);
		return S_FALSE;
	}

	ptrGlobals = ptrGlobals & 0xffffffff;

	if (GetFieldOffset("php_cgi!_php_core_globals", "http_globals", &offset) != 0)
	{
		dprintf("Error in reading _php_core_globals->http_globals offset");
		return S_FALSE;
	}

	*address = (ULONG_PTR)ptrGlobals + offset;
	DBG_TRACE("php_cgi!_php_core_globals->http_globals : %x", *address);

/*

	DBG_TRACE("php_cgi!_imp__core_globals : %x", ptrGlobals);

	if (GetFieldData(ptrGlobals, "php_cgi!_php_core_globals", "http_globals", sizeof(ULONG_PTR) * TRACK_VARS_REQUEST, ht) != 0)
	{
		dprintf("Error in reading _php_core_globals->http_globals at %x", ptrGlobals);
		return S_FALSE;
	}
*/
	//if (GetFieldValue(ptrGlobals, "php_cgi!_php_core_globals", "http_globals", ht)) {
	//	dprintf("Error in reading php_cgi!_php_core_globals->http_globals at %p\n", ptrGlobals);
	//	return S_FALSE;
	//}


	//for (int i = 0; i < TRACK_VARS_REQUEST; i++)
	//	//*address = (ULONG_PTR)(httpGlobals & 0xffffffff);
	//	DBG_TRACE("php_cgi!_php_core_globals->http_globals : %x", ht[i]);

	return S_OK;
}


void printHashTableItems(ULONG_PTR result, ULONG size)
{
	struct Track_Var *bucket = (struct Track_Var *)result;
	ULONG i;

	DBG_TRACE("result = %x", result);

	for (i = 0; i < size; i++)
	{
		DBG_TRACE("bucket = %x", bucket);
		dprintf("item #%d", i);
		dprintf("\t%s = %x\n", bucket->key, bucket->pData);
		
		bucket++;
	}

}

//HRESULT dumpHttpGlobal(int track)
//{
//	if (GetExpressionEx(args, &Address, &args)) {
//		Value = (ULONG)GetExpression(args);
//	}
//	else {
//		dprintf("Usage:   !edit <address> <value>\n");
//		return;
//	}
//
//}
/*
0:000> dt php_cgi!_php_core_globals http_globals 6bc5a260
+ 0x0b8 http_globals : [6] 0x014fc4b8 _zval_struct
*/
ULONG_PTR getHTTPGlobals(int track, ULONG *numOfElems, ULONG_PTR *out)
{
	ULONG64 httpGlobals;
	ULONG_PTR ht;
	ULONG64 pht;
	ULONG cb;
	ULONG_PTR result = 0;

	DBG_TRACE("http_globals track ID : %d", track);

	if (track > TRACK_VARS_REQUEST)
	{
		return S_FALSE;
	}


	//Get the php_cgi!_php_core_globals->http_globals address
	if (getHTTPGlobalsPtr(&httpGlobals) == S_FALSE)
		return S_FALSE;

	if (httpGlobals == NULL)
	{
		return S_FALSE;
	}

	DBG_TRACE("http_globals : %x", (ULONG_PTR)httpGlobals);

	//Get the hashtable address by given track ID
	if (!(ReadMemory(((ULONG_PTR)httpGlobals) + track*sizeof(ULONG_PTR), &ht, sizeof(ht), &cb) && cb == sizeof(ht)))
	{
		DBG_TRACE("failed to read http_globals track ID : %d, htp_Globals %x: ", track, httpGlobals);
		return S_FALSE;

	}
	DBG_TRACE("hashtable for track %d is %x", track, ht);

	if (ReadPointer(ht, &pht))
	{
		DBG_TRACE("hashtable of http_globals[%d] at %x", track, (ULONG_PTR)pht);
		return readHashTable((ULONG_PTR)pht, numOfElems, out);

		/*if (S_OK == readHashTable(pht, numOfElems, out))
		{
			*numOfElems = size;
			*out = result;
			return S_OK;
		}*/
	}

	return S_FALSE;
}


HRESULT printHttpGlobals(ULONG track)
{
	ULONG64 httpGlobals;
	ULONG_PTR ht;
	ULONG64 pht;
	ULONG cb;
	ULONG_PTR result = 0;
	ULONG size;

	//Get the php_cgi!_php_core_globals->http_globals address
	if (getHTTPGlobalsPtr(&httpGlobals) == S_FALSE)
		return S_FALSE;

	if (httpGlobals == NULL)
	{
		return S_FALSE;
	}

	DBG_TRACE("http_globals : %x", (ULONG_PTR)httpGlobals);


	//Get the hashtable address by given track ID
	if (!(ReadMemory(((ULONG_PTR)httpGlobals) + track*sizeof(ULONG_PTR), &ht, sizeof(ht), &cb) && cb == sizeof(ht)))
	{
		DBG_TRACE("failed to read http_globals track ID : %d, htp_Globals %x: ", track, httpGlobals);
		return S_FALSE;

	}
	DBG_TRACE("hashtable for track %d is %x", track, ht);

	if (ReadPointer(ht, &pht))
	{
		DBG_TRACE("hashtable of http_globals[%d] at %x", track, (ULONG_PTR)pht);
		if (S_OK == readHashTable((ULONG_PTR)pht, &size, &result))
		{
			DBG_TRACE("hashtable %x, #of items %d, result %x(%x)", (ULONG_PTR)pht, size, result, &result);
			if (size > 0)
				printHashTableItems(result, size);
			else
				dprintf("no items found\n");

			freeHashTable((struct Track_Var*)result, size);

			return S_OK;
		}

		
	}

	dprintf("failed to print HTTP Globals\n");

	return S_FALSE;

}

HRESULT copyBucket(struct Track_Var* src, struct Track_Var* dst)
{
	size_t len = strlen(src->key) + 1;

	DBG_TRACE("src = %x, dst = %x", src, dst);
	
	DBG_TRACE("%s(%x)(%d)", src->key, len);

	dst->key = (CHAR*)malloc(len);

	if (src->key == NULL)
		return S_FALSE;
	
	DBG_TRACE("%s(%x)", src->key, src->key);
	memcpy_s(dst->key, len, src->key, len);
	//dst->key[len] = '\0';

	//strcpy_s(dst->key, strlen(src->key), src->key);

	dst->pData = src->pData;

	DBG_TRACE("%s = %x", src->key, src->pData);
	DBG_TRACE("%s = %x", dst->key, dst->pData);

	return S_OK;

}
ULONG_PTR getHttpGlobal(ULONG track, PCSTR var)
{
	ULONG size;
	ULONG_PTR globals;
	struct Track_Var *bucket;
	struct Track_Var *out;
	ULONG i;

	DBG_TRACE("Server Variable Name:%s", var);

	if (S_OK != getHTTPGlobals(track, &size, &globals))
		return NULL;

	DBG_TRACE("#of elements:%d", size);
	
	DBG_TRACE("result = %x", globals);
	bucket = (struct Track_Var *)globals;

	for (i = 0; i < size; i++)
	{
		DBG_TRACE("%s = %x", bucket->key, bucket->pData);
		if (!strcmp(var, bucket->key))
		{
			out = (struct Track_Var*)malloc(sizeof(struct Track_Var));
			if (out == NULL)
				return NULL;

			if (S_FALSE == copyBucket(bucket, out))
				return NULL;

			freeHashTable((struct Track_Var *)globals, size);

			return (ULONG_PTR)out;
		}
		bucket++;
	}

	return NULL;
}

void printHttpGlobal(struct Track_Var *bucket)
{
	DBG_TRACE("bucket = %x", bucket);
	dprintf("%s = %x\n", bucket->key, bucket->pData);
	
	return;
}


void printStrHttpGlobal(struct Track_Var *bucket)
{
	char buf[MAX_PATH];
	ULONG64 var;

	DBG_TRACE("bucket = %x", bucket);
	DBG_TRACE("%s = %x\n", bucket->key, bucket->pData);
		
	if (!ReadPointer(bucket->pData, &var))
	{
		dprintf("Error in reading bucket data at %x\n",
			bucket->pData);
		return;
	}

	readString((ULONG_PTR)var, MAX_PATH, buf);
	
	dprintf("%s : %s\n", bucket->key, buf);
}

HRESULT dumpStringWithWidth(ULONG64 offSet, int width)
{
	CHAR mem;
	ULONG bytes;
	ULONG64 address = offSet;
	int len = 0;
	while (len < MAX_PATH)
	{
		if (!ReadMemory(address+len, &mem, 1, &bytes))
			return S_FALSE;
		if (mem == 0)
		{
			break;
		}
		else
		{
			dprintf("%c", mem);
			len++;
		}
	}
	
	while (len < width)
	{
		dprintf(" ");
		len++;
	}
	return S_OK;
}

HRESULT dumpString(ULONG64 offSet)
{
	CHAR mem;
	ULONG bytes;
	ULONG64 address = offSet;
	int len = 0;
	while (len < MAX_PATH)
	{
		if (!ReadMemory(address + len, &mem, 1, &bytes))
			return S_FALSE;
		if (mem ==0)
		{
			break;
		}
		else
		{
			if (mem > 31) //ignore invisible
				dprintf("%c", mem);
			else 
				dprintf("%c", '.');
			len++;
		}
	}

	return S_OK;
}

HRESULT isRunning()
{
	ULONG64 Address;
	ULONG64 inExec;

	Address = GetExpression("php5!executor_globals");

	if (GetFieldValue(Address, "php5!executor_globals", "in_execution", inExec)) {
		dprintf("Error in reading in_execution at %p\n", Address);
		return S_FALSE;
	}

	if (inExec == 0)
	{
		return S_FALSE;
	}
	
	return S_OK;
}

extern "C"
HRESULT CALLBACK
ispr(PDEBUG_CLIENT4 Client, PCSTR args)
{
	if (isRunning() == S_OK)
	{
		dprintf("One PHP page is currently executing.\n");
		return S_OK;
	}
	else
	{
		dprintf("There is no PHP page currently executing.\n");
		return S_FALSE;
	}
}

extern "C"
HRESULT CALLBACK
isPHPPageRunning(PDEBUG_CLIENT4 Client, PCSTR args)
{
	return ispr(Client, args);
}



extern "C"
HRESULT CALLBACK
dumpPhpCallStack(PDEBUG_CLIENT4 Client, PCSTR args)
{
	ULONG64 Address;
	ULONG64 curExeData;

	INIT_API();

	UNREFERENCED_PARAMETER(args);

	if (isRunning() == S_FALSE)
	{
		dprintf("There is no PHP page currently executing.\n");
		return S_FALSE;
	}

	Address = GetExpression("php5!executor_globals");

	if (GetFieldValue(Address, "php5!executor_globals", "current_execute_data", curExeData)) {
		dprintf("Error in reading in_execution at %p\n",
			Address);
		return S_FALSE;
	}

	DBG_TRACE("current_execute_data : %p", curExeData);
	if (curExeData)
	{
		dprintf("#   ");
		dprintf("Function Name");
		dprintf("%22s", " ");
		dprintf("File Name\n");
	}

	int frameNum = 0;
	while (curExeData)
	{
		ULONG64 opLine;
		ULONG64 opArray;

		if (GetFieldValue(curExeData, "php5!_zend_execute_data", "opline", opLine)) {
			dprintf("Error in reading _zend_execute_data at %p\n",
				curExeData);
		}

		DBG_TRACE("opline : %p", opLine);


		if (GetFieldValue(curExeData, "php5!_zend_execute_data", "op_array", opArray)) {
			dprintf("Error in reading _zend_execute_data at %p\n",
				curExeData);
		}

		DBG_TRACE("op_array : %p", opArray);


		ULONG64 lineno = 0;
		if (opLine)
		{
			if (GetFieldValue(opLine, "php5!_zend_op", "lineno", lineno)) {
				dprintf("Error in reading _zend_op at %p\n",
					opLine);
			}

			DBG_TRACE("lineno : %p", opLine);
		}

		ULONG64 funcName;
		if (opArray)
		{
			if (GetFieldValue(opArray, "php5!_zend_op_array", "function_name", funcName)) {
				dprintf("Error in reading _zend_op_array at %p\n",
					opArray);
			}

			DBG_TRACE("function name : %p", funcName);

			dprintf("%02x  ", frameNum);

			if (funcName)
			{
				dumpStringWithWidth(funcName, 35);
			}
			else
			{
				dprintf("%s% 31s", "main", " ");
			}

			ULONG64 fileName;
			if (GetFieldValue(opArray, "php5!_zend_op_array", "filename", fileName)) {
				dprintf("Error in reading _zend_op_array at %p\n",
					opArray);
			}

			DBG_TRACE("file name : %p", fileName);

			if (fileName)
			{
				dumpString(fileName);
				dprintf("@%i\n", lineno);
			}
			else
			{
				dprintf("\n");
			}
			
		}

		ULONG64 preExeData;
		if (GetFieldValue(curExeData, "php5!_zend_execute_data", "prev_execute_data", preExeData)) {
			dprintf("Error in reading _zend_execute_data at %p\n",
				curExeData);
		}

		DBG_TRACE("prev_execute_data : %p", preExeData);

		curExeData = preExeData;
		frameNum++;
	}



	EXIT_API();
	return S_OK;
}


extern "C"
HRESULT CALLBACK
dpcs(PDEBUG_CLIENT4 Client, PCSTR args)
{
	return dumpPhpCallStack(Client, args);
}


extern "C"
HRESULT CALLBACK
dumpCurrentRequestUrl(PDEBUG_CLIENT4 Client, PCSTR args)
{
	ULONG64 Address;
	ULONG64 sapiGlobal;

	ULONG pOffset;
	ULONG_PTR requestInfo;

	INIT_API();

	UNREFERENCED_PARAMETER(args);

	if (isRunning() == S_FALSE)
	{
		dprintf("There is no PHP page currently executing.\n");
		return S_FALSE;
	}

	Address = GetExpression("php_cgi!_imp__sapi_globals");
	if (!Address)
	{
		dprintf("Error in getting php_cgi!_imp__sapi_globals\n");
		return S_FALSE;
	}

	DBG_TRACE("php_cgi!_imp__sapi_globals : %p", Address);

	if (!ReadPointer(Address, &sapiGlobal))
	{
		dprintf("Error in reading _sapi_globals_struct at %p\n",
			Address);
		return S_FALSE;
	}

	DBG_TRACE("sapi_globals : %p", sapiGlobal);

	if (GetFieldOffset("php_cgi!_sapi_globals_struct", "request_info", &pOffset) != 0) {
		dprintf("Error in reading request_info offset\n");
		return S_FALSE;
	}

	requestInfo = (ULONG_PTR)sapiGlobal + pOffset;

	DBG_TRACE("request_info : %p", requestInfo);

	ULONG64 requestUri;

	if (GetFieldValue(requestInfo, "php_cgi!sapi_request_info", "request_uri", requestUri)) {
		dprintf("Error in reading request_uri at %p\n",
			requestUri);
	}
	DBG_TRACE("request_info : %p", requestUri);

	dumpString(requestUri);

	dprintf("\n");

	EXIT_API();
	return S_OK;
}

extern "C"
HRESULT CALLBACK
dcru(PDEBUG_CLIENT4 Client, PCSTR args)
{
	return dumpCurrentRequestUrl(Client, args);
}

extern "C"
HRESULT CALLBACK
dumpCurrentRequestInfo(PDEBUG_CLIENT4 Client, PCSTR args)
{
	ULONG64 Address;
	ULONG64 sapiGlobal;
	ULONG_PTR requestInfo;
	ULONG offset;
	ULONG64 fieldPtr;


	INIT_API();

	UNREFERENCED_PARAMETER(args);

	if (isRunning() == S_FALSE)
	{
		dprintf("There is no PHP page currently executing.\n");
		return S_FALSE;
	}

	Address = GetExpression("php_cgi!_imp__sapi_globals");

	if (!ReadPointer(Address, &sapiGlobal))
	{
		dprintf("Error in reading _imp__sapi_globals at %p\n",
			Address);
		return S_FALSE;
	}

	if (GetFieldOffset("php_cgi!_sapi_globals_struct", "request_info", &offset) != 0) {
		dprintf("Error in reading request_info offset\n");
		return S_FALSE;
	}

	requestInfo = (ULONG_PTR)sapiGlobal + offset;

	DBG_TRACE("request_info : %p", requestInfo);


	//Request Method
	if (GetFieldValue(requestInfo, "php_cgi!sapi_request_info", "request_method", fieldPtr)) {
		dprintf("Error in reading request_method at %p\n",
			fieldPtr);
	}

	DBG_TRACE("request_method : %p", fieldPtr);

	dprintf("%s% 3s : ", "request_method", " ");
	dumpString(fieldPtr);
	dprintf("\n");


	//Query String
	if (GetFieldValue(requestInfo, "php_cgi!sapi_request_info", "query_string", fieldPtr)) {
		dprintf("Error in reading query_string at %p\n",
			fieldPtr);
	}

	DBG_TRACE("query_string : %p", fieldPtr);

	dprintf("%s% 5s : ", "query_string", " ");
	dumpString(fieldPtr);
	dprintf("\n");

	//cookie data
	if (GetFieldValue(requestInfo, "php_cgi!sapi_request_info", "cookie_data", fieldPtr)) {
		dprintf("Error in reading cookie_data at %p\n",
			fieldPtr);
	}

	DBG_TRACE("cookie_data : %p", fieldPtr);

	dprintf("%s% 6s : ", "cookie_data", " ");
	dumpString(fieldPtr);
	dprintf("\n");

	//content_length
	if (GetFieldValue(requestInfo, "php_cgi!sapi_request_info", "content_length", fieldPtr)) {
		dprintf("Error in reading content_length at %p\n",
			fieldPtr);
	}

	DBG_TRACE("content_length : %d", fieldPtr);

	dprintf("%s% 3s : ", "content_length", " ");
	dprintf("%d", fieldPtr);
	dprintf("\n");

	//path_translated
	if (GetFieldValue(requestInfo, "php_cgi!sapi_request_info", "path_translated", fieldPtr)) {
		dprintf("Error in reading path_translated at %p\n",
			fieldPtr);
	}

	DBG_TRACE("path_translated : %p", fieldPtr);

	dprintf("%s% 3s: ", "path_translated", " ");
	dumpString(fieldPtr);
	dprintf("\n");

	//request_uri
	if (GetFieldValue(requestInfo, "php_cgi!sapi_request_info", "request_uri", fieldPtr)) {
		dprintf("Error in reading request_uri at %p\n",
			fieldPtr);
	}

	DBG_TRACE("request_uri : %p", fieldPtr);

	dprintf("%s% 6s : ", "request_uri", " ");
	dumpString(fieldPtr);
	dprintf("\n");

	printRequestTimeTaken();

	struct Track_Var *bucket = (struct Track_Var *)getHttpGlobal(TRACK_VARS_SERVER, "HTTP_X_ARR_LOG_ID");
	if (bucket)
	{
		printStrHttpGlobal(bucket);
		free(bucket->key);
		free(bucket);
	}

	EXIT_API();
	return S_OK;
}

extern "C"
HRESULT CALLBACK
dcri(PDEBUG_CLIENT4 Client, PCSTR args)
{
	return dumpCurrentRequestInfo(Client, args);
}

extern "C"
HRESULT CALLBACK
dumpPhpCoreGlobals(PDEBUG_CLIENT4 Client, PCSTR args)
{
	ULONG64 Address;
	ULONG64 fieldPtr;
	ULONG64 ptr;
	ULONG_PTR ptrGloblas;

	INIT_API();

	UNREFERENCED_PARAMETER(args);

	Address = GetExpression("php_cgi!_imp__core_globals");
	DBG_TRACE("php_cgi!_imp__core_globals : %d", Address);

	if (!ReadPointer(Address, &ptr))
	{
		dprintf("Error in reading _php_core_globals at %p\n",
			Address);
		return S_FALSE;
	}


	ptrGloblas = (ULONG_PTR)ptr;
	
	//output_buffering
	if (GetFieldValue(ptrGloblas, "php_cgi!_php_core_globals", "output_buffering", fieldPtr)) {
		dprintf("Error in reading output_buffering at %p\n",
			Address);
		return S_FALSE;
	}

	DBG_TRACE("output_buffering : %d", fieldPtr);

	dprintf("%s% 7s : ", "output_buffering", " ");
	dprintf("%d", fieldPtr);
	dprintf("\n");

	//memory_limit
	if (GetFieldValue(ptrGloblas, "php_cgi!_php_core_globals", "memory_limit", fieldPtr)) {
		dprintf("Error in reading memory_limit at %p\n",
			Address);
		return S_FALSE;
	}

	DBG_TRACE("memory_limit : %d", fieldPtr);

	dprintf("%s% 11s : ", "memory_limit", " ");
	dprintf("%d", fieldPtr);
	dprintf("\n");

	//memory_limit
	if (GetFieldValue(ptrGloblas, "php_cgi!_php_core_globals", "max_input_time", fieldPtr)) {
		dprintf("Error in reading max_input_time at %p\n",
			Address);
		return S_FALSE;
	}

	DBG_TRACE("max_input_time : %d", fieldPtr);

	dprintf("%s% 9s : ", "max_input_time", " ");
	dprintf("%d", fieldPtr);
	dprintf("\n");

	//track_errors
	if (GetFieldValue(ptrGloblas, "php_cgi!_php_core_globals", "track_errors", fieldPtr)) {
		dprintf("Error in reading track_errors at %p\n",
			Address);
		return S_FALSE;
	}

	DBG_TRACE("track_errors : %d", fieldPtr);

	dprintf("%s% 11s : ", "track_errors", " ");
	dprintf("%d", fieldPtr);
	dprintf("\n");

	//display_errors
	if (GetFieldValue(ptrGloblas, "php_cgi!_php_core_globals", "display_errors", fieldPtr)) {
		dprintf("Error in reading display_errors at %p\n",
			Address);
		return S_FALSE;
	}

	DBG_TRACE("display_errors : %d", fieldPtr);

	dprintf("%s% 9s : ", "display_errors", " ");
	dprintf("%d", fieldPtr);
	dprintf("\n");

	//display_startup_errors
	if (GetFieldValue(ptrGloblas, "php_cgi!_php_core_globals", "display_startup_errors", fieldPtr)) {
		dprintf("Error in reading display_startup_errors at %p\n",
			Address);
		return S_FALSE;
	}

	DBG_TRACE("display_startup_errors : %d", fieldPtr);

	dprintf("%s% s : ", "display_startup_errors", " ");
	dprintf("%d", fieldPtr);
	dprintf("\n");

	//display_startup_errors
	if (GetFieldValue(ptrGloblas, "php_cgi!_php_core_globals", "display_startup_errors", fieldPtr)) {
		dprintf("Error in reading display_startup_errors at %p\n",
			Address);
		return S_FALSE;
	}

	DBG_TRACE("display_startup_errors : %d", fieldPtr);

	dprintf("%s% s : ", "display_startup_errors", " ");
	dprintf("%d", fieldPtr);
	dprintf("\n");


	//log_errors
	if (GetFieldValue(ptrGloblas, "php_cgi!_php_core_globals", "log_errors", fieldPtr)) {
		dprintf("Error in reading log_errors at %p\n",
			Address);
		return S_FALSE;
	}

	DBG_TRACE("log_errors : %d", fieldPtr);

	dprintf("%s% 13s : ", "log_errors", " ");
	dprintf("%d", fieldPtr);
	dprintf("\n");


	//log_errors_max_len
	if (GetFieldValue(ptrGloblas, "php_cgi!_php_core_globals", "log_errors_max_len", fieldPtr)) {
		dprintf("Error in reading log_errors_max_len at %p\n",
			Address);
		return S_FALSE;
	}

	DBG_TRACE("log_errors_max_len : %d", fieldPtr);

	dprintf("%s% 5s : ", "log_errors_max_len", " ");
	dprintf("%d", fieldPtr);
	dprintf("\n");

	//error_log
	if (GetFieldValue(ptrGloblas, "php_cgi!_php_core_globals", "error_log", fieldPtr)) {
		dprintf("Error in reading error_log at %p\n",
			Address);
		return S_FALSE;
	}

	DBG_TRACE("error_log : %p", fieldPtr);

	dprintf("%s% 14s : ", "error_log", " ");
	dumpString(fieldPtr);
	dprintf("\n");

	//upload_tmp_dir
	if (GetFieldValue(ptrGloblas, "php_cgi!_php_core_globals", "upload_tmp_dir", fieldPtr)) {
		dprintf("Error in reading upload_tmp_dir at %p\n",
			Address);
		return S_FALSE;
	}

	DBG_TRACE("upload_tmp_dir : %p", fieldPtr);

	dprintf("%s% 9s : ", "upload_tmp_dir", " ");
	dumpString(fieldPtr);
	dprintf("\n");


	//upload_max_filesize
	if (GetFieldValue(ptrGloblas, "php_cgi!_php_core_globals", "upload_max_filesize", fieldPtr)) {
		dprintf("Error in reading upload_max_filesize at %p\n",
			Address);
		return S_FALSE;
	}

	DBG_TRACE("upload_max_filesize : %d", fieldPtr);

	dprintf("%s% 4s : ", "upload_max_filesize", " ");
	dprintf("%d", fieldPtr);
	dprintf("\n");

	//file_uploads
	if (GetFieldValue(ptrGloblas, "php_cgi!_php_core_globals", "file_uploads", fieldPtr)) {
		dprintf("Error in reading file_uploads at %p\n",
			Address);
		return S_FALSE;
	}

	DBG_TRACE("file_uploads : %d", fieldPtr);

	dprintf("%s% 11s : ", "file_uploads", " ");
	dprintf("%d", fieldPtr);
	dprintf("\n");


	//variables_order
	if (GetFieldValue(ptrGloblas, "php_cgi!_php_core_globals", "variables_order", fieldPtr)) {
		dprintf("Error in reading variables_order at %p\n",
			Address);
		return S_FALSE;
	}

	DBG_TRACE("variables_order : %p", fieldPtr);

	dprintf("%s% 8s : ", "variables_order", " ");
	dumpString(fieldPtr);
	dprintf("\n");

	//last_error_type
	if (GetFieldValue(ptrGloblas, "php_cgi!_php_core_globals", "last_error_type", fieldPtr)) {
		dprintf("Error in reading last_error_type at %p\n",
			Address);
		return S_FALSE;
	}

	DBG_TRACE("last_error_type : %d", fieldPtr);

	dprintf("%s% 8s : ", "last_error_type", " ");

	char* typeStr;
	switch ((ULONG)fieldPtr) {
		#define X(uc, lc) case uc: typeStr = #lc; break;
			PHP_ERROR_TYPE_MAP(X)
		#undef X
			default: typeStr = "<unknown>";
	}

	dprintf("%s\n", typeStr);

	//last_error_message
	if (GetFieldValue(ptrGloblas, "php_cgi!_php_core_globals", "last_error_message", fieldPtr)) {
		dprintf("Error in reading last_error_message at %p\n",
			Address);
		return S_FALSE;
	}

	DBG_TRACE("last_error_message : %p", fieldPtr);

	dprintf("%s% 5s : ", "last_error_message", " ");
	dumpString(fieldPtr);
	dprintf("\n");

	//last_error_file
	if (GetFieldValue(ptrGloblas, "php_cgi!_php_core_globals", "last_error_file", fieldPtr)) {
		dprintf("Error in reading last_error_file at %p\n",
			Address);
		return S_FALSE;
	}

	DBG_TRACE("last_error_file : %p", fieldPtr);

	dprintf("%s% 8s : ", "last_error_file", " ");
	dumpString(fieldPtr);
	dprintf("\n");

	//last_error_lineno
	if (GetFieldValue(ptrGloblas, "php_cgi!_php_core_globals", "last_error_lineno", fieldPtr)) {
		dprintf("Error in reading last_error_lineno at %p\n",
			Address);
		return S_FALSE;
	}


	DBG_TRACE("last_error_lineno : %d", fieldPtr);

	dprintf("%s% 6s : ", "last_error_lineno", " ");
	dprintf("%d", fieldPtr);
	dprintf("\n");

	EXIT_API();
	return S_OK;
}

extern "C"
HRESULT CALLBACK
dpcg(PDEBUG_CLIENT4 Client, PCSTR args)
{
	return dumpPhpCoreGlobals(Client, args);
}


//
//Dump Cookies
//
extern "C"
HRESULT CALLBACK dumpCookies(PDEBUG_CLIENT4 Client, PCSTR args)
{

	UNREFERENCED_PARAMETER(args);

	return printHttpGlobals(TRACK_VARS_COOKIE);

}


extern "C"
HRESULT CALLBACK dpck(PDEBUG_CLIENT4 Client, PCSTR args)
{

	return dumpCookies(Client, args);

}

//
//Specified HTTP Global
//
extern "C"
HRESULT CALLBACK dumpHttpGlobal(PDEBUG_CLIENT4 Client, PCSTR args)
{
	ULONG64 track;
	PCSTR var;
	ULONG_PTR bucket;

	DBG_TRACE("args %s",args);

	SKIP_WSPACE(args);
	var = args;
	
	//DBG_TRACE("Variable Name:%s", var);
	//while (*var !=  ' ')
	//{
	//	DBG_TRACE("Variable Name:%c", *var);
	//	DBG_TRACE("Variable Name:%s", var);
	//	var++;
	//}
	//
	//SKIP_WSPACE(var);
	//DBG_TRACE("XXX Variable Name:%s", var);

	if (GetExpressionEx(args, &track, &args)) {
		DBG_TRACE("Track ID: %d");
		if (args)
		{
			SKIP_WSPACE(args);
			DBG_TRACE("track ID: %d", track);
			DBG_TRACE("Variable Name: %s", args);

			bucket = getHttpGlobal((ULONG)track, args);
			if (bucket)
			{
				printHttpGlobal((struct Track_Var *)bucket);
				free(((struct Track_Var *)bucket)->key);
				free((void*)bucket);
			}
			else
				dprintf("%s not found\n", args);
			return S_OK;
		}
	}

	dprintf("Usage: !dumpHttpGlobal track servervariable\n");

	//DBG_TRACE("fINISHED Variable Name : %s", var);
	//DBG_TRACE("fINISHED Variable Name : %s", args);

	return S_FALSE;
}


extern "C"
HRESULT CALLBACK dphg(PDEBUG_CLIENT4 Client, PCSTR args)
{

	return dumpHttpGlobal(Client, args);

}


//
//Server variables
//
extern "C"
HRESULT CALLBACK
dumpServerVariables(PDEBUG_CLIENT4 Client, PCSTR args)
{

	UNREFERENCED_PARAMETER(args);

	return printHttpGlobals(TRACK_VARS_SERVER);

}

extern "C"
HRESULT CALLBACK
dpsv(PDEBUG_CLIENT4 Client, PCSTR args)
{

	return dumpServerVariables(Client, args);

}

//
//Environment variables
//
extern "C"
HRESULT CALLBACK
dumpEnvVariables(PDEBUG_CLIENT4 Client, PCSTR args)
{

	UNREFERENCED_PARAMETER(args);

	return printHttpGlobals(TRACK_VARS_ENV);

}

extern "C"
HRESULT CALLBACK
dpev(PDEBUG_CLIENT4 Client, PCSTR args)
{

	return dumpEnvVariables(Client, args);

}



/*
0:000> dt php_cgi!_php_core_globals http_globals 6bc5a260
+ 0x0b8 http_globals : [6] 0x014fc4b8 _zval_struct
*/
extern "C"
HRESULT CALLBACK
dumpHTTPGlobals(PDEBUG_CLIENT4 Client, PCSTR args)
{
	ULONG_PTR track;

	//TrackID
	track = (ULONG_PTR)GetExpression(args);
	DBG_TRACE("http_globals track ID : %d", track);

	if (track > TRACK_VARS_FILES)
	{
		dprintf("track ID out of index, valid track ID:\n");
		dprintf("TRACK_VARS_POST		0\n"
			"TRACK_VARS_GET		1\n"
			"TRACK_VARS_COOKIE	2\n"
			"TRACK_VARS_SERVER	3\n"
			"TRACK_VARS_ENV		4\n"
			"TRACK_VARS_FILES	5\n"
			"//TRACK_VARS_REQUEST	6(only 6 in total actually)\n"
			);

		return S_FALSE;
	}

	return printHttpGlobals(track);

}

extern "C"
HRESULT CALLBACK
dphgs(PDEBUG_CLIENT4 Client, PCSTR args)
{
	return dumpHTTPGlobals(Client, args);
}


/*
  This gets called (by DebugExtensionNotify whentarget is halted and is accessible
*/
HRESULT
NotifyOnTargetAccessible(PDEBUG_CONTROL Control)
{
    /*dprintf("Extension dll detected a break");
    if (Connected) {
        dprintf(" connected to ");
        switch (TargetMachine) {
        case IMAGE_FILE_MACHINE_I386:
            dprintf("X86");
            break;
        case IMAGE_FILE_MACHINE_IA64:
            dprintf("IA64");
            break;
        default:
            dprintf("Other");
            break;
        }
    }*/
    //dprintf("\n");

    //
    // show the top frame and execute dv to dump the locals here and return
    //
    //Control->Execute(DEBUG_OUTCTL_ALL_CLIENTS |
    //                 DEBUG_OUTCTL_OVERRIDE_MASK |
    //                 DEBUG_OUTCTL_NOT_LOGGED,
    //                 ".frame", // Command to be executed
    //                 DEBUG_EXECUTE_DEFAULT );
    //Control->Execute(DEBUG_OUTCTL_ALL_CLIENTS |
    //                 DEBUG_OUTCTL_OVERRIDE_MASK |
    //                 DEBUG_OUTCTL_NOT_LOGGED,
    //                 "dv", // Command to be executed
    //                 DEBUG_EXECUTE_DEFAULT );
    return S_OK;
}


//
//0:000>  dt 6fc263a0  _sapi_globals_struct
//php_cgi!_sapi_globals_struct
//+ 0x0d0 global_request_time : 1385026387.6237421
//

extern "C"
HRESULT CALLBACK
test(PDEBUG_CLIENT4 Client, PCSTR args)
{
	INIT_API();
	UNREFERENCED_PARAMETER(args);

	ULONG64 ptrGloblas = 0x6fc263a0;
	double rqTime;

	//last_error_message
	if (GetFieldValue(ptrGloblas, "php_cgi!_sapi_globals_struct", "global_request_time", rqTime)) {
		dprintf("Error in reading global_request_time at %p\n",
			ptrGloblas);
		return S_FALSE;
	}

	dprintf("global_request_time: %f(%f)(%x)", (float)rqTime, rqTime, (ULONG)rqTime);
	return S_OK;

}
/*
  A built-in help for the extension dll
*/
extern "C"
HRESULT CALLBACK
help(PDEBUG_CLIENT4 Client, PCSTR args)
{
    INIT_API();

    UNREFERENCED_PARAMETER(args);

    dprintf("Help for phpexts.dll\n"
            "  help                   - Shows this help\n"
            "  dumpPhpCallStack       - This dumps current running php call stacks\n"
			"  dpcs                   - Alias for dumpPhpCallStack\n"
			"  isPHPPageRunning       - This check is there a PHP page running\n"
			"  ispr                   - Alias for isPHPPageRunning\n"
			"  dumpCurrentRequestUrl  - This dumps current request URI\n"
			"  dcru                   - Alias for dumpCurrentRequestUrl \n"
			"  dumpCurrentRequestInfo - This dumps current request information\n"
			"  dcri                   - Alias for dumpCurrentRequestInfo \n"
			"  dumpPhpCoreGlobals     - This dumps the PHP core global variables\n"
			"  dpcg                   - Alias for dumpPhpCoreGlobals\n"
			"  dumpHttpGlobals        - This dumps the PHP HTTP Global variables like environment variables, server variables, etc\n"
			"  dphgs                  - Alias for dumpHTTPGlobals\n"
			"  dumpHttpGlobal         - This dumps the specified PHP HTTP Global variables like environment variables, server variables, etc\n"
			"  dphg                   - Alias for dumpHTTPGlobal\n"
			"  dumpEnvVariables       - This dumps the Environment variables\n"
			"  dpev                   - Alias for dumpEnvVariables\n"
			"  dumpServerVariables    - This dumps the server variables\n"
			"  dpsv                   - Alias for dumpServerVariables\n"
			"  dumpCookies            - This dumps the cookies\n"
			"  dpck                   - Alias for dumpCookies\n"
            );

	EXIT_API();

    return S_OK;
}

