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

#ifndef __CONFIG_TYPES_H__
#define __CONFIG_TYPES_H__

#ifndef __CONFIG_H__
#error "config_types.h should not be included directly"
#endif // __CONFIG_H__

#include <functional>

namespace Config
{

using ConfigCb = std::function<void(void)>;

enum class Errors { MISSING_TAG, MISSING_CONTEXT, BAD_NODE };
enum class ConfigFileType { Policy, Data, RawData, COUNT };

} // namespace Config

#endif // __CONFIG_TYPES_H__
