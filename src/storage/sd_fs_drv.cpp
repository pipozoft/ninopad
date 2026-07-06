#include "sd_fs_drv.h"
#include <lvgl.h>
#include <SD.h>

static bool fs_ready_cb(lv_fs_drv_t *drv)
{
    (void)drv;
    return SD.cardSize() > 0;
}

static void *fs_open_cb(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode)
{
    (void)drv;
    const char *flags = FILE_READ;
    if ((mode & LV_FS_MODE_WR) && (mode & LV_FS_MODE_RD))
        flags = "r+";
    else if (mode & LV_FS_MODE_WR)
        flags = FILE_WRITE;

    File *f = new File();
    *f = SD.open(path, flags);
    if (!*f) {
        delete f;
        return NULL;
    }
    return (void *)f;
}

static lv_fs_res_t fs_close_cb(lv_fs_drv_t *drv, void *file_p)
{
    (void)drv;
    File *f = (File *)file_p;
    f->close();
    delete f;
    return LV_FS_RES_OK;
}

static lv_fs_res_t fs_read_cb(lv_fs_drv_t *drv, void *file_p, void *buf, uint32_t btr, uint32_t *br)
{
    (void)drv;
    File *f = (File *)file_p;
    int n = f->read((uint8_t *)buf, btr);
    if (br) *br = n > 0 ? (uint32_t)n : 0;
    return n >= 0 ? LV_FS_RES_OK : LV_FS_RES_UNKNOWN;
}

static lv_fs_res_t fs_write_cb(lv_fs_drv_t *drv, void *file_p, const void *buf, uint32_t btw, uint32_t *bw)
{
    (void)drv;
    File *f = (File *)file_p;
    int n = f->write((const uint8_t *)buf, btw);
    if (bw) *bw = n > 0 ? (uint32_t)n : 0;
    return n >= 0 ? LV_FS_RES_OK : LV_FS_RES_UNKNOWN;
}

static lv_fs_res_t fs_seek_cb(lv_fs_drv_t *drv, void *file_p, uint32_t pos, lv_fs_whence_t whence)
{
    (void)drv;
    File *f = (File *)file_p;
    uint32_t target;

    switch (whence) {
    case LV_FS_SEEK_SET:
        target = pos;
        break;
    case LV_FS_SEEK_CUR:
        target = f->position() + pos;
        break;
    case LV_FS_SEEK_END:
        target = f->size() + pos;
        break;
    default:
        return LV_FS_RES_INV_PARAM;
    }

    return f->seek(target) ? LV_FS_RES_OK : LV_FS_RES_UNKNOWN;
}

static lv_fs_res_t fs_tell_cb(lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p)
{
    (void)drv;
    File *f = (File *)file_p;
    if (pos_p) *pos_p = f->position();
    return LV_FS_RES_OK;
}

void sd_fs_drv_register(void)
{
    static lv_fs_drv_t drv;
    lv_fs_drv_init(&drv);

    drv.letter = 'S';
    drv.ready_cb = fs_ready_cb;
    drv.open_cb = fs_open_cb;
    drv.close_cb = fs_close_cb;
    drv.read_cb = fs_read_cb;
    drv.write_cb = fs_write_cb;
    drv.seek_cb = fs_seek_cb;
    drv.tell_cb = fs_tell_cb;

    lv_fs_drv_register(&drv);
}
