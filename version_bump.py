import json
import os
import shutil
import hashlib
# This script is executed by PlatformIO (SCons) during the build process.
# It reads the current version from version.json and injects it as a C++ macro.
#
# IMPORTANT: The version is bumped ONCE per build session using a lock file,
# not on every individual compile. This ensures all firmware variants
# (full, nosensor, radar_only, bme680_only) share the same version number.

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

# Lock file prevents multiple bumps within the same build session.
lock_file = os.path.join(project_dir, ".version_bump_lock")


def read_version():
    """Read the current version from version.json."""
    if not os.path.exists(version_file):
        return {"project_version": "0.8.0", "project_date": "2026-04-04"}

    with open(version_file, "r") as f:
        try:
            return json.load(f)
        except Exception:
            return {"project_version": "0.8.0", "project_date": "2026-04-04"}


def bump_version():
    """Bump the patch version ONCE per build session.
    
    Uses a lock file to prevent multiple bumps when building
    multiple firmware variants (e.g., via upload_all.sh or CI matrix).
    """
    data = read_version()
    current_version = data.get("project_version", "0.8.0")

    # Disable auto-bump in CI environments (GitHub Actions)
    if os.environ.get("GITHUB_ACTIONS") == "true":
        print(f"\n>>> CI ENVIRONMENT DETECTED: Not bumping version, using {current_version} <<<\n")
        inject_version(current_version)
        return current_version

    # Check if we already bumped in this session
    if os.path.exists(lock_file):
        with open(lock_file, "r") as f:
            locked_version = f.read().strip()
        
        if locked_version == current_version:
            print(f"\n>>> VERSION ALREADY BUMPED: {current_version} (lock file active) <<<\n")
            return current_version

    # Perform the actual bump
    parts = current_version.split('.')
    if len(parts) == 3:
        major, minor, patch = map(int, parts)
        patch += 1
        new_version_str = f"{major}.{minor}.{patch}"
    else:
        new_version_str = "0.8.0" # Fallback

    data["project_version"] = new_version_str
    
    # Save updated version
    with open(version_file, "w") as f:
        json.dump(data, f, indent=4)

    # Write lock file to prevent re-bumping for subsequent variants
    with open(lock_file, "w") as f:
        f.write(new_version_str)
    
    # Inject as a C++ macro
    inject_version(new_version_str)
    
    print(f"\n>>> AUTOMATED VERSION BUMP: {new_version_str} <<<\n")
    return new_version_str


def inject_version(version_str):
    """Inject the version as a C++ macro."""
    if 'env' in globals():
        env.Append(CPPDEFINES=[
            ("CUSTOM_PROJECT_VERSION", f'\\"{version_str}\\"')
        ])


if __name__ == "__main__" or 'env' in globals():
    data = read_version()
    current_version = data.get("project_version", "0.8.0")

    # Check lock file
    if os.path.exists(lock_file):
        with open(lock_file, "r") as f:
            locked_version = f.read().strip()
        
        if locked_version == current_version:
            print(f"\n>>> REUSING VERSION: {current_version} (already bumped) <<<\n")
            inject_version(current_version)
        else:
            bump_version()
    else:
        bump_version()


# --- POST BUILD ACTION ---
def after_build(source, target, env):
    try:
        firmware_path = str(target[0])
        print(f"\n>>> POST-BUILD: Firmware found at {firmware_path}")
        
        with open(firmware_path, "rb") as f:
            md5_hash = hashlib.md5(f.read()).hexdigest()
            
        print(f">>> POST-BUILD: MD5 Hash = {md5_hash}")

        version_str = "0.0.0"
        if os.path.exists(version_file):
            with open(version_file, "r") as f:
                data = json.load(f)
                version_str = data.get("project_version", "0.0.0")

        manifest_path = os.path.join(project_dir, "manifest_example.json")
        if os.path.exists(manifest_path):
            with open(manifest_path, "r") as f:
                manifest_data = json.load(f)
                
            manifest_data["version"] = version_str
            manifest_data["builds"][0]["ota"]["md5"] = md5_hash
            
            with open(manifest_path, "w") as f:
                json.dump(manifest_data, f, indent=2)
            print(f">>> POST-BUILD: Updated {manifest_path}")
            
    except Exception as e:
        print(f"\n>>> ERROR IN POST-BUILD SCRIPT: {e} <<<\n")

if 'env' in globals():
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", after_build)
