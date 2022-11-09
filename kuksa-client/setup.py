import setuptools
try:
    from setuptools.command import build
except ImportError:
    from distutils.command import build  # pylint: disable=deprecated-module
from setuptools.command import build_py
from setuptools.command import sdist


class BuildPackageProtos:
    def run(self):
        from grpc_tools import command  # pylint: disable=import-outside-toplevel
        command.build_package_protos('.')
        super().run()


class BuildCommand(BuildPackageProtos, build.build):
    ...


class BuildPyCommand(BuildPackageProtos, build_py.build_py):  # pylint: disable=too-many-ancestors
    ...


class SDistCommand(BuildPackageProtos, sdist.sdist):
    ...


setuptools.setup(
    cmdclass={
        'build': BuildCommand,
        'build_py': BuildPyCommand,  # Used for editable installs but also for building wheels
        'sdist': SDistCommand,
    }
)
