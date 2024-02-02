#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/resource.h>
#include<sys/utsname.h>
#include<sys/sysinfo.h>
#include<sys/types.h>
#include<utmp.h>
#include<unistd.h>
#include<stdbool.h>

//define sturct for nodes used in the linked list
typedef struct NodeForLinkedList{
	char str[1024];
	struct NodeForLinkedList *next; // pointer pointing to the next node
} llNode;

//function to print the running parameters
void print_running_parameters(int samples, int tdelay, bool generateSquentially, int i){
	if(generateSquentially){
		printf(">>> iteration %d\n", i); // print the iteration number
	}
	else{
		printf("\033[H\033[2J"); // clear the terminal screen
		printf("Nbr of samples: %d -- every %d secs\n", samples, tdelay); // print number of samples and samples of frequency
	}

	struct rusage r_struct;
	getrusage(RUSAGE_SELF, &r_struct);
	printf(" Memory usage: %ld kilobytes\n", r_struct.ru_maxrss); // print memory self-utilization
	printf("---------------------------------------\n");
}

// function to print system information
void print_system_information(){
	struct utsname uts_struct;
	uname(&uts_struct); // get name and information about current kernel

	printf("---------------------------------------\n");
	printf("### System Information ### \n");
	printf(" System Name = %s\n", uts_struct.sysname); // print system name
	printf(" Machine Name = %s\n", uts_struct.nodename); // print machine name
	printf(" Version = %s\n", uts_struct.version); // print OS version
	printf(" Release = %s\n", uts_struct.release); // print OS release
	printf(" Architecture = %s\n", uts_struct.machine); // print machine architecture
	
	struct sysinfo sys_struct;
	sysinfo(&sys_struct); // get system information

	int seconds = sys_struct.uptime; // get the system running time since last reboot in seconds
	int hours = seconds / 3600; // calculate the number of hours
	seconds %= 3600;
	int minutes = seconds % 60; // calculate the number of minutes
	seconds %= 60;
	if(hours >= 24 && hours < 48){ // if the number of days is 1
		printf(" System running since last reboot: 1 day %02d:%02d:%02d (%02d:%02d:%02d)\n", 
		hours % 24, minutes, seconds, hours, minutes, seconds);
	}
	else{ // if the number of days is greater than 1
		printf(" System running since last reboot: %d days %02d:%02d:%02d (%02d:%02d:%02d)\n", 
		hours / 24, hours % 24, minutes, seconds, hours, minutes, seconds);
	}
	printf("---------------------------------------\n");
}

// function to print user usage
void print_user_usage(){
	printf("---------------------------------------\n");
	printf("### Sessions/users ### \n");

	setutent(); // rewind the file pointer to the beginning of the utmp file
	struct utmp *ut_pt = getutent(); // read a line from the utent file

	while(ut_pt != NULL){
		if(ut_pt -> ut_type == USER_PROCESS){ // check whether the user is connected
			printf(" %s       %s (%s)\n", ut_pt -> ut_user, ut_pt -> ut_line, ut_pt -> ut_host); // print user information
		}
		ut_pt = getutent(); // get the pointer to a user
	}

	endutent(); //close the utmp file

	printf("---------------------------------------\n");
}

// function to get the CPU information
void get_CPU_info(int *idle, int *sum){
	FILE *fp = fopen("/proc/stat", "r"); // open /proc/stat to read information
	if(fp == NULL){ // if an error occurs when opening /proc/stat
		fprintf(stderr, "Error opening file\n"); // output error message
		exit(1);
	}

	char stat_buffer[256];
	fgets(stat_buffer, sizeof(stat_buffer), fp); // store the content of the first line in stat_buffer
	fclose(fp);
	char *token = strtok(stat_buffer, " "); // skip the name of the CPU
	*idle = 0;
	*sum = 0;

	for(int i = 0; token != NULL; ++i){
		token = strtok(NULL, " "); // get a parameter
		if(token == NULL){ // if all the parameters have been gone over
			break;
		}
		*sum += atoi(token); // add the number to the total cpu
		if(i == 3 || i == 4){
			*idle += atoi(token); // add up idle and iowait in /proc/stat
		}
	}
}

// funcation to calculate the percentage of CPU usage
float calc_CPU_usage_percent(int *last_idle, int *last_sum){
	int idle, sum;
	get_CPU_info(&idle, &sum); // get the CPU information for calculation

	float idle_delta = (float)(idle - *last_idle); // the amount idle changes since last sample
	float sum_delta = (float)(sum - *last_sum); // the amount total cpu changes since last sample
	float cpu_percent = (1000 * ((sum_delta - idle_delta) / sum_delta) + 1) / 10; // calculate the percentage for cpu usage

	*last_idle = idle; // store idle as the last idle
	*last_sum = sum; // store the total cpu as the last total cpu
	return cpu_percent;
}

