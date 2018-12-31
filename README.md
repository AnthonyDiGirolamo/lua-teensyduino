# Lua Example on Teensy 3.6

This is Lua 5.1.5 with the following patches:

1. [Lua Tiny RAM (LTR)](http://lua-users.org/lists/lua-l/2008-11/msg00331.html)

   This is used in eLua and nodemcu

1. [Lua Compact Debug (LCD)](https://nodemcu.readthedocs.io/en/master/en/lcd/)

   Original patch can be found here: https://github.com/TerryE/lua-5.1/commits/master

1. Changes to support printing to Serial and file loading from [SdFat](https://github.com/greiman/SdFat).

## Running

Compile in the Arduino IDE or change `ARDUINO_DIR` in `lua-teensy/Makefile` to
point to where the arduino ide is installed on your linux system.

```sh
make && make upload
sleep 2;
miniterm.py /dev/ttyACM0 115200
```

## Example output

```
--- Miniterm on /dev/ttyACM0  115200,8,N,1 ---
--- Quit: Ctrl+] | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H ---
SD Card Init ...
SD Card Init Complete

=== Help =========================

SD Card functions:
  sd.ls()
  sd.cat("ram.lua")
  sd.run()

RAM Usage:
  print(collectgarbage'count')

Force garbage collection:
  collectgarbage'collect'

Use EOT (Ctrl-D in miniterm.py) to exec code


>> print(math.sqrt(2))␄
1.4142135381699

>> print(collectgarbage'count')␄
5.8896484375

>>
```

## Helpful Lua C API Links

- https://www.lua.org/pil/contents.html
- https://github.com/tylerneylon/APIsWithLua
