Import("env")
import os

site_root_path = "FeedinTime-Site"
on_device_site_root_path = "web"

def before_build_spiffs(source, target, env):
    print("Building React App...")
    env.Execute(f'cd {site_root_path} && npm run build')
    print("React App built!")

    print("Removing old SPIFFS image...")
    env.Execute("rm -rf data")

    print("Copying React App to SPIFFS...")
    env.Execute(f'cp -r {on_device_site_root_path}/build data')    

env.AddPreAction("$BUILD_DIR/spiffs.bin", before_build_spiffs)