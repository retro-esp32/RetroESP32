#pragma once

#include "esp_err.h"

int sdcard_files_get(const char *path, const char *extension, char ***filesOut);
void sdcard_files_free(char **files, int count);
esp_err_t sdcard_open();
esp_err_t sdcard_close();
size_t sdcard_get_filesize(const char *path);
size_t sdcard_copy_file_to_memory(const char *path, void *ptr);
char *sdcard_create_savefile_path(const char *base_path, const char *fileName);