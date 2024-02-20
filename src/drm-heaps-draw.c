/*
 * Copyright (C) 2019 Texas Instruments Incorporated - http://www.ti.com/
 *      Andrew Davis <afd@ti.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <drm/drm.h>
#include <drm/drm_mode.h>

#include <linux/dma-buf.h>
#include <linux/dma-heap.h>

//#define DUMB_BUFFERS
//#define TEST_PHYS

#if TEST_PHYS
#include <linux/remoteproc_cdev.h>
#endif

#define MAKE_RGBA(r, g, b, a) \
	(((a) << 24) | \
	 ((r) << 16) | \
	 ((g) <<  8) | \
	 ((b) <<  0))

static void fill_smpte_rgb32(void *mem,
			     unsigned int width, unsigned int height,
			     unsigned int stride)
{
	const uint32_t colors_top[] = {
		MAKE_RGBA(192, 192, 192, 255),	/* grey */
		MAKE_RGBA(192, 192, 0, 255),	/* yellow */
		MAKE_RGBA(0, 192, 192, 255),	/* cyan */
		MAKE_RGBA(0, 192, 0, 255),	/* green */
		MAKE_RGBA(192, 0, 192, 255),	/* magenta */
		MAKE_RGBA(192, 0, 0, 255),	/* red */
		MAKE_RGBA(0, 0, 192, 255),	/* blue */
	};
	const uint32_t colors_middle[] = {
		MAKE_RGBA(0, 0, 192, 127),	/* blue */
		MAKE_RGBA(19, 19, 19, 127),	/* black */
		MAKE_RGBA(192, 0, 192, 127),	/* magenta */
		MAKE_RGBA(19, 19, 19, 127),	/* black */
		MAKE_RGBA(0, 192, 192, 127),	/* cyan */
		MAKE_RGBA(19, 19, 19, 127),	/* black */
		MAKE_RGBA(192, 192, 192, 127),	/* grey */
	};
	const uint32_t colors_bottom[] = {
		MAKE_RGBA(0, 33, 76, 255),	/* in-phase */
		MAKE_RGBA(255, 255, 255, 255),	/* super white */
		MAKE_RGBA(50, 0, 106, 255),	/* quadrature */
		MAKE_RGBA(19, 19, 19, 255),	/* black */
		MAKE_RGBA(9, 9, 9, 255),	/* 3.5% */
		MAKE_RGBA(19, 19, 19, 255),	/* 7.5% */
		MAKE_RGBA(29, 29, 29, 255),	/* 11.5% */
		MAKE_RGBA(19, 19, 19, 255),	/* black */
	};
	unsigned int x;
	unsigned int y;

	for (y = 0; y < height * 6 / 9; ++y) {
		for (x = 0; x < width; ++x)
			((uint32_t *)mem)[x] = colors_top[x * 7 / width];
		mem += stride;
	}

	for (; y < height * 7 / 9; ++y) {
		for (x = 0; x < width; ++x)
			((uint32_t *)mem)[x] = colors_middle[x * 7 / width];
		mem += stride;
	}

	for (; y < height; ++y) {
		for (x = 0; x < width * 5 / 7; ++x)
			((uint32_t *)mem)[x] =
				colors_bottom[x * 4 / (width * 5 / 7)];
		for (; x < width * 6 / 7; ++x)
			((uint32_t *)mem)[x] =
				colors_bottom[(x - width * 5 / 7) * 3
					      / (width / 7) + 4];
		for (; x < width; ++x)
			((uint32_t *)mem)[x] = colors_bottom[7];
		mem += stride;
	}
}

static int heap_alloc(int fd, size_t len, unsigned int flags, int* handle_fd)
{
	if (handle_fd == NULL)
		return -EINVAL;

	struct dma_heap_allocation_data data = {
		.len = len,
		.fd_flags = O_CLOEXEC | O_RDWR,
		.heap_flags = flags,
	};

	int ret = ioctl(fd, DMA_HEAP_IOCTL_ALLOC, &data);
	if (ret < 0)
		return errno;

	*handle_fd = data.fd;

	return 0;
}

