/*
Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

/*
hipMallocArray API test scenarios
1. Basic Functionality
2. Negative Scenarios
3. Allocating Small and big chunk data
4. Multithreaded scenario
*/


#include <hip_test_common.hh>

static constexpr auto NUM_W{4};
static constexpr auto BIGNUM_W{100};
static constexpr auto BIGNUM_H{100};
static constexpr auto NUM_H{4};
static constexpr auto ARRAY_LOOP{100};

/*
 * This API verifies  memory allocations for small and
 * bigger chunks of data.
 * Two scenarios are verified in this API
 * 1. NUM_W(small Data): Allocates NUM_W*NUM_H in a loop and
 *    releases the memory and verifies the meminfo.
 * 2. BIGNUM_W(big data): Allocates BIGNUM_W*BIGNUM_H in a loop and
 *    releases the memory and verifies the meminfo
 *
 * In both cases, the memory info before allocation and
 * after releasing the memory should be the same
 *
 */

static void MallocArray_DiffSizes(int gpu) {
  HIP_CHECK(hipSetDevice(gpu));
  std::vector<size_t> array_size;
  array_size.push_back(NUM_W);
  array_size.push_back(BIGNUM_W);
  for (auto &size : array_size) {
    hipArray* A_d[ARRAY_LOOP];
    size_t tot, avail, ptot, pavail;
    hipChannelFormatDesc desc = hipCreateChannelDesc<float>();
    HIP_CHECK(hipMemGetInfo(&pavail, &ptot));
    for (int i = 0; i < ARRAY_LOOP; i++) {
      if (size == NUM_W) {
        HIP_CHECK(hipMallocArray(&A_d[i], &desc,
              NUM_W, NUM_H,
              hipArrayDefault));
      } else {
        HIP_CHECK(hipMallocArray(&A_d[i], &desc,
              BIGNUM_W, BIGNUM_H,
              hipArrayDefault));
      }
    }
    for (int i = 0; i < ARRAY_LOOP; i++) {
      HIP_CHECK(hipFreeArray(A_d[i]));
    }
    HIP_CHECK(hipMemGetInfo(&avail, &tot));
    if ((pavail != avail)) {
      HIPASSERT(false);
    }
  }
}

static void MallocArrayThreadFunc(int gpu) {
  MallocArray_DiffSizes(gpu);
}

/*
 * This testcase verifies the negative scenarios of
 * hipMallocArray API
 */
TEST_CASE("Unit_hipMallocArray_Negative") {
  hipArray* A_d;
  hipChannelFormatDesc desc = hipCreateChannelDesc<float>();
#if HT_NVIDIA
  SECTION("NullPointer to Array") {
    REQUIRE(hipMallocArray(nullptr, &desc,
            NUM_W, NUM_H, hipArrayDefault) != hipSuccess);
  }

  SECTION("NullPointer to Channel Descriptor") {
    REQUIRE(hipMallocArray(&A_d, nullptr,
            NUM_W, NUM_H, hipArrayDefault) != hipSuccess);
  }
#endif
  SECTION("Width 0 in hipMallocArray") {
    REQUIRE(hipMallocArray(&A_d, &desc,
            0, NUM_H, hipArrayDefault) != hipSuccess);
  }

  SECTION("Height 0 in hipMallocArray") {
    REQUIRE(hipMallocArray(&A_d, &desc,
            NUM_W, 0, hipArrayDefault) == hipSuccess);
  }

  SECTION("Invalid Flag") {
    REQUIRE(hipMallocArray(&A_d, &desc,
            NUM_W, NUM_H, 100) != hipSuccess);
  }

  SECTION("Max int values") {
    REQUIRE(hipMallocArray(&A_d, &desc,
            std::numeric_limits<int>::max(),
            std::numeric_limits<int>::max(),
            hipArrayDefault) != hipSuccess);
  }
}
/*
 * This testcase verifies the basic scenario of
 * hipMallocArray API for different datatypes
 * of size 10
 */
TEMPLATE_TEST_CASE("Unit_hipMallocArray_Basic",
                   "", int, unsigned int, float) {
  hipArray* A_d;
  hipChannelFormatDesc desc = hipCreateChannelDesc<TestType>();
  REQUIRE(hipMallocArray(&A_d, &desc,
                         NUM_W, NUM_H,
                         hipArrayDefault) == hipSuccess);
  HIP_CHECK(hipFreeArray(A_d));
}


TEST_CASE("Unit_hipMallocArray_DiffSizes") {
  MallocArray_DiffSizes(0);
}


/*
This testcase verifies the hipMallocArray API in multithreaded
scenario by launching threads in parallel on multiple GPUs
and verifies the hipMallocArray API with small and big chunks data
*/
TEST_CASE("Unit_hipMallocArray_MultiThread") {
  std::vector<std::thread> threadlist;
  int devCnt = 0;
  devCnt = HipTest::getDeviceCount();
  size_t tot, avail, ptot, pavail;
  HIP_CHECK(hipMemGetInfo(&pavail, &ptot));
  for (int i = 0; i < devCnt; i++) {
    threadlist.push_back(std::thread(MallocArrayThreadFunc, i));
  }

  for (auto &t : threadlist) {
    t.join();
  }
  HIP_CHECK(hipMemGetInfo(&avail, &tot));

  if (pavail != avail) {
    WARN("Memory leak of hipMalloc3D API in multithreaded scenario");
    REQUIRE(false);
  }
}
