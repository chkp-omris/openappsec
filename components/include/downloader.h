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

#ifndef __DOWNLOADER_H__
#define __DOWNLOADER_H__

#include "i_downloader.h"
#include "i_orchestration_tools.h"
#include "i_update_communication.h"
#include "i_encryptor.h"
#include "url_parser.h"
#include "i_agent_details.h"
#include "i_mainloop.h"
#include "singleton.h"
#include "component.h"

class Downloader
        :
    public Component,
    Singleton::Provide<I_Downloader>,
    Singleton::Consume<I_AgentDetails>,
    Singleton::Consume<I_Encryptor>,
    Singleton::Consume<I_MainLoop>,
    Singleton::Consume<I_OrchestrationTools>,
    Singleton::Consume<I_UpdateCommunication>
{
public:
    Downloader();
    ~Downloader();

    void preload() override;

    void init() override;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl;
};

#endif // __DOWNLOADER_H__
