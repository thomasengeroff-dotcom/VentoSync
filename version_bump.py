import json
import os
import shutil
import hashlib
# This script is executed by PlatformIO (SCons) during the build process.
# It automatically increments the patch version in version.json and
# injects the version string as a C++ macro.

try:
    from SCons.Script import Import
    Import("env")
except ImportError:
    pass

# In PlatformIO/SCons, __file__ is not defined. 
# Attempt to find the project root relative to the build context.
if 'env' in globals():
    project_dir = env.get("PROJECT_DIR", os.getcwd())
else:
    project_dir = os.getcwd()

version_file = os.path.join(project_dir, "version.json")

# Fallback: If not found, try common relative paths
if not os.path.exists(version_file):
    # Check if we are in a subdirectory like .esphome/build/xxx
    potential_root = os.path.abspath(os.path.join(project_dir, "..", "..", ".."))
    if os.path.exists(os.path.join(potential_root, "version.json")):
        project_dir = potential_root
        version_file = os.path.join(project_dir, "version.json")

def bump_version():
    if not os.path.exists(version_file):
        data = {"version": "0.8.0", "date": "2026-04-04"}
    else:
        with open(version_file, "r") as f:
            try:
                data = json.load(f)
            except Exception:
                data = {"version": "0.8.0", "date": "2026-04-04"}

    # Handle the new "version": "x.y.z" format
    version_str = data.get("version", "0.8.0")
    parts = version_str.split('.')
    if len(parts) == 3:
        major, minor, patch = map(int, parts)
        patch += 1
        new_version_str = f"{major}.{minor}.{patch}"
    else:
        # Fallback for old format or malformed string
        major = data.get("major", 0)
        minor = data.get("minor", 8)
        patch = data.get("patch", 0) + 1
        new_version_str = f"{major}.{minor}.{patch}"

    data["version"] = new_version_str
    # Remove old keys if they exist
    data.pop("major", None)
    data.pop("minor", None)
    data.pop("patch", None)
    
    # Save updated version
    with open(version_file, "w") as f:
        json.dump(data, f, indent=4)
    
    # Inject as a C++ macro: -DCUSTOM_PROJECT_VERSION="0.8.31"
    if 'env' in globals():
        env.Append(CPPDEFINES=[
            ("CUSTOM_PROJECT_VERSION", f'\\"{new_version_str}\\"')
        ])
    
    print(f"\n>>> AUTOMATED VERSION BUMP: {new_version_str} <<<\n")
    return new_version_str

def update_yaml_version(version_str):
    # This function finds the project_version substitution in the common YAML 
    # and updates it to match the new version.
    yaml_file = os.path.join(project_dir, "packages", "esp32c6_common.yaml")
    if not os.path.exists(yaml_file):
        # Fallback for different project structures
        yaml_file = os.path.join(project_dir, "esp32c6_common.yaml")
    
    if os.path.exists(yaml_file):
        with open(yaml_file, "r") as f:
            lines = f.readlines()
        
        with open(yaml_file, "w") as f:
            for line in lines:
                if "project_version:" in line:
                    # preserves indentation
                    indent = line[:line.find("project_version:")]
                    f.write(f'{indent}project_version: "{version_str}"\n')
                else:
                    f.write(line)
        print(f">>> AUTOMATED YAML UPDATE: {yaml_file} set to {version_str}")

if __name__ == "__main__" or 'env' in globals():
    new_version_str = bump_version()
    update_yaml_version(new_version_str)

# --- POST BUILD ACTION ---
# This runs after the firmware.bin is compiled
def after_build(source, target, env):
    try:
        # get the path to the compiled firmware
        firmware_path = str(target[0])
        print(f"\n>>> POST-BUILD: Firmware found at {firmware_path}")
        
        # Calculate MD5
        with open(firmware_path, "rb") as f:
            md5_hash = hashlib.md5(f.read()).hexdigest()
            
        print(f">>> POST-BUILD: MD5 Hash = {md5_hash}")

        # Get the new version from version.json
        version_str = "0.0.0"
        if os.path.exists(version_file):
            with open(version_file, "r") as f:
                data = json.load(f)
                version_str = data.get("version", "0.0.0")

        # Update manifest_example.json
        manifest_path = os.path.join(project_dir, "manifest_example.json")
        if os.path.exists(manifest_path):
            with open(manifest_path, "r") as f:
                manifest_data = json.load(f)
                
            manifest_data["version"] = version_str
            manifest_data["builds"][0]["ota"]["md5"] = md5_hash
            
            with open(manifest_path, "w") as f:
                json.dump(manifest_data, f, indent=2)
                
            print(f">>> POST-BUILD: Updated {manifest_path}")

            # Define Samba target - project specific folder
            project_name = "ventosync"
            smb_dir = os.path.join(r"\\192.168.178.45\config\www\firmware", project_name)
            smb_firmware = os.path.join(smb_dir, "firmware.bin")
            smb_manifest = os.path.join(smb_dir, "manifest.json")
            
            # Try to copy files
            try:
                if not os.path.exists(smb_dir):
                    os.makedirs(smb_dir, exist_ok=True)
                
                print(f">>> POST-BUILD: Copying files to {smb_dir}...")
                shutil.copy2(firmware_path, smb_firmware)
                shutil.copy2(manifest_path, smb_manifest)
                print(f"\n>>> SUCCESSFULLY DEPLOYED: v{version_str} to Home Assistant ({project_name})! <<<\n")
            except Exception as smb_err:
                print(f"\n>>> WARNING: Target directory {smb_dir} not reachable. Copy skipped. Error: {smb_err} <<<\n")
        else:
            print(f"\n>>> WARNING: {manifest_path} not found. <<<\n")
            
    except Exception as e:
        print(f"\n>>> ERROR IN POST-BUILD SCRIPT: {e} <<<\n")

# Attach the post-build action to the SCons environment
if 'env' in globals():
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", after_build)
