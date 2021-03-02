import setuptools

with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

setuptools.setup(
    name="kuksa_val_client",
    version_config=True,
    setup_requires=['setuptools-git-versioning'],
    author="Wenwen Chen",
    author_email="wenwen.chen@de.bosch.com",
    description="Libraries to implement a kuksa.val client",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/eclipse/kuksa.val",
    project_urls={
        "Bug Tracker": "https://github.com/eclipse/kuksa.val/issues",
    },
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: EPL-2.0 License",
        "Operating System :: OS Independent",
    ],
    packages=setuptools.find_packages(),
    python_requires=">=3.6",
)
