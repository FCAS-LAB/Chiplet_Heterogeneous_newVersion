import yaml
import sys
import os
import argparse
import re

def modify_topology_and_flit(yaml_file_path, makefile_path, new_topology_path, flit_size):
    """
    Modify topology path and flit size parameters in YAML file
    
    Args:
        yaml_file_path: Path to the YAML file
        new_topology_path: New topology file path
        flit_size: New flit size value
    """
    try:
        # Check if YAML file exists
        if not os.path.exists(yaml_file_path):
            print(f"Error: YAML file does not exist: {yaml_file_path}")
            return False
        # Check if makefile exists
        if not os.path.exists(makefile_path):   
            print(f"Error: Makefile does not exist: {makefile_path}")
            return False
            
        # Read YAML file content
        with open(yaml_file_path, 'r', encoding='utf-8') as file:
            content = file.read()
        
        with open(makefile_path, 'r', encoding='utf-8') as file:
            makefile_content = file.read()
        
        file_pattern = r'(-f\s+)(\S+)'
        file_match = re.search(file_pattern, makefile_content)
        if file_match:
            old_flit_size = file_match.group(2)
            new_flit_size = str(flit_size)
            makefile_content = makefile_content.replace(old_flit_size, new_flit_size)
            print(f"Successfully modified flit size in Makefile:")
            print(f"  Old flit size: {old_flit_size}")
            print(f"  New flit size: {new_flit_size}")
        else:
            print("Warning: Flit size parameter not found in Makefile")
            
        # Modify topology path - find and replace "../topology/xxx.gv" pattern
        topology_pattern = r'(../topology/[^"]+\.gv)'
        topology_match = re.search(topology_pattern, content)
        if topology_match:
            old_topology = topology_match.group(1)
            content = content.replace(old_topology, new_topology_path)
            print(f"Successfully modified topology path:")
            print(f"  Old path: {old_topology}")
            print(f"  New path: {new_topology_path}")
        else:
            print("Warning: Topology path pattern not found")
        
        # Modify flit size parameter - find and replace "-F", "number" pattern
        flit_pattern = r'("-F",\s*")\d+(")'
        flit_match = re.search(flit_pattern, content)
        if flit_match:
            old_flit_part = flit_match.group(0)
            new_flit_part = f'"-F", "{flit_size}"'
            content = content.replace(old_flit_part, new_flit_part)
            print(f"Successfully modified flit size parameter:")
            print(f"  Old parameter: {old_flit_part}")
            print(f"  New parameter: {new_flit_part}")
        else:
            print("Warning: -F parameter not found")
        
        # Write back to file, preserving original format
        with open(yaml_file_path, 'w', encoding='utf-8') as file:
            file.write(content)
        
        with open(makefile_path, 'w', encoding='utf-8') as file:
            file.write(makefile_content)
        
        print(f"Successfully saved modifications to file: {yaml_file_path}")
        return True
        
    except FileNotFoundError:
        print(f"Error: File not found: {yaml_file_path}")
        return False
    except Exception as e:
        print(f"Error: {e}")
        return False

def main():
    """
    Main function, handles command line arguments
    """
    parser = argparse.ArgumentParser(description="Modify topology path and flit size in YAML file")
    parser.add_argument("--topology", default="mesh", help="Type of topology to modify")
    parser.add_argument("--flit_size", type=int, default=2, help="Flit size in bytes")
    parser.add_argument("--width", type=int, default=6, help="Topology width")
    parser.add_argument("--height", type=int, default=6, help="Topology height")
    
    args = parser.parse_args()
    
    # Build new topology path
    new_topology_path = f"../topology/{args.topology}_{args.width}_{args.height}_flit_{args.flit_size}.gv"
    yaml_file_path = "./mlp.yml"
    makefile_path = "./makefile"
    
    # Convert to absolute paths for debugging
    yaml_file_path = os.path.abspath(yaml_file_path)
    makefile_path = os.path.abspath(makefile_path)
    
    print(f"Preparing to modify configuration:")
    print(f"  YAML file: {yaml_file_path}")
    print(f"  Makefile: {makefile_path}")
    print(f"  New topology path: {new_topology_path}")
    print(f"  New flit size: {args.flit_size}")
    print("-" * 50)
    
    # Execute modifications
    success = modify_topology_and_flit(yaml_file_path, makefile_path, new_topology_path, args.flit_size)
    
    if success:
        print("Operation completed!")
        sys.exit(0)
    else:
        print("Operation failed!")
        sys.exit(1)

if __name__ == "__main__":
    main()