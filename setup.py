import re
import os

from setuptools import setup, find_packages

with open(os.path.join('bin', 'fleece.py')) as v_file:
    version = re.compile(r".*__version__ = '(.*?)'",
                         re.S).match(v_file.read())

requires = []

setup(name='fleece',
      version=version,
      description='stdin stream UDP jsonifier',
      classifiers=["Programming Language :: Python",
                   "Programming Language :: Python :: 2.6",
                   "Programming Language :: Python :: 2.7",
                   "Topic :: System :: Logging",
                   "License :: Other/Proprietary License",
                   "Intended Audience :: System Administrators"],
      author='Aurelien ROUGEMONT',
      author_email='beorn@gandi.net',
      packages=find_packages(),
      include_package_data=True,
      zip_safe=False,
      install_requires=requires,
      url='http://www.gandi.net/',
      license='Proprietary',
      )

