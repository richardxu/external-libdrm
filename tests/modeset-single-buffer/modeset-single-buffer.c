//
// Created by RichardXu on 2019/4/20.
//

#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

struct buffer_object {
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t handle;
    uint32_t size;
    uint8_t *vaddr;
    uint32_t fb_id;
};

struct buffer_object buf;

static int modeset_create_fb(int fd, struct buffer_object *bo)
{
    struct drm_mode_create_dumb create = {};
    struct drm_mode_map_dumb map = {};

    //4，创建连续内存 dumb-buffer，pixel format is XRGB888
    create.width = bo->width;
    create.height = bo->height;
    create.bpp = 32;
    drmIoctl(fb, DRM_IOCTL_MODE_CREATE_DUMB, &create);

    //5, 将 dumb-buffer 绑定到 一个 FB 对象
    bo->pitch = create.pitch;
    bo->size = create.size;
    bo->handle = create.handle;
    drmModeAddFB(fd, bo->width, bo->height, 24, 32, bo->pitch,
                    bo->handle, &bo->fb_id);

    //6，映射 dumb-buffer 到用户空间，用以给用户空间绘图
    map.handle = create.handle;
    drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map);
    bo->vaddr = mmap(0, create.size, PORT_READ | PORT_WRITE,
                    MAP_SHARD, fd, map.offset);

    //初始化dumb-buffer 的数据为 0xff，即为 white-color
    memset(bo->vaddr, 0xff, bo->size);

    return 0;
}

static void modeset_destroy_fb(int fb, struct buffer_object *bo)
{
    struct drm_mode_destroy_dumb destroy = {};

    drmModeRmFB(fd, bo->fb_id);

    munmap(bo->vaddr, bo->size);

    destroy.handle = bo->handle;
    drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
}

int main(int argc, char **argv)
{
    int fd;
    drmModeConnector *conn;
    drmModeRes *res;
    uint32_t conn_id;
    uint32_t crtc_id;

    //1，打开 drm 设备
    fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);

    //2,获取 crtc/connector id
    res = drmModeGetResources(fd);
    crtc_id = res->crtc[0];
    conn_id = res->connectors[0];

    //3,获取 display mode 下的connector
    conn = drmModeGetConnector(fd, conn_id);
    buf.width = conn->mode[0].hdisplay;
    buf.height = conn->mode[0].vdisplay;

    modeset_create_fb(fd, &buf);

    //7, 开始显示
    drmModeSetCrtc(fd, crtc_id, buf, fb_id,
                    0, 0, &conn_id, 1, &conn->modes[0]);

    getchar();

    modeset_destroy_fb(fd, &buf);

    drmModeFreeConnector(conn);
    drmModeFreeResouces(res);

    close(fd);

    return 0;

}





