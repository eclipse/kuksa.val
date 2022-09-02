import setuptools
import distutils.command.build
from glob import glob
import os

class BuildCommand(distutils.command.build.build):

    def run(self):
        from grpc_tools import command
        command.build_package_protos("./")
        # protobuf for python is just hopelessly broken wrt. imports
        # Read the whole story (and more elgant(?)/heavy-weight) solutions here
        # https://github.com/protocolbuffers/protobuf/issues/1491

        # we will just patch directly
        os.system('sed -i "s/import kuksa_pb2 as kuksa__pb2/from . import kuksa_pb2 as kuksa__pb2/g" kuksa_pb2_grpc.py')
        # Run the original build command
        distutils.command.build.build.run(self)


setuptools.setup(
    version_config={
        "template": "{tag}",
        "dev_template": "{tag}-{ccount}",
        "dirty_template": "{tag}-{ccount}-dirty",
        "starting_version": "0.1.6",
        "version_callback": None,
        "version_file": None,
        "count_commits_from_version_file": False
    },
    package_dir={'kuksa_viss_client': '.' },
    packages=['kuksa_viss_client'],
    data_files=[
        ("kuksa_certificates", glob('../kuksa_certificates/C*')), #Client and CA.pem
        ("kuksa_certificates/jwt", glob('../kuksa_certificates/jwt/*')),
        ],
    package_data={ "kuksa_viss_client": ["logo"] },
    setup_requires=['setuptools-git-versioning', 'grpcio', 'grpcio-tools'],
    cmdclass={
    'build': BuildCommand,
    }
)
