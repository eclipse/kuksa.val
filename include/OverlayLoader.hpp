/*
 * ******************************************************************************
 * Copyright (c) 2021 Robert Bosch GmbH.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * which accompanies this distribution, and is available at
 * https://www.eclipse.org/org/documents/epl-2.0/index.php
 *
 * *****************************************************************************
 */

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