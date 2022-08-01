/**********************************************************************
 * Copyright (c) 2019 Robert Bosch GmbH.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Contributors:
 *      Robert Bosch GmbH
 **********************************************************************/


#pragma once

#include <turtle/mock.hpp>

#include "IServer.hpp"
#include "ISubscriptionHandler.hpp"

MOCK_BASE_CLASS( ISubscriptionHandlerMock, ISubscriptionHandler )
{
  MOCK_METHOD(subscribe, 4)
  MOCK_METHOD(unsubscribe, 1)
  MOCK_METHOD(unsubscribeAll, 1)
  MOCK_METHOD(publishForVSSPath, 4)
  MOCK_METHOD(getServer, 0)
  MOCK_METHOD(startThread, 0)
  MOCK_METHOD(stopThread, 0)
  MOCK_CONST_METHOD(isThreadRunning, 0, bool(void))
  MOCK_METHOD(subThreadRunner, 0, void*(void))
  MOCK_METHOD(addPublisher, 1, void(std::shared_ptr<IPublisher>))
};