static void dmabuf_sync(int fd, int start_stop)
{
	struct dma_buf_sync sync = {
		.flags = start_stop | DMA_BUF_SYNC_WRITE,
	};
	int ret;

	ret = ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync);
	if (ret)
		printf("sync failed %d\n", errno);
}

#if TEST_PHYS
int get_phys_rproc(int fd, u_int64_t *phys)
{
	if (phys == NULL)
		return -EINVAL;

	int phys_fd = open("/dev/remoteproc0", O_RDONLY | O_CLOEXEC);
	if (phys_fd < 0) {
		printf("Failed to open Remoteproc Char device: %s\n", strerror(errno));
		return -EINVAL;
	}

	struct rproc_dma_buf_attach_data data = {
		.fd = fd,
	};

	int ret = ioctl(phys_fd, RPROC_IOC_DMA_BUF_ATTACH, &data);
	if (ret < 0)
		return -errno;

//	__s32 release = 1;
//	ret = ioctl(phys_fd, RPROC_SET_SHUTDOWN_ON_RELEASE, &release);
//	if (ret < 0)
//		return -errno;
//	close(phys_fd);

	*phys = data.da;

	return 0;
}
#endif

static uint32_t allocate_attach_fb(int dri_fd, uint width, uint height, char *heap_dev)
{
	uint32_t handle;

	size_t bpp = 32;
	size_t pitch = width * (bpp / 8);
	size_t size = height * pitch;

	printf("Buffer: bpp: %zd, pitch: %zd, size: %zd\n", bpp, pitch, size);

#ifdef DUMB_BUFFERS
	// Create dumb buffer
	struct drm_mode_create_dumb create_dumb = { 0 };
	create_dumb.width = width;
	create_dumb.height = height;
	create_dumb.bpp = 32;
	create_dumb.flags = 0;
	ioctl(dri_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_dumb);

	printf("Created dumb buffer with size: %llu, pitch: %u, and handle: %u\n", create_dumb.size, create_dumb.pitch, create_dumb.handle);

	handle = create_dumb.handle;
#else
	// Create DMA-BUF from Heaps allocator
	int heap_fd = open(heap_dev, O_RDONLY | O_CLOEXEC);
	if (heap_fd < 0) {
		printf("Failed to open DMA-Heap device: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	int dma_buf_fd;
	int ret = heap_alloc(heap_fd, size, 0, &dma_buf_fd);
	if (ret) {
		printf("DMA-Heap allocation failed: %s\n", strerror(ret));
		exit(EXIT_FAILURE);
	}

	ret = close(heap_fd);
	if (ret < 0) {
		printf("Failed to close DMA-Heap device: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

#if TEST_PHYS
	u_int64_t phys;
	ret = get_phys_rproc(dma_buf_fd, &phys);
	if (ret < 0) {
		printf("Failed to get physical address: %s\n", strerror(ret));
		exit(EXIT_FAILURE);
	}
	printf("RPROC device physical address of buffer: 0x%016llx\n", phys);
#endif

	// Convert DMA-BUF to PRIME(GEM) handle
	struct drm_prime_handle prime_to_handle = { 0 };
	prime_to_handle.fd = dma_buf_fd;
	ret = ioctl(dri_fd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &prime_to_handle);
	if (ret) {
		printf("Could not convert prime fd to handle: %s\n", strerror(ret));
		exit(EXIT_FAILURE);
	}
	handle = prime_to_handle.handle;
#endif

	// Attach buffer to plane
	struct drm_mode_fb_cmd fb_cmd = { 0 };
	fb_cmd.width = width;
	fb_cmd.height = height;
	fb_cmd.bpp = bpp;
	fb_cmd.pitch = pitch;
	fb_cmd.depth = 24;
	fb_cmd.handle = handle;
	ioctl(dri_fd, DRM_IOCTL_MODE_ADDFB, &fb_cmd);

	printf("Buffer added to FB: %u\n", fb_cmd.fb_id);

	// Map buffer to userspace
#ifdef DUMB_BUFFERS
	struct drm_mode_map_dumb map_dumb = { 0 };
	map_dumb.handle = handle;
	ioctl(dri_fd, DRM_IOCTL_MODE_MAP_DUMB, &map_dumb);
	void *fb_base = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, dri_fd, map_dumb.offset);
#else
	void *fb_base = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, dma_buf_fd, 0);
#endif
	if (fb_base == MAP_FAILED) {
		printf("Could not mmap buffer: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	dmabuf_sync(dma_buf_fd, DMA_BUF_SYNC_START);

	// Fill with test pattern
	fill_smpte_rgb32(fb_base, width, height, pitch);

	dmabuf_sync(dma_buf_fd, DMA_BUF_SYNC_END);

	munmap(fb_base, size);

	return fb_cmd.fb_id;
}

#define MAX_CONNECTORS 40
#define MAX_MODES 40

int main(int argc, char* argv[])
{
	if (argc != 2) {
		printf("Usage %s /dev/dma_heap/<heap device>\n", argv[0]);
		return EXIT_FAILURE;
	}

	int dri_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);

	// Become the DRM device master
	ioctl(dri_fd, DRM_IOCTL_SET_MASTER, 0);

	// Get first connector
	struct drm_mode_card_res res = { 0 };
	uint32_t res_conn_buf[MAX_CONNECTORS];
	res.connector_id_ptr = (uint64_t)(uintptr_t)res_conn_buf;
	res.count_connectors = MAX_CONNECTORS;
	ioctl(dri_fd, DRM_IOCTL_MODE_GETRESOURCES, &res);
	uint32_t connector_id = res_conn_buf[0];

	printf("Total connectors: %d, Choosing connector: %d\n", res.count_connectors, connector_id);

	// Get info about connector
	struct drm_mode_get_connector conn = { 0 };
	conn.connector_id = connector_id;

	struct drm_mode_modeinfo conn_mode_buf[MAX_MODES] = { 0 };
	conn.modes_ptr = (uint64_t)(uintptr_t)conn_mode_buf;
	conn.count_modes = MAX_MODES;
	ioctl(dri_fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn);

	// Check if the connector is OK to use (connected to something)
	if (conn.count_encoders < 1 || conn.count_modes < 1 || !conn.encoder_id || !conn.connection) {
		printf("Not connected\n");
		return EXIT_FAILURE;
	}

	printf("Total mode count for connector %d: %d\n", connector_id, conn.count_modes);

	printf("Using connector default mode: %dx%d\n", conn_mode_buf[0].hdisplay, conn_mode_buf[0].vdisplay);

	// All the buffer handling
	uint32_t fb_id = allocate_attach_fb(dri_fd, conn_mode_buf[0].hdisplay, conn_mode_buf[0].vdisplay, argv[1]);

	// Get current CRTC for our encoder
	struct drm_mode_get_encoder enc = { 0 };
	enc.encoder_id = conn.encoder_id;
	ioctl(dri_fd, DRM_IOCTL_MODE_GETENCODER, &enc);

	// Get info about CRTC
	struct drm_mode_crtc crtc = { 0 };
	crtc.crtc_id = enc.crtc_id;
	ioctl(dri_fd, DRM_IOCTL_MODE_GETCRTC, &crtc);

	// Setup CRTC
	crtc.fb_id = fb_id;
	crtc.set_connectors_ptr = (uint64_t)(uintptr_t)&connector_id;
	crtc.count_connectors = 1;
	crtc.mode = conn_mode_buf[0];
	crtc.mode_valid = 1;
	ioctl(dri_fd, DRM_IOCTL_MODE_SETCRTC, &crtc);

	// Stop being DRM device master
	ioctl(dri_fd, DRM_IOCTL_DROP_MASTER, 0);

	// Wait for keypress
	getchar();

	return 0;
}
