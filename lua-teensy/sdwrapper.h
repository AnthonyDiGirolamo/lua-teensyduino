#ifndef SDWRAPPER_H
#define SDWRAPPER_H

#include "SdFat.h"

#include "Arduino.h"

#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

int sd_ls(lua_State *L);
int sd_cat(lua_State *L);
int sd_run(lua_State *L);

static const luaL_Reg sd_functions[] = {
  {"ls", sd_ls},
  {"cat", sd_cat},
  {"run", sd_run},
  {NULL, NULL}
};

#endif
