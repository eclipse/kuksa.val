/**********************************************************************
 * Copyright (c) 2021 Robert Bosch GmbH.
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
 **********************************************************************/

#ifndef __OVERLAY_LOADER__H__
#define __OVERLAY_LOADER__H__

#include <vector>
#include <boost/filesystem.hpp>

#include "ILogger.hpp"
#include "IVssDatabase.hpp"


/** Finding overlays to load. Returns an alphanumerically list of paths pointing
 * to *.json files in  directory 
 */
std::vector<boost::filesystem::path> gatherOverlays(std::shared_ptr<ILogger> log, boost::filesystem::path directory);

/** Iterates over paths in vector, tries to parse as JSON and merge with existing
 *  structure in database
 */
void applyOverlays(std::shared_ptr<ILogger> log, const std::vector<boost::filesystem::path> overlayfiles, std::shared_ptr<IVssDatabase> db);

#endif