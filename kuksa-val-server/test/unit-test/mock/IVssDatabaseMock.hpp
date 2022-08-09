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

#include "IVssDatabase.hpp"

MOCK_BASE_CLASS( IVssDatabaseMock, IVssDatabase )
{
  MOCK_METHOD(initJsonTree, 1)
  MOCK_METHOD(updateJsonTree, 2)
  MOCK_METHOD(updateMetaData, 3)
  MOCK_METHOD(getMetaData, 1)
  MOCK_METHOD(setSignal, 3)
  MOCK_METHOD(getSignal, 3 )
  MOCK_METHOD(pathExists, 1)
  MOCK_METHOD(pathIsWritable, 1)
  MOCK_METHOD(pathIsReadable, 1)
  MOCK_METHOD(pathIsAttributable, 2)
  MOCK_METHOD(getLeafPaths,1)
  MOCK_METHOD(getDatatypeForPath,1)
  MOCK_METHOD(checkAndSanitizeType,2)
};
  

