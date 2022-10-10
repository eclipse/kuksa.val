import setuptools
from setuptools.command import build
from setuptools.command import build_py
from setuptools.command import sdist


class BuildPackageProtos:
    def run(self):
        from grpc_tools import command
        command.build_package_protos('.')
        super().run()


class BuildCommand(BuildPackageProtos, build.build):
    ...


class BuildPyCommand(BuildPackageProtos, build_py.build_py):
    ...


class SDistCommand(BuildPackageProtos, sdist.sdist):
    ...


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
        'build_py': BuildPyCommand,  # Used for editable installs but also for building wheels
        'sdist': SDistCommand,
    }
)
