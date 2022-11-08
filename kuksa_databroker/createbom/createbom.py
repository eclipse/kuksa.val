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

'''
Thid will generate a list of all dependencies and licenses of a
Rust project. It will create a folder called thirdparty in that
project folder containing a list of dependencies and a copy
of each license used in dependencies
'''

import argparse
import sys
import json
import re
import os
import gzip

from subprocess import check_output, CalledProcessError

from bomutil import maplicensefile
from bomutil import quirks

def extract_license_list(all_license_list, component):
    '''Extract valid licenses for each dependency. We most of the time
    do not care whether it is "AND" or "OR" currently, we currently assume we are
    compatible to all "OR" variants, and thus include all'''
    component = quirks.apply_quirks(component)
    licenses = re.split(r'\s*AND\s*|\s*OR\s*|\(|\)', component["license"])
    licenses = list(filter(None, licenses))
    print(f"Licenses for {component['name']}: {licenses}")

    del component['license_file']

    for license_id in licenses:
        if license_id not in maplicensefile.MAP:
            print(f"BOM creation failed, can not find license text for {license_id} \
                 used in dependency {component['name']}")
            sys.exit(-100)
        if maplicensefile.MAP[license_id] not in license_list:
            all_license_list.append(maplicensefile.MAP[license_id])

parser = argparse.ArgumentParser()
parser.add_argument("dir", help="Rust project directory")
args = parser.parse_args()

sourcepath = os.path.abspath(args.dir)
targetpath = os.path.join(sourcepath,"thirdparty")

print(f"Generating BOM for project in {sourcepath}")

if os.path.exists(targetpath):
    print(f"Folder {targetpath} already exists. Remove it before running this script.")
    sys.exit(-2)


try:
    cargo_output=check_output(\
        ["cargo", "license", "--json", "--avoid-build-deps", "--current-dir", sourcepath])
except CalledProcessError as e:
    print(f"Error running cargo license: {e}")
    sys.exit(-1)


licensedict=json.loads(cargo_output)

license_list = []
for entry in licensedict:
    extract_license_list(license_list,entry)

# Exporting
os.mkdir(targetpath)

for licensefile in license_list:
    print(f"Copying {licensefile[:-2]}")
    with gzip.open("licensestore/"+licensefile, 'rb') as inf:
        content = inf.read()
    with open(os.path.join(targetpath,licensefile[:-3]),'wb') as outf:
        outf.write(content)

print("Writing thirdparty_components.txt")
with open(os.path.join(targetpath,"thirdparty_components.txt"),'w',encoding="utf-8") as jsonout:
    json.dump(licensedict,jsonout, indent=4)
