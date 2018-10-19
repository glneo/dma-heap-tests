/*
 * Copyright (C) 2013 The Android Open Source Project
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

#include <sys/mman.h>

#include <gtest/gtest.h>

#include "heap_test_fixture.h"
#include "heap_helper.h"

class Map: public HeapAllHeapsTest {};

TEST_F(Map, MapFd)
{
	static const size_t allocationSizes[] = { 4 * 1024, 64 * 1024, 1024 * 1024, 2 * 1024 * 1024 };
	for (struct Heap heap : m_allHeaps) {
		for (size_t size : allocationSizes) {
			SCOPED_TRACE(::testing::Message() << "heap " << heap.dev_name);
			SCOPED_TRACE(::testing::Message() << "size " << size);
			int map_fd = -1;

			ASSERT_EQ(0, heap_alloc(heap.fd, size, 0, &map_fd));
			ASSERT_GE(map_fd, 0);

			void *ptr;
			ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, map_fd, 0);
			ASSERT_TRUE(ptr != MAP_FAILED);

			ASSERT_EQ(0, close(map_fd));

			memset(ptr, 0xaa, size);

			ASSERT_EQ(0, munmap(ptr, size));
		}
	}
}

TEST_F(Map, MapOffset) {
	for (struct Heap heap : m_allHeaps) {
		SCOPED_TRACE(::testing::Message() << "heap " << heap.dev_name);
		int map_fd = -1;

		unsigned long psize = sysconf(_SC_PAGESIZE);

		ASSERT_EQ(0, heap_alloc(heap.fd, psize * 2, 0, &map_fd));
		ASSERT_GE(map_fd, 0);

		unsigned char *ptr;
		ptr = (unsigned char *) mmap(NULL, psize * 2, PROT_READ | PROT_WRITE, MAP_SHARED, map_fd, 0);
		ASSERT_TRUE(ptr != NULL);

		memset(ptr, 0, psize);
		memset(ptr + psize, 0xaa, psize);

		ASSERT_EQ(0, munmap(ptr, psize * 2));

		ptr = (unsigned char *) mmap(NULL, psize, PROT_READ | PROT_WRITE, MAP_SHARED, map_fd, psize);
		ASSERT_TRUE(ptr != NULL);

		ASSERT_EQ(ptr[0], 0xaa);
		ASSERT_EQ(ptr[psize - 1], 0xaa);

		ASSERT_EQ(0, munmap(ptr, psize));

		ASSERT_EQ(0, close(map_fd));
	}
}
