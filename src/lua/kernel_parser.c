#include "kernel_parser.h"
#include <string.h>
#include <stdlib.h>

// Create the Lua state and load the Lua script
lua_State* createLuaState()
{
  lua_State *L = lua_open();
  luaL_openlibs(L);

  if (luaL_loadfile(L, CONFIG_FILE) || lua_pcall(L, 0, 0, 0))
    {
      error(L, "cannot run configuration file: %s",
	    lua_tostring(L, -1));
      L = NULL;
    }

  return L;
}


// Open the kernel file
void loadKernelFile(lua_State *L, const char *fileName)
{
  lua_getglobal(L, "loadFile");
  lua_pushstring(L, fileName);
  
  int numArgs = 1;
  int numResults = 0;
      
  if (lua_pcall(L, numArgs, numResults, 0) != 0)
    {
      error(L, "error running function `f': %s",
	    lua_tostring(L, -1));
      return;
    }

}


// Return the next kernel information
void loadNextKernel(lua_State *L, char **kernelName, int *numKernelArgs)
{      
  if(L == NULL)
    {
      printf("lua_State is null\n");
      return;
    }
  
  lua_getglobal(L, "getNextKernel");
  
  int numInput = 0;
  int numResults = 2;
  
  if (lua_pcall(L, numInput, numResults, 0) != 0)
    {
      error(L, "error running function `f': %s",
	    lua_tostring(L, -1));
      (*kernelName) = NULL;
      (*numKernelArgs) = -1;
      return;
    }

  const char *funcName = lua_tostring(L, -2);
  size_t length = lua_strlen(L, -2);
  int numArgs = lua_tonumber(L, -1);
  
  if(funcName == NULL || strcmp(funcName, "(nil)") == 0)
    {
      (*kernelName) = NULL;
      (*numKernelArgs) = -1;
    }
  else
    {
      (*kernelName) = malloc(length);
      strncpy(*kernelName, funcName, length);
      *numKernelArgs = numArgs;
    }
  
}



