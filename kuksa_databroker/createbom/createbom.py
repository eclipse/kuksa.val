#! /usr/bin/env python
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

# This will take a target folder with a RUST project as argument
# It will generate a list of all dependencies and licenses and 
# populate a folder thirdparty with all licenses

import argparse
import sys
import json
import re
import os
import gzip

from subprocess import check_output

import maplicensefile
import quirks

def extract_license_list(license_list, component):
    # We do not care whether it is "AND" or "OR" currently, we currently assume we are
    # compatible to all "OR" variants, and thus include all
    component = quirks.apply_quirks(component)
    licenses = re.split(r'\s*AND\s*|\s*OR\s*|\(|\)', component["license"])
    licenses = list(filter(None, licenses))
    print(f"Licenses for {component['name']}: {licenses}")

    del component['license_file']
    
    for license in licenses:
        if license not in maplicensefile.MAP:
            print(f"BOM creation failed, can not find license text for {license} used in dependency {component['name']}")
            sys.exit(-100)
        if maplicensefile.MAP[license] not in license_list:
            license_list.append(maplicensefile.MAP[license])

parser = argparse.ArgumentParser()
parser.add_argument("dir", help="Rust project directory")
args = parser.parse_args()

fullpath : os.path = os.path.abspath(args.dir)
print(f"Generating BOM for project in {fullpath}")

try:
    cargo_output=check_output(["cargo", "license", "--json", "--avoid-build-deps", "--current-dir", fullpath])
except Exception as e:
    print(f"Error running cargo license: {e}")
    sys.exit(-1)


licensedict=json.loads(cargo_output)

license_list = []
for entry in licensedict:
    extract_license_list(license_list,entry)

# Exporting
print(f"Path {fullpath} Will be {os.path.join(fullpath,'thirdparty')}")
os.mkdir(os.path.join(fullpath,"thirdparty"))

for license in license_list:
    print(f"Copying {license[:-2]}")
    with gzip.open("licensestore/"+license, 'rb') as inf:
        content = inf.read()
    with open(os.path.join(fullpath,"thirdparty/"+license[:-3]),'wb') as outf:
        outf.write(content)

with open(os.path.join(fullpath,"thirdparty/thirdparty_components.txt"),'w') as jsonout:
    json.dump(licensedict,jsonout, indent=4)





