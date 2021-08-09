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
#include "velox/exec/tests/OperatorTestBase.h"
#include <velox/parse/Expressions.h>
#include <velox/parse/ExpressionsParser.h>
#include "velox/dwio/common/DataSink.h"
#include "velox/exec/Exchange.h"
#include "velox/exec/tests/utils/FunctionUtils.h"
#include "velox/functions/common/CoreFunctions.h"
#include "velox/functions/common/VectorFunctions.h"
#include "velox/serializers/PrestoSerializer.h"

namespace facebook::velox::exec::test {

OperatorTestBase::OperatorTestBase() {
  facebook::velox::exec::ExchangeSource::registerFactory();
  if (!isRegisteredVectorSerde()) {
    velox::serializer::presto::PrestoVectorSerde::registerVectorSerde();
  }
  registerTypeResolver();
}

OperatorTestBase::~OperatorTestBase() {
  exec::Driver::testingJoinAndReinitializeExecutor();
}

void OperatorTestBase::SetUpTestCase() {
  functions::registerFunctions();
  functions::registerVectorFunctions();
}

std::shared_ptr<Task> OperatorTestBase::assertQuery(
    const std::shared_ptr<const core::PlanNode>& plan,
    const std::vector<std::shared_ptr<connector::ConnectorSplit>>&
        connectorSplits,
    const std::string& duckDbSql,
    std::optional<std::vector<uint32_t>> sortingKeys) {
  std::vector<exec::Split> splits;
  splits.reserve(connectorSplits.size());
  for (const auto& connectorSplit : connectorSplits) {
    splits.emplace_back(exec::Split(folly::copy(connectorSplit), -1));
  }

  return assertQuery(plan, std::move(splits), duckDbSql, sortingKeys);
}

std::shared_ptr<Task> OperatorTestBase::assertQuery(
    const std::shared_ptr<const core::PlanNode>& plan,
    std::vector<exec::Split>&& splits,
    const std::string& duckDbSql,
    std::optional<std::vector<uint32_t>> sortingKeys) {
  bool noMoreSplits = false;
  return test::assertQuery(
      plan,
      [&](Task* task) {
        if (noMoreSplits) {
          return;
        }
        for (auto& split : splits) {
          task->addSplit("0", std::move(split));
        }
        task->noMoreSplits("0");
        noMoreSplits = true;
      },
      duckDbSql,
      duckDbQueryRunner_,
      sortingKeys);
}

// static
std::shared_ptr<core::FieldAccessTypedExpr> OperatorTestBase::toFieldExpr(
    const std::string& name,
    const std::shared_ptr<const RowType>& rowType) {
  auto type = rowType->findChild(name);

  std::vector<std::shared_ptr<const core::ITypedExpr>> inputs{
      std::make_shared<core::InputTypedExpr>(rowType)};

  return std::make_shared<core::FieldAccessTypedExpr>(
      type, std::move(inputs), name);
}

std::shared_ptr<const core::ITypedExpr> OperatorTestBase::parseExpr(
    const std::string& text,
    std::shared_ptr<const RowType> rowType) {
  auto untyped = parse::parseExpr(text);
  return core::Expressions::inferTypes(untyped, rowType, pool_.get());
}

} // namespace facebook::velox::exec::test
