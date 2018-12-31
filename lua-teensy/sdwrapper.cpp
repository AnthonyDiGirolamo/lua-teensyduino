#include "sdwrapper.h"

#ifdef TEENSYDUINO
extern SdFatSdioEX sd;
#else
extern SdFat sd;
#endif

// sd.ls()
int sd_ls(lua_State *L) {
  // sd.ls("/", LS_R);
  sd.ls(&Serial, LS_R | LS_DATE | LS_SIZE);
  return 0;  // number of lua visible return values
}

// sd.cat('filename')
int sd_cat(lua_State *L) {
  const char *filename = lua_tostring(L, 1);
  File f;
  f = sd.open(filename);

  if (!f) {
    Serial.print("Read file failed: ");
    Serial.println(filename);
    return 0;
  }

  while (f.available()) {
    Serial.write(f.read());
  }
  f.close();
  return 0;
}

// sd.run('filename')
int sd_run(lua_State *L) {
  const char *filename = lua_tostring(L, 1);
  int error = 0;
  char output[1024];

  // Serial.println("luaL_loadfile()");
  error = luaL_dofile(L, filename);
  if (error) {
    Serial.print("Error Loading file: ");
    Serial.println(filename);
    sprintf(output, "%s", lua_tostring(L, -1));
    Serial.println(output);
    // Serial.println("lua_pop()");
    lua_pop(L, 1); /* pop error message from the stack */
  }
  return 0;
}

