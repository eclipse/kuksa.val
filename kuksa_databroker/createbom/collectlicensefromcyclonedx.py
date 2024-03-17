#!/usr/bin/env python3
########################################################################
# Copyright (c) 2024 Robert Bosch GmbH
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
This script will parse a CycloneDX SBOM and collect all mentioned
license texts to be included in a distribution
"""

import json
import argparse
import sys
import gzip
import os
import re
import logging

import yaml

from bomutil.maplicensefile import MAP as supported_licenses

logger = logging.getLogger(__name__)

COMP_CURATION = {}
EXPRESSION_CURATION = {}


def check_component_curation(component):
    """Component curations define a license expression for a given package.
    This does NOT overwrite what is in the SBOM, it is merely used to detect
    which license to copy int the deliverable"""
    logger.debug("Checking component curation for %s", component['name'])
    if component['name'] in COMP_CURATION:
        logger.info("Found component curation for %s: %s", component['name'], COMP_CURATION[component['name']])
        return COMP_CURATION[component['name']]['expression']
    return None


def check_expression_curation(license_exp):
    """Can be used to apply general rules for license, i.e. if a license is A OR B, you may opt to use A only"""
    logger.debug("Checking expression curation for %s", license_exp)
    if license_exp in EXPRESSION_CURATION:
        logger.info("Found expression curation for  %s: %s", license_exp, EXPRESSION_CURATION[license_exp])
        return EXPRESSION_CURATION[license_exp]
    return license_exp


def collect_license_id_from_expression(license_exp, component, license_ids):
    ids = re.split(r"\s*AND\s*|\s*OR\s*|\(|\)", license_exp)
    ids = list(filter(None, ids))
    logger.debug("%s license info is %s", component['name'], ids)
    for lic_id in ids:
        if lic_id not in license_ids:
            license_ids.append(lic_id)


def get_expression_from_component(component):
    try:
        license_exp = component['licenses'][0]['expression']
        return license_exp
    except KeyError:
        logger.warning("Not able to determine license expression for %s", component['name'])
        return None


def collect(sbom, targetpath):
    logger.info("Collecting licenses from %s, exporting license texts to %s", sbom, targetpath)

    # Opening JSON file
    f = open(sbom, encoding="utf-8", mode="r")
    sbom = json.load(f)

    license_ids = []

    for component in sbom["components"]:
        license_exp = check_component_curation(component)
        if license_exp is None:
            license_exp = get_expression_from_component(component)

        license_exp = check_expression_curation(license_exp)

        # If we do not have a license expression, we cannot do anything
        if license_exp is None:
            logger.error("Could not find license expression for %s", component['name'])
            logger.error("Consider adding a curation for this component")
            sys.exit(-1)

        logger.info("License expression for %s is %s",  component['name'],  license_exp)

        collect_license_id_from_expression(license_exp, component, license_ids)

    logger.debug("All license ids %s:", license_ids)

    logger.info("Exporting relevant license texts to %s", targetpath)
    # Exporting
    os.mkdir(targetpath)

    for license_id in license_ids:
        if license_id not in supported_licenses:
            logger.error("License %s not supported. You need to extend this tools or check curations", license_id)
            logger.error("This tool currently supports the following license identifiers: %s",
                         supported_licenses.keys())
            sys.exit(-3)

        logger.info("Copying %s", supported_licenses[license_id][:-3])
        with gzip.open("licensestore/" + supported_licenses[license_id], "rb") as inf:
            content = inf.read()
        with open(os.path.join(targetpath, supported_licenses[license_id][:-3]), "wb") as outf:
            outf.write(content)


def main(args=None):
    parser = argparse.ArgumentParser()
    parser.add_argument("sbom", type=str, help="CycloneDX SBOM in JSON format")
    parser.add_argument("dir", default="./", type=str, help="Output directory")
    parser.add_argument("--curation", type=str, help="Curation file", default=None)
    parser.add_argument('--log-level', choices=['DEBUG', 'INFO', 'WARNING', 'ERROR', 'CRITICAL'],
                        default='INFO', help='Set the log level')
    args = parser.parse_args(args)

    logging.basicConfig(format='%(asctime)s %(levelname)s: %(message)s',
                        datefmt='%Y-%m-%dT%H:%M:%S%z', level=args.log_level)
    if not os.path.exists(args.sbom):
        logger.error("Input SBOM %s does not exist", args.sbom)
        return -1

    # We want to start with an empty folder, making sure it only contains license
    # texts extracted form the input sbom
    if os.path.exists(args.dir):
        logger.error(
            "Target folder %s already exists. Remove it before running this script.", args.dir
        )
        return -2

    if "curation" in args and args.curation is not None:
        logger.info("Using curation file %s", args.curation)
        with open(args.curation, encoding="utf-8", mode="r") as f:
            cur = yaml.safe_load(f)
            if "components" in cur:
                logger.info("Adding %s component curations", len(cur['components'].keys()))
                logger.debug(cur['components'].keys())
                global COMP_CURATION
                COMP_CURATION = cur["components"]
            if "expressions" in cur:
                logger.info("Adding %s expression curations", len(cur['expressions'].keys()))
                global EXPRESSION_CURATION
                EXPRESSION_CURATION = cur["expressions"]
    else:
        logger.info("No curation file specified.")

    collect(args.sbom, args.dir)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
