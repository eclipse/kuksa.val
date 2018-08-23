/*
 * ******************************************************************************
 * Copyright (c) 2018 Robert Bosch GmbH.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * which accompanies this distribution, and is available at
 * https://www.eclipse.org/org/documents/epl-2.0/index.php
 *
 *  Contributors:
 *      Robert Bosch GmbH - initial API and functionality
 * *****************************************************************************
 */

#include"accesschecker.hpp"

accesschecker::accesschecker(class  authenticator* vdator) {
   tokenValidator = vdator;
}

bool accesschecker::checkAccess(class wschannel& channel , string path) {
 
   if (channel.isAuthorized() ) {
       return tokenValidator->isStillValid (channel); 
   } else {
       return false;
   }

   return true;
} 
