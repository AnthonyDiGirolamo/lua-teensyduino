// Allow printing (eg with Serial) using the stream operator
template<class T> inline Print& operator <<(Print &obj,     T arg) { obj.print(arg);    return obj; }
template<>        inline Print& operator <<(Print &obj, float arg) { obj.print(arg, 4); return obj; }

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

#include <SPI.h>
#include "SdFat.h"
#include "sdwrapper.h"

#ifdef TEENSYDUINO
#include <TimeLib.h>

time_t getTeensy3Time() {
  return Teensy3Clock.get();
}
#endif

// Compatibility functions for Serial output and SdFat file operations
extern "C" {
#ifdef TEENSYDUINO
  SdFatSdioEX sd;
#else
  SdFat sd;
#endif

  StdioStream lua_temp_stdio_file;

  int lua_compat_fopen(const char *filename) {
    // Serial << "-- sd fopen: " << filename << "\n";
    return (lua_temp_stdio_file.fopen(filename, "r") ? 1 : 0);
  }

  void lua_compat_fclose() {
    // Serial << "-- sd fclose\n";
    lua_temp_stdio_file.fclose();
  }

  char lua_compat_getc() {
    // Serial << "-- sd getc\n";
    return lua_temp_stdio_file.getc();
  }

  int lua_compat_feof() {
    // Serial << "-- sd feof\n";
    return lua_temp_stdio_file.feof();
  }

  size_t lua_compat_fread(void* ptr, size_t size, size_t count) {
    // Serial << "-- sd fread -- size: " << size << " count: " << count << "\n";
    return lua_temp_stdio_file.fread(ptr, size, count);
  }

  int lua_compat_ferror() {
    // Serial << "-- sd ferror\n";
    return lua_temp_stdio_file.ferror();
  }

  void lua_compat_printf(char *fmt, ... ){
    Serial.print("SPF:");
    char buf[1024];
    va_list args;
    va_start (args, fmt );
    vsnprintf(buf, 128, fmt, args);
    va_end (args);
    Serial.print(buf);
  }

  void lua_compat_print(const char *s){
    Serial.print(s);
  }

}

const char help_function[] = "function help ()\n"
"  print('=== Help =========================')\n"
"  print()\n"
"  print('SD Card functions:')\n"
"  print('  sd.ls()')\n"
"  print('  sd.cat(\"ram.lua\")')\n"
"  print('  sd.run()')\n"
"  print()\n"
"  print('RAM Usage: ')\n"
"  print(\"  print(collectgarbage'count')\")\n"
"  print()\n"
"  print('Force garbage collection: ')\n"
"  print(\"  collectgarbage'collect'\")\n"
"  print()\n"
"  print('Use EOT (Ctrl-D in miniterm.py) to exec code')\n"
"  print()\n"
"end\n";


// This doesn't work for lua tiny ram rotables
const char dir_function[] = "function dir (prefix, a)\n"
"  for i,v in pairs (a) do\n"
"    if i ~= 'loaded' and i ~= '_G' then\n"
"      if type(v) == 'table' then\n"
"        dir(prefix .. '.' .. i,v)\n"
"      elseif type(v) == 'function' then\n"
"        print (prefix .. '.' .. i .. '()')\n"
"      end\n"
"    end\n"
"  end\n"
"end\n";

const char v_function[] = "function v (a) for i,v in pairs (a) do print(i, v) end end";

// fr() = free ram
// gc = garbage collect
const char ram_functions[] = "function fr ()\n"
"  print('ram: '..collectgarbage'count')\n"
"end\n"
"fr()\n"
"function gc ()\n"
"  print('collectgarbage...')\n"
"  collectgarbage'collect'\n"
"  fr()\n"
"end\n"
"fr()\n"
"t={}\n"
"for i=1,10 do\n"
"  table.insert(t, math.pow(2,i))\n"
"end\n"
"for i,v in ipairs(t) do\n"
"  print(i, v)\n"
"end\n"
"fr()\n"
"gc()\n";


