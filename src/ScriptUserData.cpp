#include "ScriptUserData.h"
#include <stdio.h>

//Some debug functions
void scriptUserClassDebugCreate(char sig1,char sig2,char sig3,char sig4,const void* p1,const void* p2) {
#if defined(DISABLED_DEBUG_STUFF)
	printf("ScriptUserClass '%c%c%c%c' (%p) created userdata: %p\n",
		sig1,sig2,sig3,sig4,p1,p2);
#endif
}

void scriptUserClassDebugInvalidate(char sig1,char sig2,char sig3,char sig4,const void* p1,const void* p2) {
#if defined(DISABLED_DEBUG_STUFF)
	printf("ScriptUserClass '%c%c%c%c' (%p) invalidated userdata: %p\n",
		sig1,sig2,sig3,sig4,p1,p2);
#endif
}

void scriptUserClassDebugUnlink(char sig1,char sig2,char sig3,char sig4,const void* p1,const void* p2) {
#if defined(DISABLED_DEBUG_STUFF)
	printf("ScriptUserClass '%c%c%c%c' (%p) unlinked userdata: %p\n",
		sig1,sig2,sig3,sig4,p1,p2);
#endif
}

void scriptProxyUserClassCreateUserDataFailed(char sig1, char sig2, char sig3, char sig4, const void* pThis, const void* pActive) {
	fprintf(stderr, "ERROR: ScriptProxyUserClass '%c%c%c%c' (%p) refused to create userdata because the active object is %p which is not equal to this object!\n"
		"Maybe (1) there is a bug in code or (2) the newly created object get deleted immediately by script\n",
		sig1, sig2, sig3, sig4, pThis, pActive);
}
