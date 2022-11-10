import kuksa_client
import config

class Client:

    def start(self):
        clientType = self.cfg.get("protocol")
        if clientType == 'ws':
            print('Starting VISS client')
            self.connect()
            self.commThread.authorize()
        elif clientType == 'grpc':
            print('Starting gRPC client')
            self.connect()
        else: 
            raise ValueError(f"Unknown client type {clientType}")

    def connect(self):
        self.commThread = kuksa_client.KuksaClientThread(self.cfg)
        self.commThread.start()

    def __init__(self, file):
        self.cfg = config.getConfig(file)
        self.commThread = None
