import os
from multiprocessing import Pool, cpu_count

# File path for the template
template_file_path = 'dataTemplate_crabNtuple.py'

# EG datasets
eg_datasets = [
    '/EGamma/Run2018A-15Feb2022_UL2018-v1/AOD',
    '/EGamma/Run2018B-15Feb2022_UL2018-v1/AOD',
    '/EGamma/Run2018C-15Feb2022_UL2018-v1/AOD',
    '/EGamma/Run2018D-15Feb2022_UL2018-v1/AOD'
]


# Ensure output directory exists
os.makedirs('crabScripts', exist_ok=True)

# Read the template once globally
with open(template_file_path, 'r') as file:
    template_content = file.read()

# Function to submit a job for one dataset
def process_dataset(dataset):
    
    data_type = dataset.split('/')[1]
    data_year = dataset.split('/')[2].split('-')[0]
    
    modified_content = (
        template_content
        .replace('WORKFLOW_MODE', 'DATA')
        .replace('DATA_TYPE_I', data_type)
        .replace('DATA_YEAR_I', data_year)
        .replace('DATASET_I', dataset)
    )

    filename = f'crabScripts/crabNtuple_DATA_{data_type}_{data_year}.py'
    with open(filename, 'w') as f:
        f.write(modified_content)
    os.system(f'crab submit -c {filename}')
    return f"Submitted: {data_type} {data_year}"

if __name__ == '__main__':
    
    bkg_datasets = eg_datasets
    pool = Pool(processes=min(len(bkg_datasets), cpu_count()))

    results = []
    for dataset in bkg_datasets:
        res = pool.apply_async(process_dataset, args=(dataset,))
        results.append(res)

    for r in results:
        print(r.get())

    pool.close()
    pool.join()

