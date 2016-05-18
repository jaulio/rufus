#pragma once


// helper methods
#define TOSTR(value) case value: return _T(#value)

#define IFFALSE_PRINTERROR(__CONDITION__, __ERRROR_MSG__) if(!(__CONDITION__)) { if(strlen(__ERRROR_MSG__)) uprintf(__ERRROR_MSG__); }
#define IFFALSE_GOTOERROR(__CONDITION__, __ERRROR_MSG__) if(!(__CONDITION__)) { if(strlen(__ERRROR_MSG__)) uprintf(__ERRROR_MSG__); goto error; }
#define IFFALSE_GOTO(__CONDITION__, __ERRROR_MSG__, __LABEL__) if(!(__CONDITION__)) { uprintf(__ERRROR_MSG__); goto __LABEL__; }
#define IFFALSE_RETURN(__CONDITION__, __ERRROR_MSG__) if(!(__CONDITION__)) { uprintf(__ERRROR_MSG__); return; }
#define IFFALSE_RETURN_VALUE(__CONDITION__, __ERRROR_MSG__, __RET__) if(!(__CONDITION__)) { uprintf(__ERRROR_MSG__); return __RET__; }
#define IFFALSE_BREAK(__CONDITION__, __ERRROR_MSG__) if(!(__CONDITION__)) { uprintf(__ERRROR_MSG__); break; }
#define IFFALSE_CONTINUE(__CONDITION__, __ERRROR_MSG__) if(!(__CONDITION__)) { uprintf(__ERRROR_MSG__); continue; }