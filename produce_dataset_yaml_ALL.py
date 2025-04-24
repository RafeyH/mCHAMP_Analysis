import os
import subprocess
import yaml
import json

def get_files_from_dasgoclient(dataset):
    # Query DAS for files and number of events in JSON format
    das_query = f'dasgoclient -query="file dataset={dataset} | grep file.nevents" -json'
    try:
        result = subprocess.check_output(das_query, shell=True)
    except subprocess.CalledProcessError as e:
        print(f"Error querying DAS for dataset {dataset}: {e}")
        return []

    # Parse JSON and extract all file names and their event counts
    data = json.loads(result)
    files = {}
    for file_info in data:
        file_name = file_info['file'][0]['name']
        num_events = file_info['file'][0]['nevents']
        files[f"root://cms-xrd-global.cern.ch//{file_name}"] = num_events
    
    return files

def generate_yaml_for_mass_charge_combinations(charges, masses, output_file):
    yaml_data = {}
    
    for charge in charges:
        for mass in masses:
            dataset = f"/HSCPmchamp_Q-{charge*3}_M-{mass}_TuneCP2_13TeV_pythia8/RunIISummer20UL18RECO-106X_upgrade2018_realistic_v11_L1v1-v2/AODSIM"
            
            # Query DAS to get all files
            files = get_files_from_dasgoclient(dataset)
            if files:
                key = f"Q{charge*3}_M{mass}"
                yaml_data[key] = {
                    'files': files,
                    'metadata': {
                        'is_mc': True,
                        'charge': charge,
                        'mass': mass
                    }
                }
            else:
                print(f"No files found for dataset: {dataset}")

    # Write the YAML data to the output file
    with open(output_file, 'w') as yaml_file:
        yaml.dump(yaml_data, yaml_file, default_flow_style=False)
    print(f"YAML file '{output_file}' created successfully.")

# Define the range of charges and masses
charges = [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,20,24,30] 
masses = [100,200,300,400,500,600,700,800,900,1000,1200,1400,1600,1800,2000,2400,2800]

# Generate the YAML file with the dataset information
output_yaml_file = "AOD_datasets_allFiles.yaml"
generate_yaml_for_mass_charge_combinations(charges, masses, output_yaml_file)

