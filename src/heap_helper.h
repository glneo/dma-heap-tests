/*
 * Copyright (C) 2020 Texas Instruments Incorporated - http://www.ti.com/
 *      Andrew F. Davis <afd@ti.com>
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

#ifndef HEAP_HELPER_H_
#define HEAP_HELPER_H_

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include <linux/dma-heap.h>

__BEGIN_DECLS

static int heap_open(const char *heap_path)
{
	int fd = open(heap_path, O_RDONLY | O_CLOEXEC);
	if (fd < 0)
		return -errno;

	return fd;
}

static int heap_close(int fd)
{
	int ret = close(fd);
	if (ret < 0)
		return -errno;

	return 0;
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
		return -errno;

	*handle_fd = data.fd;

	return 0;
}

__END_DECLS

#endif /* HEAP_HELPER_H_ */
