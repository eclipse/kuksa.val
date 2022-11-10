#! /usr/bin/bash
cd ../../
python3 -m venv testenv
source testenv/bin/activate
cd kuksa-client
pip install -e .
cd ../test/stresstest
gnome-terminal -- python3 server.py
gnome-terminal -- python3 broker.py
gnome-terminal -- python3 subscriber.py
gnome-terminal -- python3 stress.py