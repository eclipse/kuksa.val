import subprocess
import os
import config
import signal
import argparse

class Server:
    def build(self):
        os.makedirs(self.build_path,  exist_ok=True)
        cmake_cmd = ['cmake', '..', self.build_path]
        subprocess.check_call(cmake_cmd)
        subprocess.check_call('make', cwd=self.src_path)
    def start(self):
        port = self.cfg.get('port')
        address = self.cfg.get('ip')
        kuksa_val_server_cmd = ['./kuksa-val-server', '--log-level', 'ALL', '--address', address, '--port', port]
        subprocess.check_call(kuksa_val_server_cmd, cwd=self.src_path)
    def __init__(self, config):
        self.cfg = config
        self.build_path = '../../kuksa-val-server/build'
        self.src_path = self.build_path + '/src'

def main():
    parser = argparse.ArgumentParser(description='Run stresstest with specific configuration')
    parser.add_argument('-f', '--file', action='store', default='config.ini',
                    help='The configuration file you want to use')
    args = parser.parse_args()
    cfg = config.getConfig(args.file)
    kuksa_val_server = Server(cfg)
    kuksa_val_server.build()
    kuksa_val_server.start()

if __name__ == '__main__':
    main()
