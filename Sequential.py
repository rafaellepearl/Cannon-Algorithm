def print_matrix(matrix, name):
    """Utility function to print a matrix in a readable format."""
    print(f"Matrix {name}:")
    for row in matrix:
        print("  " + " ".join(f"{val:3}" for val in row))
    print()

def cannons_algorithm(A, B):
    """
    Simulates Cannon's algorithm for matrix multiplication.
    Assumes A and B are n-by-n square matrices.
    """
    n = len(A)
    # Initialize the result matrix C with zeros
    C = [[0] * n for _ in range(n)]

    # Make copies of A and B so we don't modify the original inputs
    A_curr = [row[:] for row in A]
    B_curr = [row[:] for row in B]

    print("--- 1. Initial Alignment ---")
    # Shift row 'i' of A to the left by 'i' positions
    for i in range(n):
        A_curr[i] = A_curr[i][i:] + A_curr[i][:i]

    # Shift column 'j' of B upwards by 'j' positions
    for j in range(n):
        # Extract the column
        col = [B_curr[k][j] for k in range(n)]
        # Shift the column up
        col_shifted = col[j:] + col[:j]
        # Put the column back into the matrix
        for k in range(n):
            B_curr[k][j] = col_shifted[k]
            
    print_matrix(A_curr, "A (After Initial Shift)")
    print_matrix(B_curr, "B (After Initial Shift)")

    print("--- 2. Multiplication and Shifting Steps ---")
    # Perform n steps of multiplication and shifting
    for step in range(n):
        # Multiply local elements and add to C
        for i in range(n):
            for j in range(n):
                C[i][j] += A_curr[i][j] * B_curr[i][j]
        
        print_matrix(C, f"C (Accumulated after Step {step + 1})")

        # Shift A left by 1 position for the next step
        for i in range(n):
            A_curr[i] = A_curr[i][1:] + A_curr[i][:1]

        # Shift B upwards by 1 position for the next step
        for j in range(n):
            col = [B_curr[k][j] for k in range(n)]
            col_shifted = col[1:] + col[:1]
            for k in range(n):
                B_curr[k][j] = col_shifted[k]

    return C

# ==========================================
# Hardcoded Example
# ==========================================
if __name__ == "__main__":
    # Define two 3x3 matrices
    matrix_A = [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9]
    ]

    matrix_B = [
        [9, 8, 7],
        [6, 8, 4],
        [3, 2, 1]
    ]

    print("=== Input Matrices ===")
    print_matrix(matrix_A, "A")
    print_matrix(matrix_B, "B")

    print("=== Executing Cannon's Algorithm ===")
    result_matrix = cannons_algorithm(matrix_A, matrix_B)

    print("=== Final Result ===")
    print_matrix(result_matrix, "C (Result of A x B)")