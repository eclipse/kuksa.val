import setuptools

__packages__=setuptools.find_packages()
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
    setup_requires=['setuptools-git-versioning'],
    packages=__packages__,
)
