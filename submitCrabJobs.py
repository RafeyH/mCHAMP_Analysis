import os
from multiprocessing import Pool, cpu_count

# File path for the template
template_file_path = 'template_crabNtuple.py'

# Charges and masses
charges = [3, 6, 9, 12, 15, 30, 60, 72, 90]
masses = [100, 200, 400, 600, 800, 1200, 1600, 2000, 2800]

# Ensure output directory exists
os.makedirs('crabScripts', exist_ok=True)

# Read the template once globally
with open(template_file_path, 'r') as file:
    template_content = file.read()

# Function to submit a job for one charge-mass combo
def process_combination(charge, mass):
    modified_content = template_content.replace('CHARGE_I', str(charge)).replace('MASS_I', str(mass))
    filename = f'crabScripts/crabNtuple_q{charge}_m{mass}.py'
    with open(filename, 'w') as f:
        f.write(modified_content)
    os.system(f'crab submit -c {filename}')
    return f"Submitted: q={charge}, m={mass}"

if __name__ == '__main__':
    combinations = [(c, m) for c in charges for m in masses]
    pool = Pool(processes=min(len(combinations), cpu_count()))

    # Launch jobs asynchronously
    results = []
    for charge, mass in combinations:
        res = pool.apply_async(process_combination, args=(charge, mass))
        results.append(res)

    # Optional: wait and collect results
    for r in results:
        print(r.get())  # This will block until the job is done and print the status

    pool.close()
    pool.join()

