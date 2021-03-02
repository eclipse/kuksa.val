import setuptools

with open("kuksa_client/README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

setuptools.setup(
    name="kuksa_client",
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
    author="Sebastian Schildt, Naresh Nayak, Wenwen Chen",
    author_email="sebastian.schildt@de.bosch.com, naresh.nayak@de.bosch.com, wenwen.chen@de.bosch.com",
    description="kuksa.val python client SDK",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/eclipse/kuksa.val",
    project_urls={
        "Bug Tracker": "https://github.com/eclipse/kuksa.val/issues",
    },
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Environment :: Console",
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: Eclipse Public License 2.0 (EPL-2.0)",
        "Operating System :: OS Independent",
        "Topic :: Software Development"
    ],
    packages=setuptools.find_packages(),
    python_requires=">=3.6",
    install_requires= ['cmd2']
)
