#include "sandbox.h"
#include "platforms.h"
#include "apps.h"

//on linux we can tell the kernel not to allow switch-user/super-user for this application
int SetNoSU()
{
#ifdef HAVE_PRCTL
#include <sys/prctl.h>
#ifdef PR_SET_NO_NEW_PRIVS

//set, then check that the set worked. This correctly handles situations where we ask to set more than once
//as the second attempt may 'fail', but we already have the desired result
    prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
    if (prctl(PR_GET_NO_NEW_PRIVS, 0, 0, 0, 0) == 1)
    {
        TerminalPutStr("~gNO_NEW_PRIVS set for process~0 (security feature that prevents su/sudo)\n", NULL);
        return(TRUE);
    }

    //if we get here then something went wrong
    RaiseError(ERRFLAG_ERRNO, "SetNoSU", "Failed to set 'no new privs', even though this seems to be supported");
#endif

#endif

    return(FALSE);
}



//select a 'level' for the seccomp sandbox this will usually be one of 'minimal', 'user'
char *SeccompSandboxGetLevel(char *RetStr, TAction *Act)
{
    char *Platform=NULL;
    const char *ptr="";

    Platform=CopyStr(Platform, GetVar(Act->Vars, "platform"));
    if ( NativeBitWidth() == PlatformBitWidth(Platform) )
    {
        ptr=GetVar(Act->Vars, "security_level");
				if (! StrValid(ptr))
        {
            //if either the app or the config has 'allow su' set, then we cannot use seccomp
            //applications that have a google-chrome/chromium component need capset and bpf for their sandboxing
            if (! AppAllowSU(Act)) ptr="syscall_allow=capset;bpf user";
        }
				else if (strcasecmp(ptr, "none")==0) ptr="";
    }

    RetStr=CopyStr(RetStr, ptr);

    Destroy(Platform);

    return(RetStr);
}
