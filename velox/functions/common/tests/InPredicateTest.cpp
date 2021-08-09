/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "velox/functions/common/tests/FunctionBaseTest.h"

using namespace facebook::velox;
using namespace facebook::velox::functions::test;

class InPredicateTest : public FunctionBaseTest {
 protected:
  template <typename T>
  void testIntegers() {
    const vector_size_t size = 1'000;
    auto vector = makeFlatVector<T>(size, [](auto row) { return row % 17; });
    auto vectorWithNulls = makeFlatVector<T>(
        size, [](auto row) { return row % 17; }, nullEvery(7));
    auto rowVector = makeRowVector({vector, vectorWithNulls});

    // no nulls
    auto result = evaluate<SimpleVector<bool>>("c0 IN (1, 3, 5)", rowVector);
    auto expected = makeFlatVector<bool>(size, [](auto row) {
      auto n = row % 17;
      return n == 1 || n == 3 || n == 5;
    });

    assertEqualVectors(expected, result);

    // some nulls
    result = evaluate<SimpleVector<bool>>("c1 IN (1, 3, 5)", rowVector);
    expected = makeFlatVector<bool>(
        size,
        [](auto row) {
          auto n = row % 17;
          return n == 1 || n == 3 || n == 5;
        },
        nullEvery(7));

    assertEqualVectors(expected, result);

    // null values in the in-list
    // The results can be either true or null, but not false.
    result = evaluate<SimpleVector<bool>>("c0 IN (1, 3, null, 5)", rowVector);
    expected = makeFlatVector<bool>(
        size,
        [](auto /* row */) { return true; },
        [](auto row) {
          auto n = row % 17;
          return !(n == 1 || n == 3 || n == 5);
        });

    assertEqualVectors(expected, result);

    result = evaluate<SimpleVector<bool>>("c1 IN (1, 3, null, 5)", rowVector);
    expected = makeFlatVector<bool>(
        size,
        [](auto /* row */) { return true; },
        [](auto row) {
          auto n = row % 17;
          return row % 7 == 0 || !(n == 1 || n == 3 || n == 5);
        });

    assertEqualVectors(expected, result);

    result = evaluate<SimpleVector<bool>>("c0 IN (2, null)", rowVector);
    expected = makeFlatVector<bool>(
        size,
        [](auto /* row */) { return true; },
        [](auto row) {
          auto n = row % 17;
          return !(n == 2);
        });

    assertEqualVectors(expected, result);

    result = evaluate<SimpleVector<bool>>("c1 IN (2, null)", rowVector);
    expected = makeFlatVector<bool>(
        size,
        [](auto /* row */) { return true; },
        [](auto row) {
          auto n = row % 17;
          return row % 7 == 0 || !(n == 2);
        });

    assertEqualVectors(expected, result);
  }

  template <typename T>
  void testsIntegerConstant() {
    const vector_size_t size = 1'000;
    auto rowVector = makeRowVector(
        {makeConstant((T)123, size),
         makeNullConstant(CppToType<T>::create()->kind(), size)});

    auto constTrue = makeConstant(true, size);
    auto constFalse = makeConstant(false, size);
    auto constNull = makeNullConstant(TypeKind::BOOLEAN, size);

    // a miss
    auto result = evaluate<SimpleVector<bool>>("c0 IN (1, 3, 5)", rowVector);
    assertEqualVectors(constFalse, result);

    // a hit
    result = evaluate<SimpleVector<bool>>("c0 IN (1, 123, 5)", rowVector);
    assertEqualVectors(constTrue, result);

    // a miss that is a null
    result = evaluate<SimpleVector<bool>>("c0 IN (1, null, 5)", rowVector);
    assertEqualVectors(constNull, result);

    // null
    result = evaluate<SimpleVector<bool>>("c1 IN (1, 3, 5)", rowVector);
    assertEqualVectors(constNull, result);
  }
};

TEST_F(InPredicateTest, bigint) {
  testIntegers<int64_t>();
  testsIntegerConstant<int64_t>();
}

TEST_F(InPredicateTest, integer) {
  testIntegers<int32_t>();
  testsIntegerConstant<int32_t>();
}

TEST_F(InPredicateTest, smallint) {
  testIntegers<int16_t>();
  testsIntegerConstant<int16_t>();
}

TEST_F(InPredicateTest, tinyint) {
  testIntegers<int8_t>();
  testsIntegerConstant<int8_t>();
}

