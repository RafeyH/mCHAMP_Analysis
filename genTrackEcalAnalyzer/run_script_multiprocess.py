import yaml
import subprocess
import os
import multiprocessing

def run_cmsRun(key, file_list, output_dir):
    """Run cmsRun on a list of files for a given key."""
    output_file = os.path.join(output_dir, f"{key}.root")

    # Skip if output file already exists
    if os.path.exists(output_file):
        print(f"Skipping {key}, output already exists: {output_file}")
        return

    for file_path in file_list:
        cmsRun_command = [
            'cmsRun', 'GenTrackEcalAnalyzer/python/ConfFile_cfg.py',
            'inputFiles=' + file_path,
            'outputFile=' + output_file
        ]

        try:
            print(f"Running cmsRun for {key} with file {file_path}...")
            subprocess.run(cmsRun_command, check=True)
            print(f"Output saved to {output_file}")
            return  # Exit loop if successful
        except subprocess.CalledProcessError as e:
            print(f"Error running cmsRun for {key} with file {file_path}: {e}")
    
    print(f"All files failed for {key}, moving to the next dataset.")

# Configurations
yaml_file = 'AOD_datasets_allFiles.yaml'
output_dir = 'AOD_trackSel_wDedx_wEB'
os.makedirs(output_dir, exist_ok=True)

# Load YAML data
with open(yaml_file, 'r') as file:
    data = yaml.safe_load(file)

# Setup multiprocessing pool
pool = multiprocessing.Pool(processes=multiprocessing.cpu_count())

for key, content in data.items():
    file_list = list(content['files'].keys())  # Get all files for the dataset
    if file_list:
        pool.apply_async(run_cmsRun, args=(key, file_list, output_dir))

pool.close()
pool.join()

print("All processes completed.")