lua_State *L;
char output[1024];

void load_lua_string(const char lbuff[]) {
  int error = 0;

  // Serial.println("luaL_loadstring()");
  error = luaL_loadstring(L, lbuff);
  // Serial.println(error);
  if (error) {
    Serial << "Error Loading string: " << lbuff << "\n";
    sprintf(output, "%s", lua_tostring(L, -1));
    Serial.println(output);
    // Serial.println("lua_pop()");
    lua_pop(L, 1); /* pop error message from the stack */
  }
  else {
    // Serial.println("lua_pcall()");
    error = lua_pcall(L, 0, 0, 0);
    if (error) {
      Serial << "Error running:\n";
      sprintf(output, "%s", lua_tostring(L, -1));
      Serial.println(output);
      // Serial.println("lua_pop()");
      lua_pop(L, 1); /* pop error message from the stack */
    }
  }
}

void populate_sd_card() {
  SdFile file;
  if (!file.open("ram.lua", O_CREAT | O_WRITE)) {
    Serial.println("create ram.lua failed");
  }
  file.write(ram_functions);
  file.close();
}

void setup(void) {
  // Enable printf/sprintf to print floats
  asm(".global _printf_float");
  Serial.begin(115200);
  delay(2000);

#ifdef TEENSYDUINO
  setSyncProvider(getTeensy3Time);
  Serial.println("SD Card Init ... ");
  // SdFat Setup
  if (!sd.begin()) {
    Serial.println("SdFatSdioEX begin() failed");
    sd.initErrorHalt("SdFatSdioEX begin() failed");
  }
  sd.chvol(); // make sd the current volume.
  Serial.println("SD Card Init Complete\n");
#endif

  // Create ram.lua on the sd card
  populate_sd_card();

  char repl_buffer[1024];
  char new_char;
  uint16_t buff_position = 0;

  // Serial.println("luaL_newstate()");
  L = luaL_newstate(); /* opens Lua */
  // Serial.println("luaL_openlibs()");
  luaL_openlibs(L); /* opens the standard libraries */

  // Serial << "luaL_register_light sd_functions\n";
  luaL_register_light(L, "sd", sd_functions);

  // Load global help() function
  load_lua_string(help_function);

  // Clear the stack
  lua_settop(L, 0);
  // Exec help() lua function
  lua_getglobal(L, "help");
  lua_call(L, 0, 0);  // 0, 0 = #args, #retvals

  // prompt
  Serial << "\n>> ";
  while (1) {
    if (Serial.available() > 0) {
      new_char = Serial.read();
      if ((new_char >= 32 && new_char <= 127) // printable character
          || new_char == '\n' // line break
          || new_char == 4    // EOT end of transmission
          || new_char == 8    // backspace
          || new_char == 9    // tab
          ) {
        repl_buffer[buff_position] = new_char;
        // echo new char
        Serial.write(new_char);
        buff_position++;
      }
      // TODO handle escape sequences 27 (and arrow keys)
    }

    // if no characters received skip the rest of the loop
    if (buff_position == 0)
      continue;

    // if backspace was pressed
    if (repl_buffer[buff_position-1] == 8) {
      if (buff_position == 1)
        // just remove the backspace character
        buff_position--;
      else
        // remove both the backspace character and the previously entered character
        buff_position = buff_position - 2;
    }
    // if EOT end of transmission == 4 (Ctrl-D in miniterm.py)
    else if (repl_buffer[buff_position-1] == 4) {
      // set the last character to the null char (should overwrite the EOT)
      repl_buffer[buff_position-1] = '\0';
      // Serial << "\nGot: '" << repl_buffer << "'";
      Serial << '\n';

      load_lua_string(repl_buffer);

      // reset buffer index
      buff_position = 0;
      // erase repl_buffer?
      Serial << "\n>> ";
    }
  }

  // destroy lua vm
  // Serial.println("lua_close()");
  // lua_close(L);
}

void loop() {
}
