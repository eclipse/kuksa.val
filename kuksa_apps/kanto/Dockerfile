#########################################################################
# Copyright (c) 2022 Bosch.IO GmbH
# Copyright (c) 2022 Contributors to the Eclipse Foundation
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

FROM python:3.9-slim

COPY ditto-clients-python ./ditto-clients-python
RUN pip3 install ./ditto-clients-python


COPY requirements.txt ./
RUN pip3 install --no-cache-dir -r requirements.txt

COPY ./*.py ./
ADD config ./config

CMD [ "python3", "./edge_client.py" ]
