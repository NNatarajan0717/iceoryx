// Copyright (c) 2019 by Robert Bosch GmbH. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef IOX_POSH_MOCKS_WAITSET_MOCK_HPP
#define IOX_POSH_MOCKS_WAITSET_MOCK_HPP

#include "iceoryx_posh/popo/wait_set.hpp"

class WaitSetMock : public iox::popo::WaitSet<>
{
  public:
    WaitSetMock(iox::popo::ConditionVariableData* condVarDataPtr) noexcept
        : WaitSet(condVarDataPtr)
    {
    }

    using WaitSet::EVENT_ACCESSOR;
};


#endif
