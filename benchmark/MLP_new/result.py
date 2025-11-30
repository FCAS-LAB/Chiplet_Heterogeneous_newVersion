import re
import argparse

def extract_benchmark_cycles(log_file, output_file):
    """
    Extract the Benchmark elapses cycle information after '**** End of Simulation ****'
    """
    try:
        with open(log_file, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # Find content after '**** End of Simulation ****'
        end_simulation_pattern = r'\*\*\*\* End of Simulation \*\*\*\*'
        match = re.search(end_simulation_pattern, content)
        
        if not match:
            print(f"'**** End of Simulation ****' marker not found")
            return
        
        # Get content after the match position
        after_end = content[match.end():]
        
        # Extract Benchmark elapses information
        benchmark_pattern = r'Benchmark elapses (\d+) cycle'
        benchmark_match = re.search(benchmark_pattern, after_end)
        
        if benchmark_match:
            cycles = benchmark_match.group(1)
            result = f"Benchmark elapses {cycles} cycle"
            
            # Write to result file
            with open(output_file, 'w', encoding='utf-8') as f:
                f.write(result + '\n')
            
            print(f"Extraction successful: {result}")
            print(f"Results saved to: {output_file}")
        else:
            print("Benchmark elapses information not found")
            
    except FileNotFoundError:
        print(f"File {log_file} does not exist")
    except Exception as e:
        print(f"Error processing file: {e}")

# Usage example
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Modify topology path and flit size in YAML file")
    parser.add_argument("--input", default="result_mesh_flit_2", help="Input log file path")
    
    args = parser.parse_args()
    log_file = args.input+".log"
    output_file = args.input+".txt"
    
    extract_benchmark_cycles(log_file, output_file)