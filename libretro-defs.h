#include "libretro.h"

struct libretro_handle;
struct libretro_handle *libretro_vfs_open(const char *filename, const char *mode);
void libretro_vfs_close(struct libretro_handle *h);
int libretro_vfs_getc(struct libretro_handle *h);
void libretro_vfs_putc(int c, struct libretro_handle *h);
void libretro_vfs_flush(struct libretro_handle *h);
int64_t libretro_vfs_get_size(struct libretro_handle *h);
void libretro_vfs_read(struct libretro_handle *h, void *s, uint64_t len);
void scr_mark_dirty (void);
void tty_set_keymap (void);
void tty_poll();
char* strtoupper(char* str);

