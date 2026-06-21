import math
import sys

def verify_matrices(file_seq, file_par, tolerance=1e-5):
    print(f"Comparing {file_seq} and {file_par}...")
    
    try:
        with open(file_seq, 'r') as f1, open(file_par, 'r') as f2:
            # Read all content and split it into a list of strings (ignoring spaces/newlines)
            data_seq = f1.read().split()
            data_par = f2.read().split()
    except FileNotFoundError as e:
        print(f"❌ Error: Could not find one of the files. Details: {e}")
        return

    # 1. Check total length
    if len(data_seq) != len(data_par):
        print(f"❌ Dimensional Error: {file_seq} has {len(data_seq)} elements, {file_par} has {len(data_par)}.")
        return

    # 2. Element-by-element comparison with tolerance
    mismatches = 0
    for i, (val1, val2) in enumerate(zip(data_seq, data_par)):
        try:
            v1, v2 = float(val1), float(val2)
            # math.isclose handles tiny differences in parallel floating-point calculations
            if not math.isclose(v1, v2, rel_tol=tolerance, abs_tol=tolerance):
                print(f"Discrepancy at index {i}: Seq = {v1}, Par = {v2}")
                mismatches += 1
                if mismatches >= 5: # Stop after 5 errors to avoid flooding the terminal
                    print("... too many discrepancies, aborting log printing.")
                    break
        except ValueError:
            # If for some reason (e.g. headers) there are non-numeric strings
            if val1 != val2:
                print(f"Textual discrepancy at index {i}: '{val1}' != '{val2}'")
                mismatches += 1

    # 3. Final outcome
    if mismatches == 0:
        print("✅ SUCCESS: The matrices are mathematically identical!")
    else:
        print(f"❌ FAILURE: Found {mismatches} discrepancies between the files.")

if __name__ == "__main__":
    verify_matrices("mul_sequential.txt", "mul_parallel.txt")