TEST_F(InPredicateTest, varchar) {
  const vector_size_t size = 1'000;

  std::vector<std::string> fruits = {
      "apple", "banana", "pear", "grapes", "mango", "grapefruit"};

  auto vector = makeFlatVector<StringView>(size, [&fruits](auto row) {
    return StringView(fruits[row % fruits.size()]);
  });
  auto vectorWithNulls = makeFlatVector<StringView>(
      size,
      [&fruits](auto row) { return StringView(fruits[row % fruits.size()]); },
      nullEvery(7));
  auto rowVector = makeRowVector({vector, vectorWithNulls});

  // no nulls
  auto result = evaluate<SimpleVector<bool>>(
      "c0 IN ('apple', 'pear', 'banana')", rowVector);
  auto expected = makeFlatVector<bool>(size, [&fruits](auto row) {
    auto fruit = fruits[row % fruits.size()];
    return fruit == "apple" || fruit == "pear" || fruit == "banana";
  });

  assertEqualVectors(expected, result);

  // some nulls
  result = evaluate<SimpleVector<bool>>(
      "c1 IN ('apple', 'pear', 'banana')", rowVector);
  expected = makeFlatVector<bool>(
      size,
      [&fruits](auto row) {
        auto fruit = fruits[row % fruits.size()];
        return fruit == "apple" || fruit == "pear" || fruit == "banana";
      },
      nullEvery(7));

  assertEqualVectors(expected, result);

  // null values in the in-list
  // The results can be either true or null, but not false.
  result = evaluate<SimpleVector<bool>>(
      "c0 IN ('apple', 'pear', null, 'banana')", rowVector);
  expected = makeFlatVector<bool>(
      size,
      [](auto /* row */) { return true; },
      [&fruits](auto row) {
        auto fruit = fruits[row % fruits.size()];
        return !(fruit == "apple" || fruit == "pear" || fruit == "banana");
      });

  assertEqualVectors(expected, result);

  result = evaluate<SimpleVector<bool>>(
      "c1 IN ('apple', 'pear', null, 'banana')", rowVector);
  expected = makeFlatVector<bool>(
      size,
      [](auto /* row */) { return true; },
      [&fruits](auto row) {
        auto fruit = fruits[row % fruits.size()];
        return row % 7 == 0 ||
            !(fruit == "apple" || fruit == "pear" || fruit == "banana");
      });

  assertEqualVectors(expected, result);

  result = evaluate<SimpleVector<bool>>("c0 IN ('banana', null)", rowVector);
  expected = makeFlatVector<bool>(
      size,
      [](auto /* row */) { return true; },
      [&fruits](auto row) {
        auto fruit = fruits[row % fruits.size()];
        return !(fruit == "banana");
      });

  assertEqualVectors(expected, result);

  result = evaluate<SimpleVector<bool>>("c1 IN ('banana', null)", rowVector);
  expected = makeFlatVector<bool>(
      size,
      [](auto /* row */) { return true; },
      [&fruits](auto row) {
        auto fruit = fruits[row % fruits.size()];
        return row % 7 == 0 || !(fruit == "banana");
      });

  assertEqualVectors(expected, result);
}

TEST_F(InPredicateTest, varcharConstant) {
  const vector_size_t size = 1'000;
  auto rowVector = makeRowVector(
      {makeConstant("apple", size), makeNullConstant(TypeKind::VARCHAR, size)});

  auto constTrue = makeConstant(true, size);
  auto constFalse = makeConstant(false, size);
  auto constNull = makeNullConstant(TypeKind::BOOLEAN, size);

  // a miss
  auto result =
      evaluate<SimpleVector<bool>>("c0 IN ('pear', 'banana')", rowVector);
  assertEqualVectors(constFalse, result);

  // a hit
  result = evaluate<SimpleVector<bool>>(
      "c0 IN ('apple', 'pear', 'banana')", rowVector);
  assertEqualVectors(constTrue, result);

  // a miss that is a null
  result =
      evaluate<SimpleVector<bool>>("c0 IN ('pear', null, 'banana')", rowVector);
  assertEqualVectors(constNull, result);

  // null
  result = evaluate<SimpleVector<bool>>(
      "c1 IN ('apple', 'pear', 'banana')", rowVector);
  assertEqualVectors(constNull, result);
}
