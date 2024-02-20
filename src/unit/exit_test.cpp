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

class Exit : public HeapAllHeapsTest {};

TEST_F(Exit, WithAlloc)
{
	static const size_t allocationSizes[] = {4*1024, 64*1024, 1024*1024, 2*1024*1024};
	for (struct Heap heap : m_allHeaps) {
		for (size_t size : allocationSizes) {
			SCOPED_TRACE(::testing::Message() << "heap " << heap.dev_name);
			SCOPED_TRACE(::testing::Message() << "size " << size);
			EXPECT_EXIT({
				int handleFd = -1;
				ASSERT_EQ(0, heap_alloc(heap.fd, size, 0, &handleFd));
				ASSERT_TRUE(handleFd >= 0);
				exit(0);
			}, ::testing::ExitedWithCode(0), "");
		}
	}
}

TEST_F(Exit, WithAllocFd)
{
	static const size_t allocationSizes[] = {4*1024, 64*1024, 1024*1024, 2*1024*1024};
	for (struct Heap heap : m_allHeaps) {
		for (size_t size : allocationSizes) {
			SCOPED_TRACE(::testing::Message() << "heap " << heap.dev_name);
			SCOPED_TRACE(::testing::Message() << "size " << size);
			EXPECT_EXIT({
				int handle_fd = -1;

				ASSERT_EQ(0, heap_alloc(heap.fd, size, 0, &handle_fd));
				ASSERT_NE(-1, handle_fd);
				exit(0);
			}, ::testing::ExitedWithCode(0), "");
		}
	}
}

TEST_F(Exit, WithRepeatedAllocFd)
{
	static const size_t allocationSizes[] = {4*1024, 64*1024, 1024*1024, 2*1024*1024};
	for (struct Heap heap : m_allHeaps) {
		for (size_t size : allocationSizes) {
			for (unsigned int i = 0; i < 64; i++) {
				SCOPED_TRACE(::testing::Message() << "heap " << heap.dev_name);
				SCOPED_TRACE(::testing::Message() << "size " << size);
				ASSERT_EXIT({
					int handle_fd = -1;

					ASSERT_EQ(0, heap_alloc(heap.fd, size, 0, &handle_fd));
					ASSERT_NE(-1, handle_fd);
					exit(0);
				}, ::testing::ExitedWithCode(0), "")
					<< "failed on heap " << heap.dev_name
					<< " and size " << size
					<< " on iteration " << i;
			}
		}
	}
}

TEST_F(Exit, WithMapping)
{
	static const size_t allocationSizes[] = {4*1024, 64*1024, 1024*1024, 2*1024*1024};
	for (struct Heap heap : m_allHeaps) {
		for (size_t size : allocationSizes) {
			SCOPED_TRACE(::testing::Message() << "heap " << heap.dev_name);
			SCOPED_TRACE(::testing::Message() << "size " << size);
			EXPECT_EXIT({
				int map_fd = -1;

				ASSERT_EQ(0, heap_alloc(heap.fd, size, 0, &map_fd));
				ASSERT_GE(map_fd, 0);

				void *ptr;
				ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, map_fd, 0);
				ASSERT_TRUE(ptr != NULL);
				exit(0);
			}, ::testing::ExitedWithCode(0), "");
		}
	}
}

TEST_F(Exit, WithPartialMapping)
{
	static const size_t allocationSizes[] = {64*1024, 1024*1024, 2*1024*1024};
	for (struct Heap heap : m_allHeaps) {
		for (size_t size : allocationSizes) {
			SCOPED_TRACE(::testing::Message() << "heap " << heap.dev_name);
			SCOPED_TRACE(::testing::Message() << "size " << size);
			EXPECT_EXIT({
				int map_fd = -1;

				ASSERT_EQ(0, heap_alloc(heap.fd, size, 0, &map_fd));
				ASSERT_GE(map_fd, 0);

				void *ptr;
				ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, map_fd, 0);
				ASSERT_TRUE(ptr != NULL);

				ASSERT_EQ(0, munmap(ptr, size / 2));
				exit(0);
			}, ::testing::ExitedWithCode(0), "");
		}
	}
}
