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

#include "velox/vector/SimpleVector.h"
#include "velox/common/base/Exceptions.h"
#include "velox/vector/ConstantVector.h"
#include "velox/vector/DictionaryVector.h"
#include "velox/vector/FlatVector.h"

namespace facebook {
namespace velox {
using functions::stringCore::isAscii;
using functions::stringCore::StringEncodingMode;

template <>
void SimpleVector<StringView>::setMinMax(
    const folly::F14FastMap<std::string, std::string>& metaData) {
  auto it = metaData.find(META_MIN);
  if (it != metaData.end()) {
    minString_ = it->second;
    min_ = StringView(minString_);
  }
  it = metaData.find(META_MAX);
  if (it != metaData.end()) {
    maxString_ = it->second;
    max_ = StringView(maxString_);
  }
}

} // namespace velox
} // namespace facebook
