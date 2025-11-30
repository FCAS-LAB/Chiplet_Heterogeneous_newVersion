import numpy as np
import matplotlib.pyplot as plt
import re
from collections import Counter
from matplotlib.ticker import MaxNLocator
import argparse 

def main():
    parser = argparse.ArgumentParser(description="Modify topology path and flit size in YAML file")
    parser.add_argument("--topology", default="mesh", help="Type of topology to modify")
    parser.add_argument("--flit_size", type=int, default=2, help="Flit size in bytes")
    
    args = parser.parse_args()
    # Step 1: Read the log data from popnet.log file
    with open("./proc_r1_p2_t0/popnet.log", "r") as file:
        log_data = file.readlines()

    # Step 2: Parse the data to extract router communication pairs
    pattern = re.compile(r"From Router (\d+) to Router (\d+)")
    matches = [pattern.search(line).groups() for line in log_data if pattern.search(line)]
    counter = Counter(matches)

    # Step 3: Convert router numbers to coordinates in a 6x6 grid
    def router_to_coordinates(router_id):
        y = int(router_id) // 6
        x = int(router_id) % 6
        print("router_id: ", router_id, "x: ", x, "y: ", y)
        return x, y

    # Step 4: Initialize the communication matrix
    comm_matrix = np.zeros((6, 6))

    # Populate the communication matrix with counts
    for (src, dest), count in counter.items():
        src_x, src_y = router_to_coordinates(src)
        dest_x, dest_y = router_to_coordinates(dest)
        comm_matrix[src_x, src_y] += count

    # Step 5: Plot the heatmap
    plt.figure(figsize=(8, 6))
    plt.imshow(comm_matrix, cmap='hot', interpolation='nearest')
    cbar = plt.colorbar(label='Traffic Count', fraction=0.046, pad=0.04)
    cbar.set_label('Traffic volume', fontsize=25)  # Set colorbar label font size
    cbar.ax.tick_params(labelsize=14)  # Set colorbar tick label size
    # plt.title('Heatmap of Communication Frequency Between Routers')
    plt.xlabel('Y', fontsize=25)
    plt.ylabel('X', fontsize=25)
    plt.xticks(fontsize=20)  # X-axis tick labels
    plt.yticks(fontsize=20)  # 
    # Ensure only integers are displayed on ticks
    plt.gca().xaxis.set_major_locator(MaxNLocator(integer=True))  # X-axis
    plt.gca().yaxis.set_major_locator(MaxNLocator(integer=True))  # Y-axis
    # plt.show()

    plt.savefig(f"router_traffic_heatmap_{args.topology}_{args.flit_size}.png", dpi=300, bbox_inches='tight')

if __name__ == "__main__":
    main()