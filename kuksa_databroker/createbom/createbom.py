#!/usr/bin/env python3
########################################################################
# Copyright (c) 2022 Robert Bosch GmbH
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0
########################################################################

"""
This script will generate a list of all dependencies and licenses of a
Rust project. It will create a folder called thirdparty in that
project folder containing a list of dependencies and a copy
of each license used in dependencies
"""

import argparse
import sys
import json
import re
import os
import gzip

from subprocess import check_output, CalledProcessError

from bomutil.maplicensefile import MAP as supported_licenses
from bomutil import quirks


class LicenseException(Exception):
    pass


class CargoLicenseException(Exception):
    pass


def extract_license_ids(crate):
    """Extract valid licenses for each dependency. We most of the time
    do not care whether it is "AND" or "OR" currently, we currently assume we are
    compatible to all "OR" variants, and thus include all"""
    crate = quirks.apply_quirks(crate)
    license = crate["license"]

    if license:
        license_ids = re.split(r"\s*AND\s*|\s*OR\s*|\(|\)", license)
        license_ids = list(filter(None, license_ids))
        return license_ids
    else:
        err = f"No license specified for dependency {crate['name']}"
        raise LicenseException(err)


def generate_bom(source_path, target_path):
    try:
        cargo_output = check_output(
            [
                "cargo",
                "license",
                "--json",
                "--avoid-build-deps",
                "--current-dir",
                source_path,
            ]
        )
    except CalledProcessError as e:
        raise CargoLicenseException(f"Error running cargo license: {e}")

    crates = json.loads(cargo_output)

    license_files = set()
    errors = []
    for crate in crates:
        try:
            license_ids = extract_license_ids(crate)
            print(f"Licenses for {crate['name']}: {license_ids}")
            for license_id in license_ids:
                if license_id in supported_licenses:
                    license_file = supported_licenses[license_id]
                    license_files.add(license_file)
                else:
                    err = (
                        f"Could not find license text for {license_id}"
                        f" used in dependency {crate['name']}"
                    )
                    errors.append(err)

        except LicenseException as e:
            errors.append(e)

    if errors:
        for error in errors:
            print(error)
        raise LicenseException("BOM creation failed, unresolved licenses detected")

    # Exporting
    os.mkdir(target_path)

    for license_file in license_files:
        print(f"Copying {license_file[:-2]}")
        with gzip.open("licensestore/" + license_file, "rb") as inf:
            content = inf.read()
        with open(os.path.join(target_path, license_file[:-3]), "wb") as outf:
            outf.write(content)

    print("Writing thirdparty_components.txt")
    with open(
        os.path.join(target_path, "thirdparty_components.txt"), "w", encoding="utf-8"
    ) as jsonout:
        json.dump(crates, jsonout, indent=4)


def main(args=None):
    parser = argparse.ArgumentParser()
    parser.add_argument("dir", help="Rust project directory")
    args = parser.parse_args(args)

    source_path = os.path.abspath(args.dir)
    target_path = os.path.join(source_path, "thirdparty")

    if os.path.exists(target_path):
        print(
            f"Folder {target_path} already exists. Remove it before running this script."
        )
        return -2

    print(f"Generating BOM for project in {source_path}")
    try:
        generate_bom(source_path, target_path)
    except LicenseException as e:
        print(f"Error: {e}")
        return -100
    except CargoLicenseException as e:
        print(f"Error: {e}")
        return -1


if __name__ == "__main__":
    import sys

    sys.exit(main(sys.argv[1:]))
