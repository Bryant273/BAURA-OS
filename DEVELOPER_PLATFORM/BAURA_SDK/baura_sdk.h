
#ifndef BAURA_SDK_H
#define BAURA_SDK_H
#include "BAURA_GUI_FFI.h"
#include "BAURA_VFS_RUST.h"
#include "BAURA_MESSAGE_BUS_GO.h"
typedef int (*BauraAppEntry)();
void baura_app_init();
void baura_create_main_window(const char* title, int w, int h);
#endif
