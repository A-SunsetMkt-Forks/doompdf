#include <ctype.h>
#include <emscripten.h>
#include <stdio.h>
#include <stdlib.h>
#include "doomgeneric.h"
#include "doomkeys.h"

uint32_t start_time;
int frame_count = 0;

uint32_t get_time() {
  return EM_ASM_INT({
    return Date.now();
  });
}

void DG_SleepMs(uint32_t ms) {}


uint32_t DG_GetTicksMs() {
  return get_time() - start_time;
}

void DG_Init() {
  start_time = get_time();
}

int DG_GetKey(int* pressed, unsigned char* doomKey) {
  int key_data = EM_ASM_INT({
    if (key_queue.length === 0) 
      return 0;
    let key_data = key_queue.shift();
    let key = key_data[0];
    let pressed = key_data[1];
    return (pressed << 8) | key;
  });

  if (key_data == 0)
    return 0;
  
  *pressed = key_data >> 8;
  *doomKey = key_data & 0xFF;
  return 1;
}

void DG_SetWindowTitle(const char * title) {}

int key_to_doomkey(int key) {
  if (key == 97) //a
    return KEY_LEFTARROW;
  if (key == 100) //d
    return KEY_RIGHTARROW;
  if (key == 119) //w
    return KEY_UPARROW;
  if (key == 115) //s
    return KEY_DOWNARROW;
  if (key == 113) //q
    return KEY_ESCAPE;
  if (key == 122) //z
    return KEY_ENTER;
  if (key == 101) //e
    return KEY_USE;
  if (key == 32) //<space>
    return KEY_FIRE;
  if (key == 109) //,
    return KEY_TAB;
  if (key == 95) //_
    return KEY_RSHIFT;
  return tolower(key);
}

void DG_DrawFrame() {
  EM_ASM({
    for (let key of Object.keys(pressed_keys)) {
      key_queue.push([key, !!pressed_keys[key]]);

      if (pressed_keys[key] === 0)
        delete pressed_keys[key];
      if (pressed_keys[key] === 2) 
        pressed_keys[key] = 0;
    }
  });

  int framebuffer_len = DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4;
  EM_ASM({
    let framebuffer_ptr = $0;
    let framebuffer_len = $1;
    let width = $2;
    let height = $3;
    let framebuffer = Module.HEAPU8.subarray(framebuffer_ptr, framebuffer_ptr + framebuffer_len);
    for (let y=0; y < height; y++) {
      let row = Array(width);
      for (let x=0; x < width; x++) {
        let index = (y * width + x) * 4;
        let r = framebuffer[index];
        let g = framebuffer[index+1];
        let b = framebuffer[index+2];
        let avg = (r + g + b) / 3;
        //let avg = (x/width) * 255; // (uncomment for a gradient test)

        //note - these ascii characters were all picked because they have the same width in the sans-serif font that chrome decided to use for text fields
        if (avg > 200)
          row[x] = "_";
        else if (avg > 150)
          row[x] = "::";
        else if (avg > 100)
          row[x] = "?";
        else if (avg > 50)
          row[x] = "//";
        else if (avg > 25)
          row[x] = "b";
        else
          row[x] = "#";
      }
      globalThis.getField("field_"+(height-y-1)).value = row.join("");
    }
  }, DG_ScreenBuffer, framebuffer_len, DOOMGENERIC_RESX, DOOMGENERIC_RESY);
}

void doomjs_tick() {
  int start = get_time();
  doomgeneric_Tick();
  int end = get_time();
  frame_count ++;

  if (frame_count % 60 == 0) {
    printf("frame time: %i ms\n", end - start);
  }
}

int main(int argc, char **argv) {
  doomgeneric_Create(argc, argv);

  EM_ASM({
    app.setInterval("_doomjs_tick()", 0);
  });
  return 0;
}