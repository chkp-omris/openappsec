// Copyright (C) 2022 Check Point Software Technologies Ltd. All rights reserved.

// Licensed under the Apache License, Version 2.0 (the "License");
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "generic_rulebase/evaluators/parameter_eval.h"

#include <vector>
#include <string>

#include "generic_rulebase/rulebase_config.h"
#include "config.h"
#include "debug.h"

using namespace std;

string ParameterMatcher::ctx_key = "parameters";

ParameterMatcher::ParameterMatcher(const vector<string> &params)
{
    if (params.size() != 1) reportWrongNumberOfParams(ParameterMatcher::getName(), params.size(), 1, 1);
    parameter_id = params[0];
}

Maybe<bool, Context::Error>
ParameterMatcher::evalVariable() const
{
    auto rule = getConfiguration<BasicRuleConfig>("rulebase", "rulesConfig");
    return rule.ok() && rule.unpack().isParameterActive(parameter_id);
}
