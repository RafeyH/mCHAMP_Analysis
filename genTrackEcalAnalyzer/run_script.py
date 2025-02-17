import yaml
import subprocess
import os

# Load YAML entries
yaml_file = 'AOD_datasets_allFiles.yaml'
output_dir = 'AOD_trackSel_wDedx_wEB'
os.makedirs(output_dir, exist_ok=True)

with open(yaml_file, 'r') as file:
    data = yaml.safe_load(file)

# Loop over each entry in the YAML file
for key, content in data.items():
    file_path = list(content['files'].keys())[0]
    output_file = os.path.join(output_dir, f"{key}.root")

    cmsRun_command = [
        'cmsRun', 'GenTrackEcalAnalyzer/python/ConfFile_cfg.py',
        'inputFiles=' + file_path,
        'outputFile=' + output_file
    ]

    try:
        print(f"Running cmsRun for {key} with file {file_path}...")
        subprocess.run(cmsRun_command, check=True)
        print(f"Output saved to {output_file}")
    except subprocess.CalledProcessError as e:
        print(f"Error running cmsRun for {key}: {e}")

