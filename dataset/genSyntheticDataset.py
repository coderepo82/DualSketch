import numpy as np
import os


def gen_synthetic_dataset_by_zipf_simple():

    FULL_PATH = "./skewed_dataset_zipf01.txt"

    NUM_RECORDS = 127001730
    MAX_FLOW_ID = 149197297
    MAX_ELEMENT_ID = 87963297

    ALPHA_FLOW = 1.1
    ALPHA_ELEMENT = 1.4

    dir_path = os.path.dirname(FULL_PATH)
    if dir_path and not os.path.exists(dir_path):
        print(f"Directory {dir_path} does not exist. Attempting to create it...")
        try:
            os.makedirs(dir_path, exist_ok=True)
        except OSError as e:
            print(
                f"ERROR: Could not create directory {dir_path}. Please check the path/permissions. Skipping generation.")
            print(f"Error details: {e}")
            return

    print("--- Starting Synthetic Dataset Generation (Python) ---")
    print(f"Output File: {FULL_PATH}")
    print(f"Total Records: {NUM_RECORDS}")
    print(f"Flow Alpha: {ALPHA_FLOW} | Element Alpha: {ALPHA_ELEMENT}")

    flow_rank = np.random.zipf(a=ALPHA_FLOW, size=NUM_RECORDS)
    element_rank = np.random.zipf(a=ALPHA_ELEMENT, size=NUM_RECORDS)

    flow_ids = np.clip(flow_rank, 1, MAX_FLOW_ID).astype(np.uint32)
    element_ids = np.clip(element_rank, 1, MAX_ELEMENT_ID).astype(np.uint32)

    data = np.stack((flow_ids, element_ids), axis=1)

    np.savetxt(FULL_PATH, data, fmt='%u', delimiter=' ')

    print("--- Data Generation Complete ---")
    print(f"File saved to {FULL_PATH}")


if __name__ == "__main__":
    gen_synthetic_dataset_by_zipf_simple()