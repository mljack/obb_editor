def z_order_to_2d(z: int, m: int, n: int) -> tuple[int, int]:
    """
    Convert a Z-order linear index back to 2D (i, j) coordinates for an m×n array.
    
    Args:
        z: Z-order linear index
        m: Number of rows in the 2D array
        n: Number of columns in the 2D array
    
    Returns:
        (i, j): 2D coordinates corresponding to the Z-order index
    
    Raises:
        IndexError: If the resulting coordinates are out of bounds for the m×n array
    """
    i = 0  # Row index to reconstruct
    j = 0  # Column index to reconstruct
    
    # Extract interleaved bits (supports up to 64-bit indices)
    for k in range(64):  # More than enough for most practical cases
        # Extract even bits (0, 2, 4...) for row index i
        i |= ((z >> (2 * k)) & 1) << k
        # Extract odd bits (1, 3, 5...) for column index j
        j |= ((z >> (2 * k + 1)) & 1) << k
    
    # Check if coordinates are within the array bounds
    if i >= m or j >= n:
        return None
    
    return i, j
c = 0
m = 15
n = 8
for z in range(m*n+10):
    pair = z_order_to_2d(z, m, n)
		
    if pair is not None:
      x, y = pair
      c += 1
      print(f"z={z}, x={x}, y={y} , [{c}] {m*n}")

