/*
 * ******************************************************************************
 * Copyright (c) 2019 Robert Bosch GmbH.
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

#include "wsserver.hpp"

#define PORT 8090

/**
 * @brief  Test main.
 * @return
 */
int main(int argc, char* argv[]) {
  (void) argc;
  (void) argv;

  wsserver server(PORT, true);
  server.start();

  while (1) {
    usleep(1000000);
  };
}
