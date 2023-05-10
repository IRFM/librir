import os
import re
import sys

from setuptools import setup, find_packages


def find_version():
    filename = "CMakeLists.txt"
    with open(filename, "r") as f:
        txt = f.read()

    start = txt.find("project(")
    end = txt[start:].find(")")
    sub = txt[start : start + end + 1]
    m = re.search(r"VERSION.*(?P<version>\d+\.\d+\.\d+).*", sub)
    if m:
        d = m.groupdict()
        return d["version"]
    return ""
    # mm = re.search(r"(?P<major>\d+)\.(?P<minor>\d+)\.(?P<patch>\d+)", d['version'])
    # if mm:
    # mm.groupdict()


def get_requires(path):
    try:
        res = []
        with open(path, "r") as file:
            # reading each line
            for line in file:
                # reading each word
                for word in line.split():
                    res.append(word)
    except:
        pass
    return res


def get_extra_requires(path, add_all=True):
    """
    https://hanxiao.io/2019/11/07/A-Better-Practice-for-Managing-extras-require-Dependencies-in-Python/

    @param path:
    @param add_all:
    @return:
    """
    from collections import defaultdict

    with open(path) as fp:
        extra_deps = defaultdict(set)
        for k in fp:
            if k.strip() and not k.startswith("#"):
                tags = set()
                if ":" in k:
                    k, v = k.split(":")
                    tags.update(vv.strip() for vv in v.split(","))
                for t in tags:
                    extra_deps[t].add(k)

        # add tag `all` at the end
        if add_all:
            extra_deps["all"] = set(vv for v in extra_deps.values() for vv in v)

    return extra_deps


def package_files(directory):
    print(directory)
    paths = []
    for (path, directories, filenames) in os.walk(directory):
        for filename in filenames:
            paths.append(os.path.join(".", path, filename))
            paths[-1] = paths[-1].replace("\\", "/")
    return paths


licenses_files = package_files("librir/LICENSES")
libs_files = package_files("librir/libs")
masks_files = package_files("librir/masks")
print(licenses_files)


DYNAMIC_LIBRARY_SUFFIX = "dll" if sys.platform == "win32" else "so"

EXCLUDED_PACKAGES = []
PACKAGES = find_packages(exclude=EXCLUDED_PACKAGES)

LIB_GLOBPATH = f"libs/*.{DYNAMIC_LIBRARY_SUFFIX}*"
# add masks
MASK_GLOBPATH = "masks/*.txt*"

setup(
    name="librir",
    version=find_version(),
    packages=PACKAGES,
    package_data={"librir": [LIB_GLOBPATH, MASK_GLOBPATH]},
    data_files=[
        ("librir-core/LICENSES", licenses_files),
        ("librir-core/masks", masks_files),
        ("librir-core", ["./librir/LICENSE"]),
    ],
    include_package_data=True,
    install_requires=["numpy", "pandas", "future-annotations;python_version<'3.7'",]
    + get_requires("requirements.txt"),
    # dependency_links=[
    #     # location to your egg file
    #     "git+http://irfm-gitlab.intra.cea.fr/exploitation/data-access/pywed"
    #     # f"file://{os.path.join(os.getcwd(), '3rd_64', 'dist_PyWED-0.3.1.tar.gz')}#egg=PyWED-0.3.1",
    # ],
    extras_require=get_extra_requires("extra-requirements.txt"),
    # extras_require={'lab': ['pandas', 'matplotlib', 'opencv-python', 'pymysql==0.10.1'],
    #                 'west': ['pymysql'],
    #                 'test': ['pytest', 'chardet'],
    #                 'full': ['pandas', 'matplotlib', 'opencv-python', 'pytest', 'chardet', 'pymysql'],
    #                 'dev': ['pandas', 'matplotlib', 'opencv-python', 'pytest', 'chardet', 'pymysql']},
    python_requires=">=3.7",
    url="",
    license="CEA",
    author="VM213788, LD243615, EG264877",
    author_email="victor.moncada@cea.fr, leo.dubus@cea.fr, erwan.grelier@cea.fr",
    description="Librir is a C/C++/Python library to manipulate infrared video from the WEST tokamak, accessing WEST diagnostic signals and building Cognitive Vision/Machine Learning applications.",
    long_description=open("README.md", "rt").read(),
    long_description_content_type="text/markdown",
)
