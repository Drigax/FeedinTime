Import("env")
import os

site_root_path = "FeedinTime-Site"

def before_build_spiffs(source, target, env):
    print("Building React App...")
    env.Execute(f'cd {site_root_path} && npm run build')
    print("React App built!")

    print("Removing old SPIFFS image...")
    env.Execute("rm -rf data")

    print("Copying React App to SPIFFS...")
    env.Execute(f'cp -r {site_root_path}/build data')

    print ("Moving site /static assets to /data root to reduce filepath...")
    env.Execute(f'rm -rf data/*/*.map data/*/*.LICENSE.txt')
    env.Execute(f'mv data/static/* data/ && rmdir data/static')

env.AddPreAction("$BUILD_DIR/spiffs.bin", before_build_spiffs)