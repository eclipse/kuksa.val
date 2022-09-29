import distutils.command.build
import setuptools


class BuildCommand(distutils.command.build.build):
    def run(self):
        from grpc_tools import command
        command.build_package_protos('.')
        super().run()


setuptools.setup(
    setuptools_git_versioning={
        "template": "{tag}",
        "dev_template": "{tag}-{ccount}",
        "dirty_template": "{tag}-{ccount}",
        "starting_version": "0.1.6",
        "version_callback": None,
        "version_file": None,
        "count_commits_from_version_file": False
    },
    package_data={ "kuksa_client": ["logo"], "kuksa_certificates": ["*", "jwt/*"]},
    setup_requires=['setuptools-git-versioning', 'grpcio', 'grpcio-tools'],
    cmdclass={
    'build': BuildCommand,
    }
)