// function to print CPU usage without graphics and return the percentage of CPU usage
float print_CPU_usage(int *last_idle, int *last_sum){
	printf("Number of cores: %d \n", get_nprocs()); // print the number of cores
	float cpu_percent = calc_CPU_usage_percent(last_idle, last_sum); // store the percentage of CPU usage
	printf(" total cpu use = %.2f%%\n", cpu_percent); // print total CPU usage
	return cpu_percent;
}

// function to print the graphics of CPU usage
void print_CPU_graphics(float current_cpu, bool generateSquentially, llNode **cpu_head){
	llNode *new_node = (llNode *)malloc(sizeof(llNode)); // use malloc to allocate memory for a new node
	if(new_node == NULL){ // if no available memory is left
		fprintf(stderr, "Error: malloc failed\n");
		exit(1);
	}

	new_node->next = NULL;
	if(*cpu_head == NULL){ // set the new node as the head of the linked list if the linked list is empty
		*cpu_head = new_node;
	}
	else{ // add the new node to the tail of the linked list if the linked is not empty
		llNode *node = *cpu_head;
		while(node->next != NULL){
			node = node->next;
		}
		node->next = new_node; 
	}

	strcpy(new_node->str, "         ");
	int times = current_cpu;
	for(int j = 1; j <= times + 3; ++j){ // add the desired number of '|' to the string
		strcat(new_node->str, "|");
	}
	char str[32];
	sprintf(str, " %.2f", current_cpu);
	strcat(new_node->str, str); // add the cpu usage to the string

	if(!generateSquentially){ // if report needs to be generated sequentially
		llNode *node = *cpu_head;
		// print the strings one by one
		while(node != NULL){
			printf("%s\n", node->str);
			node = node->next;
		}
	}
	else{ // if report does not need to be generated sequentially
		llNode *node = *cpu_head;
		// only print the current string on one line
		while(node->next != NULL){
			printf("\n");
			node = node->next;
		}
		printf("%s\n", node->str);
	}
}

// function to calculate the memory usage and return the virtual RAM memory used
float calc_memory_usage(llNode **memory_head){
	printf("### Memory ### (Phys.Used/Tot -- Virtual Used/Tot) \n"); // print the first line of memory report

	llNode *new_node = (llNode *)malloc(sizeof(llNode)); // use malloc to allocate memory for a new node
	if(new_node == NULL){ // if no available memory is left
		fprintf(stderr, "Error: malloc failed\n");
		exit(1);
	}

	new_node -> next = NULL;
	//insert the new node to the tail of the linked list
	if(*memory_head == NULL){
		*memory_head = new_node;
	}
	else{
		llNode *node = *memory_head;
		while(node->next != NULL){
			node = node->next;
		}
		node->next = new_node;
	}

	struct sysinfo sys_struct;
	sysinfo(&sys_struct); // get system information and store it in sys_struct
	float phys_used = (float)(sys_struct.totalram - sys_struct.freeram)/1000/1000/1000; // calculate the physical RAM memory used (converted to GB)
    float phys_total = (float)sys_struct.totalram/1000/1000/1000; // calculate the physical RAM memory in total (converted to GB)
	float virtual_used = phys_used + (float)(sys_struct.totalswap - sys_struct.freeswap) /1000/1000/1000; // calculate the virtual RAM memory used (converted to GB)
    float virtual_total = (float)(sys_struct.totalram + sys_struct.totalswap)/1000/1000/1000; // calculate the virtual RAM memory in total (converted to GB)

	// update the string with the memory information
	sprintf(new_node->str, "%.2f GB / %.2f GB -- %.2f GB / %.2f GB", phys_used,
			phys_total, virtual_used, virtual_total);
	return virtual_used;
}

// function to print memory usage
void print_memory_usage(int samples, bool generateSquentially, int i, llNode *memory_head){
	if(!generateSquentially){ // if report does not need to be generated sequentially
		llNode *node = memory_head;
		// print all the generated reports of memory usage
		while(node != NULL){
			printf("%s\n", node->str);
			node = node->next;
		}
		for(int j = i + 1; j < samples; ++j){
			printf("\n");
		}
	}
	else{ // if report needs to be generated sequentially
		// find the current node;
		llNode *node = memory_head;
		for(int j = 0; j < i; ++j){
			node = node->next;
		}
		// print the report only on the current line
		for(int j = 0; j < samples; ++j){
			if(j == i){
				printf("%s\n", node->str);
			}
			else{
				printf("\n");
			}
		}
	}
}

// function to add memory usage graphics to the end of the memory usage report
void add_memory_graphics(float *last_virtual, float current_virtual, int i, llNode *memory_head){
	// find the current node
	llNode *node = memory_head;
	for(int j = 0; j < i; ++j){
		node = node->next;
	}

	strcat(node->str, "   |"); // generate the delimiter

	float delta = (*last_virtual == -1.00)?0:(current_virtual - *last_virtual); // calculate the change in virtual memory usage
	if(delta >= 0 && delta < 0.01){ // generate the symbol for zero+
		strcat(node->str, "o ");
	}
	else if(delta < 0 && delta > -0.01){ // generate the symbol for zero-
		strcat(node->str, "@ ");
	}
	else{ // generate the symbols for changes that do not round to 0.00
		int times = delta * 100;
		for(int i = 1; i <= times; ++i){
			strcat(node->str, (delta < 0)?":":"#"); // ':' for negative and '#' for positve
		}
		strcat(node->str, (delta < 0)?"@ ":"* "); // '@' for negative and '*' for postive
	}

	char str[128];
	sprintf(str, "%.2f (%.2f)", delta, current_virtual);
	strcat(node->str, str);  // generate the changes in memory usage and the virtual memory usage

	*last_virtual = current_virtual; // store the current virtual memory usage for later use
}

