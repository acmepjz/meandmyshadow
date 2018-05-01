#include "ScriptUserData.h"
#include <stdio.h>

//Some debug functions
void scriptUserClassDebugCreate(char sig1,char sig2,char sig3,char sig4,const void* p1,const void* p2) {
#ifdef _DEBUG
	printf("ScriptUserClass '%c%c%c%c' (%p) created userdata: %p\n",
		sig1,sig2,sig3,sig4,p1,p2);
#endif
}

void scriptUserClassDebugInvalidate(char sig1,char sig2,char sig3,char sig4,const void* p1,const void* p2) {
#ifdef _DEBUG
	printf("ScriptUserClass '%c%c%c%c' (%p) invalidated userdata: %p\n",
		sig1,sig2,sig3,sig4,p1,p2);
#endif
}

void scriptUserClassDebugUnlink(char sig1,char sig2,char sig3,char sig4,const void* p1,const void* p2) {
#ifdef _DEBUG
	printf("ScriptUserClass '%c%c%c%c' (%p) unlinked userdata: %p\n",
		sig1,sig2,sig3,sig4,p1,p2);
#endif
}

