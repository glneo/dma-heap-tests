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
#include <sys/mman.h>

#include <gtest/gtest.h>

#include "heap_test_fixture.h"
#include "heap_helper.h"

class InvalidValues: public HeapAllHeapsTest {
public:
	virtual void SetUp();
	virtual void TearDown();
	int m_validFd;
	int const m_badFd = -1;
};

void InvalidValues::SetUp()
{
	HeapAllHeapsTest::SetUp();
	ASSERT_EQ(0, heap_alloc(m_allHeaps[0].fd, 4096, 0, &m_validFd));
	ASSERT_TRUE(m_validFd >= 0);
}

void InvalidValues::TearDown()
{
	ASSERT_EQ(0, close(m_validFd));
	m_validFd = -1;
	HeapAllHeapsTest::TearDown();
}

TEST_F(InvalidValues, heap_alloc_fd)
{
	int fd;
	for (struct Heap heap : m_allHeaps) {
		SCOPED_TRACE(::testing::Message() << "heap " << heap.dev_name);
		/* zero size */
		EXPECT_EQ(-EINVAL, heap_alloc(heap.fd, 0, 0, &fd));
		/* too large size */
		EXPECT_EQ(-EINVAL, heap_alloc(heap.fd, -1, 0, &fd));
		/* NULL handle */
		EXPECT_EQ(-EINVAL, heap_alloc(heap.fd, 4096, 0, NULL));
	}
}

TEST_F(InvalidValues, heap_map)
{
	/* bad fd */
	EXPECT_EQ(MAP_FAILED, mmap(NULL, 4096, PROT_READ, 0, m_badFd, 0));
	/* zero length */
	EXPECT_EQ(MAP_FAILED, mmap(NULL, 0, PROT_READ, 0, m_validFd, 0));
	/* bad prot */
	EXPECT_EQ(MAP_FAILED, mmap(NULL, 4096, -1, 0, m_validFd, 0));
	/* bad offset */
	EXPECT_EQ(MAP_FAILED, mmap(NULL, 4096, PROT_READ, 0, m_validFd, -1));
}
