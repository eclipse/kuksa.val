#!/usr/bin/env python3
########################################################################
# Copyright (c) 2022,2023 Robert Bosch GmbH
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


class RunCargoException(Exception):
    pass


def extract_license_ids(license):
    """Extract valid licenses for each dependency. We most of the time
    do not care whether it is "AND" or "OR" currently, we currently assume we are
    compatible to all "OR" variants, and thus include all"""
    license_ids = []
    if license:
        license_ids = re.split(r"\s*AND\s*|\s*OR\s*|\(|\)", license)
        license_ids = list(filter(None, license_ids))

    return license_ids


def extract_license_filenames(crate):
    license_files = []
    crate = quirks.apply_quirks(crate)

    crate_name = crate["name"]
    license_ids = extract_license_ids(crate["license"])
    license_file = crate["license_file"]

    if not license_ids and not license_file:
        raise LicenseException(
            f"Neither license nor license file specified for {crate_name}"
        )

    if license_file:
        license_ids.append(crate_name)

    missing = []
    for license_id in license_ids:
        if license_id in supported_licenses:
            license_file = supported_licenses[license_id]
            license_files.append(license_file)
        else:
            missing.append(license_id)

    if missing:
        missing_licenses = ", ".join(missing)
        raise LicenseException(
            f"Could not find license file for {missing_licenses} in {crate_name}"
        )

    return license_files


def generate_bom(source_path, target_path, dashout):
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
        raise RunCargoException(f"Error running cargo license: {e}")

    crates = json.loads(cargo_output)
    dashlist = []

    # Cargo will also pick up our own dependencies. As they are not thirdparty
    # creating a new list without our own packages
    crates = [
        crate for crate in crates if not crate["name"].startswith("databroker") and not crate["name"].startswith("sdk")]

    license_files = set()
    errors = []
    for crate in crates:
        try:
            print(f"License for {crate['name']} {crate['version']}: ", end="")
            license_filenames = extract_license_filenames(crate)
            for license_filename in license_filenames:
                license_files.add(license_filename)
            unpacked_filenames = [
                filename[:-3] if filename.endswith(".gz") else filename
                for filename in license_filenames
            ]
            print(" ".join(unpacked_filenames))
            del crate["license_file"]
            crate["license_files"] = unpacked_filenames
            dashlist.append(
                f"crate/cratesio/-/{crate['name']}/{crate['version']}")
        except LicenseException as e:
            errors.append(e)

    if errors:
        for error in errors:
            print(error)
        raise LicenseException(
            "BOM creation failed, unresolved licenses detected")

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

    if dashout is not None:
        print(f"Exporting dash output to {dashout}")
        with open(dashout, 'w') as f:
            for line in dashlist:
                f.write(f"{line}\n")


def main(args=None):
    parser = argparse.ArgumentParser()
    parser.add_argument("dir", help="Rust project directory")
    parser.add_argument("--dash", default=None, type=str,
                        help="if present, write an input file for dash PATH", metavar="PATH")
    args = parser.parse_args(args)

    source_path = os.path.abspath(args.dir)
    target_path = os.path.join(source_path, "thirdparty")

    if os.path.exists(target_path):
        print(
            f"Folder {target_path} already exists. Remove it before running this script."
        )
        return -2

    if args.dash is not None and os.path.exists(args.dash):
        print(
            f"Requested Dash output file {args.dash} exists. Remove it before running this script.")
        return -3

    print(f"Generating BOM for project in {source_path}")
    try:
        generate_bom(source_path, target_path, args.dash)
    except LicenseException as e:
        print(f"Error: {e}")
        return -100
    except RunCargoException as e:
        print(f"Error: {e}")
        return -1


if __name__ == "__main__":

    sys.exit(main(sys.argv[1:]))
