import pandas as pd
from datetime import datetime, timedelta

def expand_filenames(frequency, base_name, start_date, end_date):
    parts = base_name.split('.')
    if len(parts) < 3:
        raise ValueError("base_name should have at least 3 parts")
    
    *dirs, identifier = parts
    directory_path = "/".join(dirs)

    start = datetime.strptime(start_date, "%Y%m%d")
    end = datetime.strptime(end_date, "%Y%m%d")

    if frequency != 'Daily':
        raise ValueError("Only 'Daily' frequency is supported in this example")

    filenames = []
    current_date = start
    while current_date <= end:
        date_str = current_date.strftime("%Y%m%d")
        filename = f"{directory_path}/{identifier}/{identifier}_{date_str}.pqt"
        filenames.append(filename)
        current_date += timedelta(days=1)
    
    return filenames

def load(frequency, base_name, start_date, end_date):
    """
    Load data from files for each date in the specified range.

    Parameters:
    - frequency (str): Frequency of the files (e.g., 'Daily').
    - base_name (str): Base name in a dot-separated format.
    - start_date (str): Start date in 'YYYYMMDD' format.
    - end_date (str): End date in 'YYYYMMDD' format.

    Returns:
    - list: List of data loaded from each file.
    """
    filenames = expand_filenames(frequency, base_name, start_date, end_date)
    data = []
    
    for filename in filenames:
        try:
            print(f"Loading data from {filename}")
            df = pd.read_parquet(filename)
            data.append(df)
        except FileNotFoundError:
            print(f"File not found: {filename}")
        except Exception as e:
            print(f"Error loading {filename}: {e}")
    
    return data

def store(data_list, frequency, base_name, start_date, end_date):
    """
    Store data to files for each date in the specified range.

    Parameters:
    - data_list (list of pd.DataFrame): List of dataframes to store, one for each date.
    - frequency (str): Frequency of the files (e.g., 'Daily').
    - base_name (str): Base name in a dot-separated format.
    - start_date (str): Start date in 'YYYYMMDD' format.
    - end_date (str): End date in 'YYYYMMDD' format.
    """
    filenames = expand_filenames(frequency, base_name, start_date, end_date)
    
    if len(data_list) != len(filenames):
        raise ValueError("The length of data_list must match the number of generated filenames.")
    
    for filename, df in zip(filenames, data_list):
        try:
            print(f"Storing data to {filename}")
            df.to_parquet(filename)
        except Exception as e:
            print(f"Error storing {filename}: {e}")

# Example usage:
# Assuming we have data to store for dates '20210101' and '20210102'
data1 = pd.DataFrame({'value': [1, 2, 3]})
data2 = pd.DataFrame({'value': [4, 5, 6]})
data_list = [data1, data2]

# Store data
store(data_list, "Daily", "data.md.other.name", "20210101", "20210102")

# Load data
loaded_data = load("Daily", "data.md.other.name", "20210101", "20210102")
for data in loaded_data:
    print(data)
