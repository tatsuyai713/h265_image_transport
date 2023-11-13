import os
from glob import glob
# ...
setup(
 # ...
 data_files=[
   # ...
   (os.path.join('share', package_name), glob('launch/*_launch.py')),
  ]
 )