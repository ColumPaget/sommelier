/*
 * LD_PRELOAD shim which applies two patches necesary to get the game
 * Divinity: Original Sin Enhanded Edition for Linux to work with Mesa (12+)
 *
 * Build with: gcc -s -O2 -shared -fPIC -o divos-hack.{so,c} -ldl
 */

/* for RTLD_NEXT */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#include <GL/gl.h>
#include <string.h>

#define _GLX_PUBLIC

/*
 *   https://github.com/karolherbst/mesa/commit/aad2543bf6cfbd7df795d836e5ff4ec8686e4fdf
 *     - allow env override of vendor string.  I actually just hard-coded
 *       ATI Technologies, Inc., since that appears to be what's needed
 */
const GLubyte *GLAPIENTRY glGetString( GLenum name )
{
    static void *next = NULL;
    static const char *vendor = "ATI Technologies, Inc.";
    if(name == GL_VENDOR)
	return (const GLubyte *)vendor;
    if(!next)
	next = dlsym(RTLD_NEXT, "glGetString");
    return ((const GLubyte *GLAPIENTRY (*)(GLenum))next)(name);
}

/*
 *   https://gist.github.com/karolherbst/b279233f8b13c9db1f3e1e57c6ecfbd2
 */
_GLX_PUBLIC void (*glXGetProcAddressARB(const GLubyte * procName)) (void)
{
   static void *next = NULL;
   if (strcmp((const char *) procName, "glNamedStringARB") == 0 ||
       strcmp((const char *) procName, "glDeleteNamedStringARB") == 0 ||
       strcmp((const char *) procName, "glCompileShaderIncludeARB") == 0 ||
       strcmp((const char *) procName, "glIsNamedStringARB") == 0 ||
       strcmp((const char *) procName, "glGetNamedStringARB") == 0 ||
       strcmp((const char *) procName, "glGetNamedStringivARB") == 0)
      return NULL;
   if(!next)
	next = dlsym(RTLD_NEXT, "glXGetProcAddressARB");
    return ((_GLX_PUBLIC void (*(*)(const GLubyte *))(void))next)(procName);
}