// function to print all the stats needed to be reported
void print_all_stats(int samples, int tdelay, bool generateSystem, bool generateUser,
				bool generateGraphics, bool generateSquentially){
	int last_idle = 0, last_sum = 0;
	float last_virtual = -1, current_virtual = 0, current_cpu = 0;
	llNode *memory_head = NULL, *cpu_head = NULL;

	get_CPU_info(&last_idle, &last_sum); // get the CPU information for the first sample
	for(int i = 0; i < samples; ++i){
		sleep(tdelay); // wait for tdelay seconds
		
		print_running_parameters(samples, tdelay, generateSquentially, i); // print the running parameters	
		if(!generateSystem){ // system usage should not be generated
			print_user_usage(); // print user usage
		}
		else{ // system usage should be generated
			current_virtual = calc_memory_usage(&memory_head); // calculate memory usage and store virtual memory usage
			if(generateGraphics){
				add_memory_graphics(&last_virtual, current_virtual, i, memory_head); // generate graphics for memory usage
			}
			print_memory_usage(samples, generateSquentially, i, memory_head); //  print memory usage
			
			if(generateUser){
				print_user_usage(); // print user usage
			}

			current_cpu = print_CPU_usage(&last_idle, &last_sum); // print CPU usage
			if(generateGraphics){
				print_CPU_graphics(current_cpu, generateSquentially, &cpu_head); // print graphics for CPU usage
			}
		}
	}

	print_system_information(); //  print system information

	// delete the linked lists by freeing all the nodes
	llNode *node;
	while(memory_head != NULL){
		node = memory_head->next;
		free(memory_head);
		memory_head = node;
	}
	while(cpu_head != NULL){
		node = cpu_head->next;
		free(cpu_head);
		cpu_head = node;
	}
}

// function to check whether the string is an integer
bool isInteger(const char *str){
	if(!*str) return false; // empty string

	char *ptr;
	strtol(str, &ptr, 10); // attempt to convert str to a long
	return *ptr == '\0'; // check whether str can be fully converted to a long (can be considered as an integer)
}

int main(int argc, char *argv[]){
	int samples = 10, tdelay = 1; // number of samples and sample frequency
	bool generateSystem = true; // whether or not to generate the system usage
	bool generateUser = true; // whether or not to generate the users usage
	bool generateGraphics = false; // whether or not to generate the graphical output
	bool generateSquentially = false; // whether or not to generate sequentially
	
	if(argc > 1){
		for(int i = 1; i < argc; ++i){
			char *token = strtok(argv[i], "="); // get the part of argv[i] before '='
			if(!strcmp(argv[i], "--system")){ // argv[i] equals "--system", meaning only system usage should be generated
				generateUser = false;
			}
			else if(!strcmp(argv[i], "--user")){ // argv[i] equals "--user", meaning only user usage should be generated
				generateSystem = false;
			}
			else if(!strcmp(argv[i], "--graphics")){ // argv[i] equals "--graphics", meaning graphical output should be generated
				generateGraphics = true;
			}
			else if(!strcmp(argv[i], "--sequential")){ // argv[i] equals "--sequential", meaning the reports should be generated sequentially
				generateSquentially = true;
			}
			else if(!strcmp(token, "--samples")){ // argv[i] equals "--samples=N", meaning N samples should be collected
				samples = atoi(strtok(NULL, ""));
			}
			else if(!strcmp(token, "--tdelay")){ // argv[i] equals "--tdelay=T", meaning sampling frequency is tdelay seconds
				tdelay = atoi(strtok(NULL, ""));
			}
			else if(isInteger(argv[i])){ // argv[i] is an integer
				if(i + 1 >= argc || !isInteger(argv[i + 1])){ // if the next argument is not an integer or there is no next argument
					fprintf(stderr, "Invalid command line arguments!\n");
					return 1;
				}
				// store the number of samples and the sampling frequency
				samples = atoi(argv[i]);
				tdelay = atoi(argv[++i]);
			}
			else{ // the command line arguments are invalid inputs
				fprintf(stderr, "Invalid command line arguments!\n");
				return 1;
			}
		}
	}

	// cases for --system and --user both occur in the command line arguments
	if(!generateSystem && !generateUser){
		generateSystem = true;
		generateUser = true;
	}

	// print all the stats
	print_all_stats(samples, tdelay, generateSystem, generateUser, generateGraphics, generateSquentially);
	return 0;
}