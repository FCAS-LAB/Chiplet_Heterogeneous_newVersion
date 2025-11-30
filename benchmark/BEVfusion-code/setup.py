# 分别进入camera_backbone、camera_vtransform、lidar_backbone、fuser、head的build目录，执行make命令

import os
import subprocess

# 设置SIMULATOR_ROOT环境变量
SIMULATOR_ROOT = os.environ.get('SIMULATOR_ROOT')

# 分别进入camera_backbone、camera_vtransform、lidar_backbone、fuser、head的build目录，执行make命令
for module in ['camera_backbone', 'camera_vtransform', 'lidar_backbone', 'fuser', 'head', 'main']:
    # 如果没有build目录，则创建build目录
    build_dir = os.path.join(SIMULATOR_ROOT, 'benchmark', 'BEVfusion-code', module, 'build')
    if not os.path.exists(build_dir):
        os.makedirs(build_dir)
    os.chdir(build_dir)
    # 执行cmake
    subprocess.run(['cmake', '..'])
    # 执行make
    subprocess.run(['make'])
    print(f"{module} 编译完成")