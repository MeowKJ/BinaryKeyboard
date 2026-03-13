#ifndef CH552G_KEY_SCANNER_H
#define CH552G_KEY_SCANNER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// FUNC 键激活标志 — LightingController 设置，KeyScanner 读取
extern volatile bool funcActive;

void KeyScanner_init(void);
void KeyScanner_process(void);
bool KeyScanner_isPressed(uint8_t index);

#ifdef __cplusplus
}
#endif

#endif
