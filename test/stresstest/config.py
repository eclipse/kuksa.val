import configparser
import pathlib

def getConfig(cfg_path = 'config.ini'):

    config = configparser.ConfigParser()
    config_path = pathlib.Path(cfg_path)
    config.read(config_path)

    vsscfg = config['vss']      #get data from config file

    return vsscfg