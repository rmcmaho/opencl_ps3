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
  int numResults = 3;
  
  if (lua_pcall(L, numInput, numResults, 0) != 0)
    {
      error(L, "error running function `f': %s",
	    lua_tostring(L, -1));
      (*kernelName) = NULL;
      (*numKernelArgs) = -1;
      return;
    }

  const char *funcName = lua_tostring(L, -3);
  size_t length = lua_strlen(L, -3);
  int numArgs = lua_tonumber(L, -2);

  if(funcName == NULL || strcmp(funcName, "(nil)") == 0)
    {
      (*kernelName) = NULL;
      (*numKernelArgs) = -1;
      return;
    }
  else
    {
      (*kernelName) = malloc(length);
      strncpy(*kernelName, funcName, length);
      *numKernelArgs = numArgs;
    }

  //Convert Lua table into array
  int i;
  char **argTypes = calloc(numArgs, sizeof(char *));
  for (i=1; i<=numArgs; i++)
    {
      lua_rawgeti(L, -1, i);
      const char *type = lua_tostring(L, -1);
      size_t len = lua_strlen(L, -1);
      lua_pop(L, 1);
      argTypes[i-1] = calloc(len, sizeof(char));
      strncpy(argTypes[i-1], type, len);
    }  
}



