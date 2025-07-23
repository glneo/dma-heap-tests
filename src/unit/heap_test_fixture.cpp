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

#include <cerrno>
#include <fcntl.h>
#include <dirent.h>
#include <gtest/gtest.h>

#include "heap_test_fixture.h"

#define HEAP_DIR "/dev/dma_heap"

HeapAllHeapsTest::HeapAllHeapsTest() :
	m_allHeaps()
{
}

void HeapAllHeapsTest::SetUp()
{
	struct dirent *entry = nullptr;
	DIR *dp = nullptr;

	dp = opendir(HEAP_DIR);
	if (dp != nullptr) {
		while ((entry = readdir(dp))) {
			if (!strcmp (entry->d_name, "."))
				continue;
			if (!strcmp (entry->d_name, ".."))
				continue;

			struct Heap heap;
			heap.dev_name = (std::string)HEAP_DIR + "/" + entry->d_name;
			heap.fd = open(heap.dev_name.c_str(), O_RDONLY | O_CLOEXEC);
			if (heap.fd < 0)
				continue;
			m_allHeaps.push_back(heap);
		}
	}
	closedir(dp);

	RecordProperty("Heaps", m_allHeaps.size());
}

void HeapAllHeapsTest::TearDown()
{
	for (struct Heap heap : m_allHeaps) {
		ASSERT_EQ(0, close(heap.fd));
	}
}
