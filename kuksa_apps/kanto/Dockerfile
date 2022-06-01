FROM python:3.9-slim

COPY ditto-clients-python ./ditto-clients-python
RUN pip3 install ./ditto-clients-python


COPY requirements.txt ./
RUN pip3 install --no-cache-dir -r requirements.txt

COPY ./*.py ./

CMD [ "python3", "./edge_client.py" ]
