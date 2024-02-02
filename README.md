
### README for the System Monitoring Tool

#### Problem Solution Overview
The development of this tool involved parsing command-line arguments to tailor the output according to user needs, such as system or user usage, graphical representations, and control over sampling frequency and duration. The implementation focused on modular programming, avoiding global variables, and adhering to best practices in C programming. Essential system metrics are gathered without using shell commands, relying instead on direct system calls and reading from system files like `/proc/stat`, `/proc/uptime`, and others relevant for obtaining CPU and memory utilization, system uptime, and user sessions.

#### Functions Overview
- `print_running_parameters()`: Prints the current iteration or the number of samples and their frequency, along with memory usage.
- `print_system_information()`: Outputs basic system information such as system name, machine name, version, release, and architecture.
- `print_user_usage()`: Lists current user sessions by reading from the utmp file.
- `get_CPU_info()`, `calc_CPU_usage_percent()`, `print_CPU_usage()`: Gather CPU statistics, calculate usage percentage, and print it out.
- `print_CPU_graphics()`: Generates a graphical representation of CPU usage using a linked list.
- `calc_memory_usage()`, `print_memory_usage()`: Calculate and print physical and virtual memory usage.
- `add_memory_graphics()`: Adds a graphical representation of memory usage changes over time.
- `print_all_stats()`: Orchestrates the collection and printing of all statistics based on user-defined parameters.
- `isInteger()`: Utility function to validate integer inputs.
- `main()`: Parses command-line arguments to configure the program's operation and initiates the monitoring process.

#### How to Run the Program
1. **Compile the Program**: Use a C compiler (e.g., gcc) to compile the program. 

   Example: `gcc -o SystemStats SystemStats.c`

2. **Execution**: Run the compiled program with optional command-line arguments to customize its operation. 

   Example: `./SystemStats --samples=20 --tdelay=2 --system --user --graphics --sequential`.

   - `--samples=<N>`: Number of samples to collect.
   - `--tdelay=<T>`: Delay in seconds between samples.
   - `--system`: Generate system usage information.
   - `--user`: Generate user session information.
   - `--graphics`: Include graphical output for CPU and memory usage.
   - `--sequential`: Control the sequential generation of report data.

Ensure that your command-line environment has access to the necessary system files and permissions required to gather the information this program relies on.
