// Kernel Parser
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>


#define CONFIG_FILE "../src/lua/kernel_parser.lua" 

lua_State* createLuaState();

void loadKernelFile(lua_State *L, const char *fileName);
void loadNextKernel(lua_State *L, char **kernelName, int *numKernelArgs);


