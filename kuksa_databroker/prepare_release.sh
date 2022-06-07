#!/bin/bash
#********************************************************************************
# Copyright (c) 2022 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License 2.0 which is available at
# http://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
#*******************************************************************************/
set -e

if [ "$#" -ne 1 ]; then
	echo "Usage: $0 <version>"
	exit 1
fi

VERSION_REGEX="[0-9]+\.[0-9]+(\.[0-9]+)?"
VERSION="$1"

if [ "$(echo "$1" | sed -E "s/$VERSION_REGEX//")" ]; then
	echo "<version> should be of the form MAJOR.MINOR[.PATCH]"
	exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
DATA_BROKER_ROOT="$SCRIPT_DIR"

# Update Cargo.toml versions.
sed -i -E "s/^version = \"${VERSION_REGEX}\"$/version = \"${VERSION}\"/" \
	"$DATA_BROKER_ROOT/databroker/Cargo.toml" \
	"$DATA_BROKER_ROOT/databroker-api/Cargo.toml" \
	"$DATA_BROKER_ROOT/databroker-cli/Cargo.toml" \
	"$DATA_BROKER_ROOT/databroker-examples/Cargo.toml"

# Create release commit and tag it
#git commit -a -m "Release ${VERSION}"
#git tag -a "v${VERSION}" -m "Release ${VERSION}"
